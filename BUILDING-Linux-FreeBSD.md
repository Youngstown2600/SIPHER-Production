# Building SIPHER 2.1 on Linux or FreeBSD

The original dependency-aware Unix builder is retained. Run `./build.sh --os linux` or `./build.sh --os freebsd` (or simply `./build.sh` on the matching host). Existing Linux packet-capture capability setup and FreeBSD BPF/devfs + audio compatibility handling remain intact.

A default `--all` build produces a single `sipher` executable. Terminal/TTY launch selects CLI; desktop launch selects GUI. Use `--cli` or `--gui` to force a frontend.
