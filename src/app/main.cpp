#include "Frontend.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>
#else
#include <unistd.h>
#endif

namespace {
enum class UiMode { Auto, Cli, Gui };

UiMode envMode()
{
    const char* value = std::getenv("SIPHER_UI");
    if (!value || !*value) return UiMode::Auto;
    std::string v(value);
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (v == "cli" || v == "tty" || v == "terminal") return UiMode::Cli;
    if (v == "gui" || v == "desktop") return UiMode::Gui;
    return UiMode::Auto;
}

bool guiSessionAvailable()
{
#ifdef _WIN32
    return true;
#elif defined(__APPLE__)
    return true;
#else
    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    const char* mir = std::getenv("MIR_SOCKET");
    return (display && *display) || (wayland && *wayland) || (mir && *mir);
#endif
}

#ifdef _WIN32
bool attachParentConsole()
{
    if (GetConsoleWindow() != nullptr) return true;
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) return false;
    FILE* stream = nullptr;
    (void)freopen_s(&stream, "CONIN$", "r", stdin);
    (void)freopen_s(&stream, "CONOUT$", "w", stdout);
    (void)freopen_s(&stream, "CONOUT$", "w", stderr);
    std::ios::sync_with_stdio(true);
    return true;
}

void ensureConsoleForExplicitCli()
{
    if (attachParentConsole()) return;
    if (AllocConsole()) {
        FILE* stream = nullptr;
        (void)freopen_s(&stream, "CONIN$", "r", stdin);
        (void)freopen_s(&stream, "CONOUT$", "w", stdout);
        (void)freopen_s(&stream, "CONOUT$", "w", stderr);
        std::ios::sync_with_stdio(true);
    }
}
#else
bool terminalAttached()
{
    return ::isatty(STDIN_FILENO) || ::isatty(STDOUT_FILENO) || ::isatty(STDERR_FILENO);
}
#endif

struct FilteredArgs {
    std::vector<std::string> storage;
    std::vector<char*> argv;
};

FilteredArgs filterUiFlags(int argc, char** argv, UiMode& requested)
{
    FilteredArgs out;
    out.storage.reserve(static_cast<std::size_t>(argc));
    if (argc > 0 && argv && argv[0]) out.storage.emplace_back(argv[0]);
    else out.storage.emplace_back("sipher");

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] ? argv[i] : "";
        if (arg == "--cli") { requested = UiMode::Cli; continue; }
        if (arg == "--gui") { requested = UiMode::Gui; continue; }
        if (arg == "--ui" && i + 1 < argc) {
            const std::string value = argv[++i] ? argv[i] : "";
            if (value == "cli") requested = UiMode::Cli;
            else if (value == "gui") requested = UiMode::Gui;
            else if (value == "auto") requested = UiMode::Auto;
            else {
                std::cerr << "SIPHER: --ui expects auto, cli, or gui.\n";
                requested = UiMode::Cli;
            }
            continue;
        }
        out.storage.push_back(arg);
    }

    out.argv.reserve(out.storage.size() + 1);
    for (auto& value : out.storage) out.argv.push_back(value.data());
    out.argv.push_back(nullptr);
    return out;
}
}

int main(int argc, char** argv)
{
    UiMode requested = envMode();
    auto args = filterUiFlags(argc, argv, requested);
    const int filteredArgc = static_cast<int>(args.storage.size());

#ifdef _WIN32
    if (requested == UiMode::Cli) {
        ensureConsoleForExplicitCli();
        return sipherRunCli(filteredArgc, args.argv.data());
    }
    if (requested == UiMode::Gui) {
        return sipherRunGui(filteredArgc, args.argv.data());
    }
    // Unified Windows build uses the GUI subsystem. If a parent console exists,
    // the process was launched from a terminal/PowerShell session; otherwise it
    // behaves like a desktop launch.
    if (attachParentConsole()) return sipherRunCli(filteredArgc, args.argv.data());
    return sipherRunGui(filteredArgc, args.argv.data());
#else
    if (requested == UiMode::Cli) return sipherRunCli(filteredArgc, args.argv.data());
    if (requested == UiMode::Gui) return sipherRunGui(filteredArgc, args.argv.data());

    // A real TTY wins even inside X11/Wayland. This makes `sipher` from an
    // XFCE/KDE/GNOME terminal open the CLI, while menu/icon launches (no TTY)
    // open the GUI. Headless/no-display launches fall back to CLI.
    if (terminalAttached()) return sipherRunCli(filteredArgc, args.argv.data());
    if (guiSessionAvailable()) return sipherRunGui(filteredArgc, args.argv.data());
    return sipherRunCli(filteredArgc, args.argv.data());
#endif
}
