# Building SIPHER 1.0.0-r17 on macOS

Run `./build.sh --os macos`. The builder targets macOS 13+, preflights full Xcode, bootstraps Homebrew when allowed, installs CMake/Ninja/pkgconf/Qt 6 and required VoIP dependencies, builds the project's managed patched PJSIP 2.17, deploys Qt into a standalone `.app`, ad-hoc signs it for local testing, and creates a DMG by default.

Use `--no-auto-deps` for audit-only dependency behavior, `--no-dmg` to skip the DMG, or `--install` to copy the app to `/Applications` and create command launchers under `/usr/local/bin`.
