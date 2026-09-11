#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR=$(cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT_DIR"
PRODUCT="SIPHER"; VERSION="2.0"; SLUG="sipher"; BINARY_NAME="sipher"
[[ -n "${TERMUX_VERSION:-}" || "${PREFIX:-}" == *com.termux* ]] || { echo "build-termux.sh must run inside Termux." >&2; exit 2; }
PREFIX=${PREFIX:-/data/data/com.termux/files/usr}
PJSIP_PREFIX=${PJSIP_PREFIX:-$HOME/.local/sipher-pjsip}
BUILD_DIR=${BUILD_DIR:-$ROOT_DIR/build-termux}
BUILD_CLI=1; BUILD_GUI=1; CLEAN=0; FORCE_PJSIP=0; AUTO_DEPS=1; INSTALL_MODE=yes; UNINSTALL=0
explicit_target=0
usage(){ cat <<USAGE
$PRODUCT $VERSION — Termux/Android builder
Usage: ./build.sh --os termux [--cli|--gui|--all] [--clean] [--pjsip] [--no-auto-deps] [--uninstall]
GUI builds use Termux:X11; CLI runs in the normal Termux terminal.
USAGE
}
while [[ $# -gt 0 ]]; do
 case "$1" in
  --cli) [[ $explicit_target -eq 1 ]] || { BUILD_CLI=0; BUILD_GUI=0; explicit_target=1; }; BUILD_CLI=1 ;;
  --gui) [[ $explicit_target -eq 1 ]] || { BUILD_CLI=0; BUILD_GUI=0; explicit_target=1; }; BUILD_GUI=1 ;;
  --all) BUILD_CLI=1; BUILD_GUI=1; explicit_target=1 ;;
  --clean) CLEAN=1 ;; --pjsip) FORCE_PJSIP=1 ;; --auto-deps) AUTO_DEPS=1 ;; --no-auto-deps) AUTO_DEPS=0 ;;
  --install|--yes|-y) INSTALL_MODE=yes ;; --no-install) INSTALL_MODE=no ;; --uninstall|--remove-only) UNINSTALL=1 ;;
  --pjsip-prefix) shift; [[ $# -gt 0 ]] || { echo "--pjsip-prefix requires a path" >&2; exit 2; }; PJSIP_PREFIX=$1 ;;
  --deps|--bootstrap-only) AUTO_DEPS=1; BUILD_CLI=0; BUILD_GUI=0 ;;
  --configure-capture|--audio-diagnose|--no-audio-fix|--purge-user-data) echo "$1 is not used by the Termux builder." >&2 ;;
  --dry-run) echo "Termux builder: clang/cmake/ninja/pkg-config/git/Qt6/PJSIP 2.17; no changes made."; exit 0 ;;
  -h|--help) usage; exit 0 ;; *) echo "Unknown Termux option: $1" >&2; usage >&2; exit 2 ;;
 esac; shift
done
if [[ $UNINSTALL -eq 1 ]]; then
  rm -f "$PREFIX/bin/$BINARY_NAME" "$PREFIX/bin/sipher-gui" "$PREFIX/bin/sipher-cli" # remove 2.0 + legacy names
  echo "$PRODUCT application files removed from Termux. User configuration was preserved."; exit 0
fi
if [[ $AUTO_DEPS -eq 1 ]]; then
  pkg update -y
  pkg install -y x11-repo
  pkg update -y
  pkg install -y clang cmake ninja pkg-config make git curl perl python openssl qt6-qtbase
  for p in portaudio opus ffmpeg tcpdump libpcap libuuid; do pkg install -y "$p" || echo "WARNING: optional Termux package '$p' is unavailable; continuing." >&2; done
else
  for c in clang clang++ cmake ninja pkg-config make git; do command -v "$c" >/dev/null || { echo "Missing required command: $c" >&2; exit 1; }; done
fi
# dependency/bootstrap-only path
if [[ $BUILD_CLI -eq 0 && $BUILD_GUI -eq 0 ]]; then echo "$PRODUCT Termux prerequisites are ready."; exit 0; fi
export CC=clang CXX=clang++
export TERMUX_SYS_PREFIX="$PREFIX"
export PKG_CONFIG_PATH="$PJSIP_PREFIX/lib/pkgconfig:$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export CMAKE_PREFIX_PATH="$PREFIX:${CMAKE_PREFIX_PATH:-}"
if [[ $FORCE_PJSIP -eq 1 ]]; then rm -f "$PJSIP_PREFIX/.sipher-pjsip-build"; fi
if [[ ! -f "$PJSIP_PREFIX/.sipher-pjsip-build" ]] || ! pkg-config --exists 'libpjproject = 2.17' 2>/dev/null; then
  PJSIP_PREFIX="$PJSIP_PREFIX" "$ROOT_DIR/scripts/bootstrap-pjsip.sh"
fi
[[ $CLEAN -eq 0 ]] || rm -rf "$BUILD_DIR"
CLI_OPT=OFF; GUI_OPT=OFF; [[ $BUILD_CLI -eq 0 ]] || CLI_OPT=ON; [[ $BUILD_GUI -eq 0 ]] || GUI_OPT=ON
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DSIPHER_BUILD_CLI="$CLI_OPT" -DSIPHER_BUILD_GUI="$GUI_OPT" -DSIPHER_BUILD_TESTS=OFF
cmake --build "$BUILD_DIR" --parallel
if [[ "$INSTALL_MODE" == yes ]]; then cmake --install "$BUILD_DIR"; fi
echo
echo "$PRODUCT $VERSION Termux build complete."
echo "Binary: $PREFIX/bin/$BINARY_NAME"
[[ $BUILD_CLI -eq 0 || $BUILD_GUI -eq 0 ]] || echo "Auto UI: terminal -> CLI; for Termux:X11 GUI use: sipher --gui"
echo "Note: packet capture and audio device availability depend on Android/Termux permissions and installed backends."
