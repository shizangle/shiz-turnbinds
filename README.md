# Shiz-Turnbinds (with pause bind)

Re-creation of [shizangle/shiz-turnbinds](https://github.com/shizangle/shiz-turnbinds) (MIT)
with an added **pause hotkey**.

- Smooth turnbinds for CS2 with a strafe modifier
- Draggable always-on-top yaw overlay (scroll over it to change `cl_yawspeed`)
- Auto activate / deactivate when CS2 is focused
- Rebindable left / right / modifier keys
- **NEW: Pause key (default `F8`)** - toggles the turnbinds on/off while in game.
  Rebindable from the console menu (select "Pause key", press Enter, press the new key).
  The overlay turns amber and shows PAUSED, and the console shows the state.

## Download and run

1. Go to the **Releases** page of this repo and download `Shiz-Turnbinds-windows-x64.zip` (or the bare `.exe`).
2. Put the exe in any folder you like and run it. No installer, no admin, no extra runtimes.
3. `settings.json` is created next to the exe the first time you change something.

Windows SmartScreen may say "unknown publisher" because the exe isn't code-signed: click *More info* -> *Run anyway*.
Some antivirus tools flag any program that sends mouse input; the full source is in this repo.

## Publishing a new release (maintainers)

    git tag v1.0.0
    git push origin v1.0.0

The GitHub Actions workflow in `.github/workflows/release.yml` builds the exe on a Windows runner and attaches it
to a new Release automatically.

## Build it yourself (Windows, Visual Studio 2022 / CMake)

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release

The exe lands in `build/Release/`. Keep `settings.json` next to it (it is created automatically if missing).
Static CRT: no extra installs, no admin needed.

## Console controls

Up/Down select - Left/Right adjust - Enter toggles/rebinds - Q quits.

## Notes
- Reads/writes nothing from the game; it only polls key state and sends mouse-move input.
- The pause key only reacts while CS2 is the focused window.
- Original code (c) shizangle, MIT license. Keep the original LICENSE notice if you redistribute.
