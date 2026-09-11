# Building SIPHER 1.0.0-r18 in Termux

Run `./build.sh --os termux` from Termux. The builder installs the native clang/CMake/Ninja/Qt 6 stack, bootstraps the same patched PJSIP 2.17 core, builds CLI and GUI, and installs into `$PREFIX/bin`. The CLI runs directly in Termux; the GUI requires Termux:X11.

Android may restrict raw packet capture and audio backends more strongly than desktop Unix. Those limitations are reported at runtime rather than weakening Android security controls.
