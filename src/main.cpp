#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

#include <array>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace {
    // This MVP records the game's input timeline. It does NOT claim to calculate
    // a true frame window: that requires deterministic replay + per-input trials.
    constexpr int kTPS = 240;
    constexpr size_t kMaxRecords = 100000;
    constexpr char kHudID[] = "temu-fpc-hud";
    constexpr char kDrawID[] = "temu-fpc-markers";

    struct InputRecord {
        long long tick = 0;
        int button = 1;
        bool down = false;
        bool player1 = true;
        float x = 0.f;
        float y = 0.f;
        int gapBucket = -1; // -1 = first click; 0..3 = interval bucket
    };

    std::vector<InputRecord> g_records;
    std::array<int, 4> g_gapCounts{0, 0, 0, 0};
    long long g_lastDownTick[2] = {-1, -1};
    int g_totalDowns = 0;
    bool g_exportedThisRun = false;

    ccColor4F bucketColor(int bucket, bool player1) {
        // Four distinct window-like interval classes; P2 is shown with lower opacity.
        ccColor4F colors[] = {
            {0.25f, 0.95f, 0.45f, 0.95f}, // 1 tick
            {0.20f, 0.75f, 1.00f, 0.95f}, // 2 ticks
            {1.00f, 0.78f, 0.20f, 0.95f}, // 3-4 ticks
            {1.00f, 0.30f, 0.35f, 0.95f}  // 5+ ticks
        };
        auto c = colors[bucket < 0 ? 3 : bucket];
        if (!player1) c.a = 0.60f;
        return c;
    }

    void resetRunData() {
        g_records.clear();
        g_gapCounts = {0, 0, 0, 0};
        g_lastDownTick[0] = g_lastDownTick[1] = -1;
        g_totalDowns = 0;
        g_exportedThisRun = false;
    }

    long long currentTick(GJBaseGameLayer* layer) {
        if (!layer) return 0;
        return static_cast<long long>(std::llround(layer->m_gameState.m_levelTime * kTPS));
    }

    void updateHud(PlayLayer* pl) {
        if (!pl || !pl->m_uiLayer) return;
        auto label = typeinfo_cast<CCLabelBMFont*>(pl->m_uiLayer->getChildByID(kHudID));
        if (!label) return;
        std::string s = fmt::format(
            "TEMU FPC (INPUT GAP)\n1 tick: {}\n2 ticks: {}\n3-4 ticks: {}\n5+ ticks: {}\nInputs: {}",
            g_gapCounts[0], g_gapCounts[1], g_gapCounts[2], g_gapCounts[3], g_totalDowns
        );
        label->setString(s.c_str());
    }

    void clearMarkers(PlayLayer* pl) {
        if (!pl) return;
        auto removeFrom = [](CCNode* node) {
            if (!node) return;
            if (auto draw = node->getChildByID(kDrawID)) draw->removeFromParentAndCleanup(true);
        };
        removeFrom(pl);
        if (pl->m_player1) removeFrom(pl->m_player1->getParent());
        if (pl->m_player2) removeFrom(pl->m_player2->getParent());
    }

    void drawMarker(PlayLayer* pl, CCNode* player, int bucket, bool player1) {
        if (!pl || !player || !Mod::get()->getSettingValue<bool>("show-markers")) return;
        auto parent = player->getParent();
        if (!parent) return;
        auto draw = typeinfo_cast<CCDrawNode*>(parent->getChildByID(kDrawID));
        if (!draw) {
            draw = CCDrawNode::create();
            if (!draw) return;
            draw->setID(kDrawID);
            parent->addChild(draw, 999);
        }
        // Use the player's local coordinates because the marker node shares its parent.
        draw->drawDot(player->getPosition(), 7.0f, bucketColor(bucket, player1));
    }

    void exportCSV() {
        if (g_exportedThisRun || g_records.empty()) return;
        g_exportedThisRun = true;
        auto path = Mod::get()->getSaveDir() / "last_run_inputs.csv";
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            log::warn("Could not write input CSV to {}", path.string());
            return;
        }
        out << "tick,button,down,player1,x,y,gap_bucket\n";
        for (auto const& r : g_records) {
            out << r.tick << ',' << r.button << ',' << (r.down ? 1 : 0) << ','
                << (r.player1 ? 1 : 0) << ',' << r.x << ',' << r.y << ',' << r.gapBucket << '\n';
        }
        log::info("Saved {} input events to {}", g_records.size(), path.string());
    }

    int classifyGap(long long gap) {
        if (gap <= 1) return 0;
        if (gap == 2) return 1;
        if (gap <= 4) return 2;
        return 3;
    }
}

class $modify(TemuFPCPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        resetRunData();
        clearMarkers(this);

        if (!m_uiLayer) return;
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        if (!label) return;
        label->setID(kHudID);
        label->setAnchorPoint({0.f, 1.f});
        label->setScale(0.34f);
        label->setOpacity(220);
        auto size = CCDirector::get()->getWinSize();
        label->setPosition({6.f, size.height - 6.f});
        m_uiLayer->addChild(label, 1000);
        updateHud(this);
    }

    void resetLevel() {
        exportCSV();
        PlayLayer::resetLevel();
        if (Mod::get()->getSettingValue<bool>("reset-on-death")) resetRunData();
        else {
            g_lastDownTick[0] = g_lastDownTick[1] = -1;
            g_exportedThisRun = false;
        }
        clearMarkers(this);
        updateHud(this);
    }

    void levelComplete() {
        exportCSV();
        PlayLayer::levelComplete();
    }
};

class $modify(TemuFPCBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        auto pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != this) return;
        if (!Mod::get()->getSettingValue<bool>("record-inputs")) return;
        if (g_records.size() >= kMaxRecords) return;

        int p = isPlayer1 ? 0 : 1;
        auto player = isPlayer1 ? static_cast<CCNode*>(pl->m_player1) : static_cast<CCNode*>(pl->m_player2);
        long long tick = currentTick(this);
        int bucket = -1;

        // Count down transitions only. Releases are still recorded for replay analysis.
        if (down) {
            if (g_lastDownTick[p] >= 0) {
                bucket = classifyGap(tick - g_lastDownTick[p]);
                g_gapCounts[bucket]++;
            }
            g_lastDownTick[p] = tick;
            g_totalDowns++;
            drawMarker(pl, player, bucket, isPlayer1);
        }

        InputRecord rec;
        rec.tick = tick;
        rec.button = button;
        rec.down = down;
        rec.player1 = isPlayer1;
        rec.gapBucket = bucket;
        if (player) {
            auto pos = player->getPosition();
            rec.x = pos.x;
            rec.y = pos.y;
        }
        g_records.push_back(rec);
        updateHud(pl);
    }
};
