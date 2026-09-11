# Building SIPHER 2.1 in Termux

Run `./build.sh --os termux` from Termux. The builder installs the native clang/CMake/Ninja/Qt 6 stack, bootstraps the same patched PJSIP 2.17 core, builds CLI and GUI, and installs into `$PREFIX/bin`. The CLI runs directly in Termux; the GUI requires Termux:X11.

Android may restrict raw packet capture and audio backends more strongly than desktop Unix. Those limitations are reported at runtime rather than weakening Android security controls.

The installed application is a single `sipher` executable. A normal Termux TTY selects CLI; use `sipher --gui` when launching it against Termux:X11.
