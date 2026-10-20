# Changelog

## v0.1.0 — Initial prototype

- Add in-level HUD for input interval buckets.
- Record press and release events that reach the Geode game-layer input hook.
- Draw colored press markers at the player's position.
- Export the latest run to CSV.
- Add Geode settings for recording, marker display, and reset behavior.
- Add GitHub Actions builds for Android32 and Android64.

### Known limitations

- Not a true frame-window scanner yet.
- No macro import or dedicated bot adapters yet.
- Not all replay/bot tools are guaranteed to route inputs through the same hook.
