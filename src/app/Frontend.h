#pragma once

// Front-end entry points used by the unified SIPHER 2.1 executable.
// The normal standalone wrappers in src/cli/main.cpp and src/gui/main.cpp
// remain available for CLI-only or GUI-only builds.
int sipherRunCli(int argc, char** argv);
int sipherRunGui(int argc, char** argv);
