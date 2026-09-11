# SIPHER — All-Platform Builder

This release keeps the existing S.I.P.H.E.R. 1.0.0 r17 Exploit-Fix Phreak Lab application core and adds the WaffleHouse-style platform builder layer.

Supported builder targets:

- Linux (Debian/Ubuntu/Mint and Fedora family via the existing dependency-aware builder)
- FreeBSD (existing base-Clang/libc++, PJSIP, BPF/devfs, and audio compatibility path)
- macOS 13+ (full Xcode preflight, Homebrew dependencies, managed PJSIP 2.17, Qt deployment, ad-hoc local signing, optional /Applications install, and DMG creation)
- Termux/Android (native clang/CMake/Ninja build; CLI in Termux and Qt GUI through Termux:X11)

Start with `./build.sh`. The host is auto-detected, or select explicitly with `./build.sh --os linux|freebsd|macos|termux`.

The macOS and Termux builds use the same C++ core as Linux/FreeBSD. Platform-specific packet-capture and audio behavior remains subject to host permissions/backends.
