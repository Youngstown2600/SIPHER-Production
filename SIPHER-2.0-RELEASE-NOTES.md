# SIPHER 2.0 — Multi-SIP / DID Intelligence / Unified Frontend

SIPHER 2.0 promotes the r19 Multi-SIP code line to a new major-version baseline and carries forward the r18 DID/route/audit fixes.

## Major changes

- One public executable name on every platform: `sipher` (`sipher.exe` on Windows).
- When both frontends are built, the same executable chooses CLI when launched from a real terminal/TTY and GUI when launched from a graphical desktop session without a TTY.
- Explicit overrides remain available: `sipher --cli`, `sipher --gui`, or `SIPHER_UI=cli|gui`.
- r19 multiple-SIP-account support is retained, including simultaneous registrations and zero-account startup.
- r18 DID Intelligence, carrier handoff/next-out analysis, Switch Audit+, and offline Legacy lab panels are retained.
- DID Intelligence no longer requires IPQualityScore. A normal **DIP / LOOKUP DID** aggregates USACallerLookup (US numbering/FTC data), SpamCalls.net reputation, tellows reputation, and c-qui.fr original-carrier allocation for French numbers.
- Added explicit **ENHANCED HLR / CURRENT CARRIER** lookup through Neutrino. It never runs automatically and only activates when `SIPHER_NEUTRINO_USER_ID` and `SIPHER_NEUTRINO_API_KEY` are supplied.
- SpamCalls/tellows/c-qui HTML sources are parsed on a best-effort basis and fail independently so one provider cannot break the entire DID result.
- Carrier results clearly distinguish registry/original allocation from live current-network data returned by an explicit HLR lookup.
- The remaining pre-SIPHER runtime/build namespace has been removed: Unix config is `~/.config/sipher`, state is `~/.local/state/sipher`, managed PJSIP is `~/.local/sipher-pjsip`, and installed data/docs are under `share/sipher` / `share/doc/sipher`.

## r18 fixes carried forward

- Linux/PJSIP 2.17 RX peer capture now uses `pjsip_rx_data::pkt_info.src_name` / `src_port`; the invalid `pkt_info.addr` references are removed.
- The Qt `emit` macro collision in `MainWindow::analyzeNextOut()` is removed by using a non-reserved helper name.
- CTest registers current r18/r19/2.0 feature contracts instead of stale r15/r17 release-version/hash gates.

## Packaging

Linux, FreeBSD, macOS, Termux, and Windows builders all produce or install the single `sipher` executable name. macOS still packages it in `SIPHER.app`; Windows portable packages contain only `sipher.exe` for the application itself.
