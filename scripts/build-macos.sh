#!/bin/sh
set -eu
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT_DIR"
[ "$(uname -s)" = Darwin ] || { echo "build-macos.sh must run on macOS." >&2; exit 2; }

PRODUCT="SIPHER"
VERSION="1.0.0-r18"
SLUG="sipher"
CLI_NAME="sipher"
GUI_NAME="sipher-gui"
BUNDLE_ID="org.sipher.client"
APP_NAME="SIPHER.app"
BUILD_DIR=${BUILD_DIR:-$ROOT_DIR/build-macos}
PJSIP_PREFIX=${PJSIP_PREFIX:-${HOME:-$ROOT_DIR}/.local/trunkmonkey-pjsip}
INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}
APP_INSTALL_DIR=${APP_INSTALL_DIR:-/Applications}
BUILD_CLI=1
BUILD_GUI=1
CLEAN=0
FORCE_PJSIP=0
AUTO_DEPS=1
MAKE_DMG=1
INSTALL_MODE=ask
UNINSTALL=0
ASSUME_YES=0
BOOTSTRAP_ONLY=0
JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)
BUILD_TYPE=Release

usage() {
cat <<USAGE
$PRODUCT $VERSION — macOS builder
Usage: ./build.sh --os macos [--cli|--gui|--all] [options]

  --cli            build CLI only
  --gui            build Qt GUI only
  --all            build CLI + GUI (default)
  --clean          remove macOS build directory first
  --pjsip          force rebuild managed PJSIP 2.17
  --dmg/--no-dmg   enable/disable DMG creation (default: DMG on when GUI is built)
  --auto-deps      install/bootstrap required dependencies (default)
  --no-auto-deps   audit only; do not install dependencies
  --bootstrap-only prepare Xcode/Homebrew/dependencies then exit
  --install        install without final prompt
  --no-install     build only
  --uninstall      remove installed app/launchers; preserve user data
  --prefix PATH    command launcher prefix (default /usr/local)
  --pjsip-prefix P managed PJSIP prefix
  --jobs N         parallel jobs
  --yes, -y        answer yes to builder-owned prompts
USAGE
}

explicit_target=0
while [ "$#" -gt 0 ]; do
  case "$1" in
    --cli) [ "$explicit_target" -eq 1 ] || { BUILD_CLI=0; BUILD_GUI=0; explicit_target=1; }; BUILD_CLI=1 ;;
    --gui) [ "$explicit_target" -eq 1 ] || { BUILD_CLI=0; BUILD_GUI=0; explicit_target=1; }; BUILD_GUI=1 ;;
    --all) BUILD_CLI=1; BUILD_GUI=1; explicit_target=1 ;;
    --clean) CLEAN=1 ;;
    --pjsip) FORCE_PJSIP=1 ;;
    --dmg) MAKE_DMG=1 ;;
    --no-dmg) MAKE_DMG=0 ;;
    --auto-deps) AUTO_DEPS=1 ;;
    --no-auto-deps) AUTO_DEPS=0 ;;
    --bootstrap-only) BOOTSTRAP_ONLY=1; INSTALL_MODE=no ;;
    --install) INSTALL_MODE=yes ;;
    --no-install) INSTALL_MODE=no ;;
    --uninstall|--remove-only) UNINSTALL=1; INSTALL_MODE=no ;;
    --yes|-y) ASSUME_YES=1 ;;
    --prefix) shift; [ "$#" -gt 0 ] || { echo "--prefix requires a path" >&2; exit 2; }; INSTALL_PREFIX=$1 ;;
    --pjsip-prefix) shift; [ "$#" -gt 0 ] || { echo "--pjsip-prefix requires a path" >&2; exit 2; }; PJSIP_PREFIX=$1 ;;
    --jobs) shift; [ "$#" -gt 0 ] || { echo "--jobs requires a value" >&2; exit 2; }; JOBS=$1 ;;
    --build-type) shift; [ "$#" -gt 0 ] || { echo "--build-type requires a value" >&2; exit 2; }; BUILD_TYPE=$1 ;;
    --deps) BOOTSTRAP_ONLY=1; INSTALL_MODE=no ;;
    --dry-run) echo "macOS: use --bootstrap-only --no-auto-deps for a dependency-only audit."; exit 0 ;;
    --purge-user-data) echo "--purge-user-data is intentionally not automated on macOS; application uninstall preserves user data." >&2 ;;
    --configure-capture|--audio-diagnose|--no-audio-fix) echo "$1 is Linux/FreeBSD-specific and is not used on macOS." >&2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown macOS builder option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

case "$JOBS" in ''|*[!0-9]*|0) echo "--jobs requires a positive integer" >&2; exit 2 ;; esac
INSTALL_BIN="$INSTALL_PREFIX/bin/$SLUG"
INSTALL_GUI_BIN="$INSTALL_PREFIX/bin/$SLUG-gui"
INSTALL_APP="$APP_INSTALL_DIR/$APP_NAME"
DMG_PATH="$BUILD_DIR/$PRODUCT-$VERSION-macOS.dmg"

say(){ printf '\n==> %s\n' "$*"; }
fail(){ echo "ERROR: $*" >&2; exit 1; }
run_admin(){ if [ "$(id -u)" -eq 0 ]; then "$@"; else sudo "$@"; fi; }
ask_yes_no(){
  prompt=$1; default=${2:-no}
  [ "$ASSUME_YES" -eq 1 ] && return 0
  [ -t 0 ] || { [ "$default" = yes ]; return; }
  if [ "$default" = yes ]; then printf '%s [Y/n]: ' "$prompt"; else printf '%s [y/N]: ' "$prompt"; fi
  IFS= read -r ans
  case "$ans" in y|Y|yes|YES|Yes) return 0;; n|N|no|NO|No) return 1;; '') [ "$default" = yes ];; *) return 1;; esac
}

if [ "$UNINSTALL" -eq 1 ]; then
  if [ "$ASSUME_YES" -ne 1 ] && [ -t 0 ]; then ask_yes_no "Remove installed $PRODUCT application files and preserve user configuration?" no || { echo "Uninstall cancelled."; exit 0; }; fi
  say "Removing installed $PRODUCT"
  [ -e "$INSTALL_APP" ] && run_admin rm -rf "$INSTALL_APP" || true
  [ -e "$INSTALL_BIN" ] || [ -L "$INSTALL_BIN" ] && run_admin rm -f "$INSTALL_BIN" || true
  [ -e "$INSTALL_GUI_BIN" ] || [ -L "$INSTALL_GUI_BIN" ] && run_admin rm -f "$INSTALL_GUI_BIN" || true
  echo "$PRODUCT application files removed. User configuration was preserved."
  exit 0
fi

MACOS_VERSION=$(sw_vers -productVersion 2>/dev/null || echo unknown)
MACOS_MAJOR=$(printf '%s' "$MACOS_VERSION" | awk -F. '{print $1}')
MACOS_MINOR=$(printf '%s' "$MACOS_VERSION" | awk -F. '{print $2+0}')
case "$MACOS_MAJOR" in ''|*[!0-9]*) fail "Could not determine macOS version ($MACOS_VERSION)";; 0|1|2|3|4|5|6|7|8|9|10|11|12) fail "$PRODUCT $VERSION macOS bundle targets macOS 13 or newer; detected $MACOS_VERSION.";; esac
export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-13.0}

# Ventura cannot use today's newest Xcode. Match the WaffleHouse 5.3 builder:
# choose Apple's last compatible archived Xcode and let Safari handle Apple sign-in.
XCODE_RECOMMENDED=
XCODE_DOWNLOAD_URL=https://developer.apple.com/download/all/
if [ "$MACOS_MAJOR" -eq 13 ]; then
  if [ "$MACOS_MINOR" -ge 5 ]; then
    XCODE_RECOMMENDED=15.2
    XCODE_DOWNLOAD_URL='https://developer.apple.com/services-account/download?path=/Developer_Tools/Xcode_15.2/Xcode_15.2.xip'
  else
    XCODE_RECOMMENDED=14.3.1
    XCODE_DOWNLOAD_URL='https://developer.apple.com/services-account/download?path=/Developer_Tools/Xcode_14.3.1/Xcode_14.3.1.xip'
  fi
fi

xcode_usable(){ app=$1; [ -x "$app/Contents/Developer/usr/bin/xcodebuild" ] && DEVELOPER_DIR="$app/Contents/Developer" "$app/Contents/Developer/usr/bin/xcodebuild" -version >/dev/null 2>&1; }
find_downloaded_xcode_app(){
  for app in "$HOME"/Downloads/Xcode.app "$HOME"/Downloads/Xcode*.app; do [ -e "$app" ] || continue; xcode_usable "$app" && { printf '%s\n' "$app"; return 0; }; done
  return 1
}
find_downloaded_xip(){
  if [ -n "$XCODE_RECOMMENDED" ]; then
    candidate="$HOME/Downloads/Xcode_${XCODE_RECOMMENDED}.xip"
    [ -f "$candidate" ] && { printf '%s\n' "$candidate"; return 0; }
  fi
  for xip in "$HOME"/Downloads/Xcode*.xip; do [ -f "$xip" ] || continue; printf '%s\n' "$xip"; return 0; done
  return 1
}
configure_xcode(){
  app=$1
  if [ "$app" != /Applications/Xcode.app ]; then
    say "Moving full Xcode into /Applications"
    [ -e /Applications/Xcode.app ] && run_admin mv /Applications/Xcode.app "/Applications/Xcode.backup.$(date +%Y%m%d%H%M%S).app"
    run_admin mv "$app" /Applications/Xcode.app; app=/Applications/Xcode.app
  fi
  say "Configuring Xcode"
  run_admin xcode-select -s "$app/Contents/Developer"
  run_admin xcodebuild -license accept
  run_admin xcodebuild -runFirstLaunch
}
ensure_xcode(){
  if xcode_usable /Applications/Xcode.app; then configure_xcode /Applications/Xcode.app; return; fi
  app=$(find_downloaded_xcode_app 2>/dev/null || true); [ -z "$app" ] || { configure_xcode "$app"; return; }
  [ "$AUTO_DEPS" -eq 1 ] || fail "Full Xcode is missing. Install a compatible Xcode.app and rerun."
  echo "Full Xcode was not found."
  if [ -n "$XCODE_RECOMMENDED" ]; then
    echo "Detected macOS $MACOS_VERSION. Recommended archived Xcode: $XCODE_RECOMMENDED"
  else
    echo "Detected macOS $MACOS_VERSION. Opening Apple's official Xcode downloads page."
  fi
  echo "Apple may require browser sign-in; this builder never requests or stores Apple credentials."
  open "$XCODE_DOWNLOAD_URL" >/dev/null 2>&1 || true
  [ -t 0 ] || fail "Complete the Xcode download in Safari, then rerun the builder."
  printf 'When Xcode finishes downloading, press Enter to continue: '; IFS= read -r _
  app=$(find_downloaded_xcode_app 2>/dev/null || true)
  if [ -n "$app" ]; then configure_xcode "$app"; return; fi
  xip=$(find_downloaded_xip 2>/dev/null || true); [ -n "$xip" ] || fail "No Xcode.app or Xcode .xip found in ~/Downloads."
  say "Extracting Xcode archive"
  (cd "$(dirname "$xip")" && /usr/bin/xip -x "$(basename "$xip")")
  app=$(find_downloaded_xcode_app 2>/dev/null || true); [ -n "$app" ] || fail "Xcode extraction finished but Xcode.app was not found."
  configure_xcode "$app"
}
ensure_homebrew(){
  if ! command -v brew >/dev/null 2>&1; then
    for b in /opt/homebrew/bin/brew /usr/local/bin/brew; do [ -x "$b" ] || continue; eval "$("$b" shellenv)"; done
  fi
  if ! command -v brew >/dev/null 2>&1; then
    [ "$AUTO_DEPS" -eq 1 ] || fail "Homebrew is missing and --no-auto-deps was supplied."
    say "Installing Homebrew"
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    for b in /opt/homebrew/bin/brew /usr/local/bin/brew; do [ -x "$b" ] || continue; eval "$("$b" shellenv)"; done
  fi
  command -v brew >/dev/null 2>&1 || fail "Homebrew installation finished but brew is not on PATH."
}
ensure_formula(){ f=$1; brew list --versions "$f" >/dev/null 2>&1 && return 0; [ "$AUTO_DEPS" -eq 1 ] || fail "Missing Homebrew formula: $f"; brew install "$f"; }

say "macOS development environment preflight"
ensure_xcode
ensure_homebrew
for f in cmake ninja pkg-config git openssl@3 qtbase qttools portaudio opus; do ensure_formula "$f"; done
# Runtime/diagnostic helpers are useful but not compile blockers.
for f in ffmpeg wireshark; do if ! brew list --versions "$f" >/dev/null 2>&1 && [ "$AUTO_DEPS" -eq 1 ]; then brew install "$f" || echo "WARNING: optional formula '$f' could not be installed." >&2; fi; done

if [ "$BOOTSTRAP_ONLY" -eq 1 ]; then echo "$PRODUCT macOS prerequisites are ready."; exit 0; fi

BREW_PREFIX=$(brew --prefix)
QT_PREFIX=$(brew --prefix qtbase)
QTTOOLS_PREFIX=$(brew --prefix qttools)
OPENSSL_PREFIX=$(brew --prefix openssl@3)
PORTAUDIO_PREFIX=$(brew --prefix portaudio)
OPUS_PREFIX=$(brew --prefix opus)
export PATH="$QT_PREFIX/bin:$QTTOOLS_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="$PJSIP_PREFIX/lib/pkgconfig:$OPENSSL_PREFIX/lib/pkgconfig:$PORTAUDIO_PREFIX/lib/pkgconfig:$OPUS_PREFIX/lib/pkgconfig:$BREW_PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export CMAKE_PREFIX_PATH="$QT_PREFIX:$QTTOOLS_PREFIX:$BREW_PREFIX:${CMAKE_PREFIX_PATH:-}"
export CPPFLAGS="-I$OPENSSL_PREFIX/include -I$PORTAUDIO_PREFIX/include -I$OPUS_PREFIX/include ${CPPFLAGS:-}"
export LDFLAGS="-L$OPENSSL_PREFIX/lib -L$PORTAUDIO_PREFIX/lib -L$OPUS_PREFIX/lib ${LDFLAGS:-}"

if [ "$FORCE_PJSIP" -eq 1 ]; then rm -f "$PJSIP_PREFIX/.trunkmonkey-pjsip-build"; fi
if [ ! -f "$PJSIP_PREFIX/.trunkmonkey-pjsip-build" ] || ! PKG_CONFIG_PATH="$PJSIP_PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}" pkg-config --exists 'libpjproject = 2.17' 2>/dev/null; then
  say "Building managed PJSIP 2.17"
  PJSIP_PREFIX="$PJSIP_PREFIX" CC=clang CXX=clang++ "$ROOT_DIR/scripts/bootstrap-pjsip.sh"
fi
export PKG_CONFIG_PATH="$PJSIP_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"

[ "$CLEAN" -eq 0 ] || rm -rf "$BUILD_DIR"
CLI_OPT=OFF; GUI_OPT=OFF; [ "$BUILD_CLI" -eq 0 ] || CLI_OPT=ON; [ "$BUILD_GUI" -eq 0 ] || GUI_OPT=ON
say "Configuring $PRODUCT $VERSION"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET" \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" -DTRUNKMONKEY_BUILD_CLI="$CLI_OPT" -DTRUNKMONKEY_BUILD_GUI="$GUI_OPT" -DTRUNKMONKEY_BUILD_TESTS=OFF
cmake --build "$BUILD_DIR" --parallel "$JOBS"

APP=
if [ "$BUILD_GUI" -eq 1 ]; then
  built_app="$BUILD_DIR/$GUI_NAME.app"
  [ -d "$built_app" ] || fail "Expected macOS app bundle was not built: $built_app"
  APP="$BUILD_DIR/$APP_NAME"
  [ "$built_app" = "$APP" ] || { rm -rf "$APP"; ditto "$built_app" "$APP"; }
  mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
  if [ "$BUILD_CLI" -eq 1 ] && [ -x "$BUILD_DIR/$CLI_NAME" ]; then cp -f "$BUILD_DIR/$CLI_NAME" "$APP/Contents/MacOS/$CLI_NAME"; chmod +x "$APP/Contents/MacOS/$CLI_NAME"; fi
  MACDEPLOYQT="$QTTOOLS_PREFIX/bin/macdeployqt"
  [ -x "$MACDEPLOYQT" ] || MACDEPLOYQT=$(find "$QTTOOLS_PREFIX" -type f -name macdeployqt -perm -111 -print -quit 2>/dev/null || true)
  [ -n "$MACDEPLOYQT" ] && [ -x "$MACDEPLOYQT" ] || fail "macdeployqt not found under $QTTOOLS_PREFIX"

  # Seed Cocoa before deployment. Split Homebrew Qt kegs can otherwise leave an
  # app that builds successfully but cannot start away from the build machine.
  QT_PLUGIN_DIR=
  for qtpaths in "$QT_PREFIX/bin/qtpaths" "$QT_PREFIX/bin/qtpaths6" "$QTTOOLS_PREFIX/bin/qtpaths"; do
    if [ -x "$qtpaths" ]; then
      candidate=$($qtpaths --plugin-dir 2>/dev/null || true)
      if [ -n "$candidate" ] && [ -d "$candidate" ]; then QT_PLUGIN_DIR=$candidate; break; fi
    fi
  done
  if [ -z "$QT_PLUGIN_DIR" ]; then
    cocoa=$(find "$QT_PREFIX" -type f -path '*/platforms/libqcocoa.dylib' -print -quit 2>/dev/null || true)
    [ -n "$cocoa" ] && QT_PLUGIN_DIR=$(dirname "$(dirname "$cocoa")")
  fi
  [ -n "$QT_PLUGIN_DIR" ] || fail "Could not locate the Qt plugin directory under $QT_PREFIX"
  COCOA_PLUGIN="$QT_PLUGIN_DIR/platforms/libqcocoa.dylib"
  [ -f "$COCOA_PLUGIN" ] || fail "Qt Cocoa platform plugin is missing: $COCOA_PLUGIN"
  mkdir -p "$APP/Contents/PlugIns/platforms"
  cp -f "$COCOA_PLUGIN" "$APP/Contents/PlugIns/platforms/libqcocoa.dylib"

  say "Deploying Qt/runtime frameworks"
  set -- "$APP" -verbose=2 -always-overwrite -no-codesign
  DEP_FORMULAS="qtbase qttools openssl@3 portaudio opus"
  for root_formula in qtbase qttools; do
    deps=$(brew deps --installed --formula "$root_formula" 2>/dev/null || true)
    DEP_FORMULAS="$DEP_FORMULAS $deps"
  done
  installed_qt=$(brew list --formula 2>/dev/null | awk '/^qt/ {print}' || true)
  DEP_FORMULAS="$DEP_FORMULAS $installed_qt"
  seen_libpaths=
  for formula in $DEP_FORMULAS; do
    prefix=$(brew --prefix "$formula" 2>/dev/null || true)
    [ -n "$prefix" ] && [ -d "$prefix/lib" ] || continue
    case " $seen_libpaths " in *" $prefix/lib "*) continue ;; esac
    seen_libpaths="$seen_libpaths $prefix/lib"
    set -- "$@" "-libpath=$prefix/lib"
  done
  DEPLOY_LOG="$BUILD_DIR/macdeployqt.log"
  if "$MACDEPLOYQT" "$@" >"$DEPLOY_LOG" 2>&1; then cat "$DEPLOY_LOG"; else cat "$DEPLOY_LOG" >&2; fail "macdeployqt returned a failure status. See $DEPLOY_LOG"; fi
  if grep -E '(^|[[:space:]])ERROR:|Cannot resolve (rpath|dependency|library)' "$DEPLOY_LOG" >/dev/null 2>&1; then
    grep -E '(^|[[:space:]])ERROR:|Cannot resolve (rpath|dependency|library)' "$DEPLOY_LOG" >&2 || true
    fail "Refusing to package an app with unresolved macdeployqt errors."
  fi
  mkdir -p "$APP/Contents/Resources" "$APP/Contents/PlugIns/platforms"
  cat > "$APP/Contents/Resources/qt.conf" <<'QTCONF'
[Paths]
Plugins = PlugIns
QTCONF
  if [ ! -f "$APP/Contents/PlugIns/platforms/libqcocoa.dylib" ]; then
    cp -f "$COCOA_PLUGIN" "$APP/Contents/PlugIns/platforms/libqcocoa.dylib"
  fi
  [ -f "$APP/Contents/PlugIns/platforms/libqcocoa.dylib" ] || fail "Bundle verification failed: Cocoa plugin is missing."
  [ -d "$APP/Contents/Frameworks" ] || fail "Bundle verification failed: Contents/Frameworks is missing."
  [ -x "$APP/Contents/MacOS/$GUI_NAME" ] || fail "Bundle verification failed: GUI executable is missing."
  codesign --force --deep --sign - "$APP"
  codesign --verify --deep --strict "$APP"
  if [ "$MAKE_DMG" -eq 1 ]; then
    say "Creating DMG"
    stage="$BUILD_DIR/dmg-stage"; rm -rf "$stage" "$DMG_PATH"; mkdir -p "$stage"
    ditto "$APP" "$stage/$APP_NAME"; ln -s /Applications "$stage/Applications"
    hdiutil create -volname "$PRODUCT $VERSION" -srcfolder "$stage" -ov -format UDZO "$DMG_PATH"
    rm -rf "$stage"
  fi
fi

if [ "$INSTALL_MODE" = ask ]; then
  if [ -t 0 ]; then ask_yes_no "Install $PRODUCT now?" no && INSTALL_MODE=yes || INSTALL_MODE=no; else INSTALL_MODE=no; fi
fi
if [ "$INSTALL_MODE" = yes ]; then
  run_admin mkdir -p "$INSTALL_PREFIX/bin" "$APP_INSTALL_DIR"
  if [ "$BUILD_GUI" -eq 1 ]; then run_admin rm -rf "$INSTALL_APP"; run_admin ditto "$APP" "$INSTALL_APP"; run_admin ln -sf "$INSTALL_APP/Contents/MacOS/$GUI_NAME" "$INSTALL_GUI_BIN"; fi
  if [ "$BUILD_CLI" -eq 1 ]; then
    if [ "$BUILD_GUI" -eq 1 ] && [ -x "$INSTALL_APP/Contents/MacOS/$CLI_NAME" ]; then run_admin ln -sf "$INSTALL_APP/Contents/MacOS/$CLI_NAME" "$INSTALL_BIN"; else run_admin cp -f "$BUILD_DIR/$CLI_NAME" "$INSTALL_BIN"; fi
  fi
fi

echo
echo "$PRODUCT $VERSION macOS build complete."
[ "$BUILD_GUI" -eq 0 ] || echo "App: $APP"
[ "$BUILD_CLI" -eq 0 ] || echo "CLI: $BUILD_DIR/$CLI_NAME"
[ "$BUILD_GUI" -eq 0 ] || [ "$MAKE_DMG" -eq 0 ] || echo "DMG: $DMG_PATH"
