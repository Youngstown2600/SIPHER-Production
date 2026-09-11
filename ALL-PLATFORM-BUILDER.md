# SIPHER — All-Platform Builder

SIPHER 2.1 is based on the r19 Multi-SIP line and carries forward the r18 DID/route/audit and security fixes. The unified platform builder remains available across Linux, FreeBSD, macOS, and Termux/Android.

A normal `--all` build produces one public executable named `sipher`. From a terminal/TTY it opens the CLI; from a graphical desktop launch without a TTY it opens the GUI. `sipher --cli`, `sipher --gui`, and `SIPHER_UI=cli|gui` override automatic selection.

Supported builder targets:

- Linux (Debian/Ubuntu/Mint and Fedora family via the existing dependency-aware builder)
- FreeBSD (existing base-Clang/libc++, PJSIP, BPF/devfs, and audio compatibility path)
- macOS 13+ (full Xcode preflight, Homebrew dependencies, managed PJSIP 2.17, Qt deployment, ad-hoc local signing, optional /Applications install, and DMG creation)
- Termux/Android (native clang/CMake/Ninja build; CLI in Termux and Qt GUI through Termux:X11)

Start with `./build.sh`. The host is auto-detected, or select explicitly with `./build.sh --os linux|freebsd|macos|termux`.

The macOS and Termux builds use the same C++ core as Linux/FreeBSD. Platform-specific packet-capture and audio behavior remains subject to host permissions/backends.
