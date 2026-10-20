# Temu Frame Perfect Counter

An independently implemented **Geometry Dash Geode mod prototype** focused on recording the input timeline, displaying colored press markers, and exporting run data for analysis.

> **Status: prototype.** This project does not yet calculate a true frame window like a full frame-window scanner. Its current buckets describe the interval between recorded press events; they are not proof that an input is a frame perfect.

## Features in this version

- On-screen HUD with input interval buckets and total recorded presses
- Colored markers at the player's position when a press is recorded
- Records press/release events for player 1 and player 2 when they pass through `GJBaseGameLayer::handleButton`
- CSV export of the latest run to the mod's Geode save directory
- Geode settings to toggle recording, markers, and reset-on-death
- GitHub Actions workflow for Android 64-bit and 32-bit builds

## Not implemented yet

- True per-input frame-window scanning through controlled replay and shifted-input trials
- Importing bot macro files
- Guaranteed compatibility with every bot or replay mod
- Automatic replay/scan analysis comparable to a dedicated frame-window scanner

Bot support depends on whether a bot sends its inputs through the same game hook. Each bot/replay implementation must be tested separately.

## Build on GitHub Actions

No local Android SDK or Geode toolchain is needed on your phone.

1. Create a **new empty GitHub repository**.
2. Extract this ZIP file.
3. Upload **the contents of the extracted folder** to the repository root. Make sure `.github/workflows/build-android.yml` is included. Do not upload only the outer folder.
4. Commit the files to the `main` branch.
5. Open the repository's **Actions** tab and select **Build Android**. The workflow also runs automatically after a push to `main`.
6. Open the completed workflow run. Under **Artifacts**, download `temu-fpc-Android64` for a 64-bit Android device or `temu-fpc-Android32` for a 32-bit device.
7. Extract the downloaded artifact if GitHub gives you a ZIP, then install the `.geode` file using your normal Geode installation method.

If a build fails, open the failed workflow run and read the first error in the build log. The code has not been compiled or tested on a physical device as part of preparing this source ZIP, so the first Actions run is the real build check.

## Building locally

Use a Geode SDK/toolchain compatible with the `geode` and `gd.android` versions in `mod.json`, then run:

```sh
geode sdk install
geode build -p android64
```

See the [official Geode build guide](https://docs.geode-sdk.org/getting-started/create-mod/) and [official GitHub Actions build action](https://github.com/geode-sdk/build-geode-mod).

## CSV data

The exported `last_run_inputs.csv` contains the tick, button, press/release state, player, recorded player coordinates, and interval bucket. The game's level time is converted to a 240-tick timeline for this prototype; treat the resulting buckets as analysis data, not validated frame-window measurements.

## Contributing

Bug reports and improvements are welcome. When reporting an issue, include the Geometry Dash version, Geode version, Android architecture (32-bit or 64-bit), and the relevant GitHub Actions log. Do not attach private account tokens or other secrets.

## License

This project is provided under the MIT License. It is an independent implementation and is not affiliated with or endorsed by the developers of FB Frame Counter.
