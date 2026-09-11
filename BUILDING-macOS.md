# Building SIPHER 2.0 on macOS

Run `./build.sh --os macos`. The builder targets macOS 13+, preflights full Xcode, bootstraps Homebrew when allowed, installs CMake/Ninja/pkgconf/Qt 6 and required VoIP dependencies, builds the project's managed patched PJSIP 2.17, deploys Qt into a standalone `.app`, ad-hoc signs it for local testing, and creates a DMG by default.

Use `--no-auto-deps` for audit-only dependency behavior, `--no-dmg` to skip the DMG, or `--install` to copy the app to `/Applications` and create command launchers under `/usr/local/bin`.

The app bundle contains one `sipher` executable. Launching SIPHER.app from Finder opens the GUI; invoking its executable from a Terminal opens the CLI. `--gui` and `--cli` override auto-selection.
