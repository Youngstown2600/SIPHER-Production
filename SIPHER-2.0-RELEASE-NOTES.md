# SIPHER 2.0 — Multi-SIP / DID Intelligence / Unified Frontend

SIPHER 2.0 promotes the r19 Multi-SIP code line to a new major-version baseline and carries forward the r18 DID/route/audit fixes.

## Major changes

- One public executable name on every platform: `sipher` (`sipher.exe` on Windows).
- When both frontends are built, the same executable chooses CLI when launched from a real terminal/TTY and GUI when launched from a graphical desktop session without a TTY.
- Explicit overrides remain available: `sipher --cli`, `sipher --gui`, or `SIPHER_UI=cli|gui`.
- r19 multiple-SIP-account support is retained, including simultaneous registrations and zero-account startup.
- r18 DID Intelligence, carrier handoff/next-out analysis, Switch Audit+, and offline Legacy lab panels are retained.
- DID Intelligence no longer requires IPQualityScore: the default provider is USACallerLookup's free JSON API (no key/signup) for US carrier/line-type/location plus FTC/community complaint signals.
- Added **OPEN SPAMCALLS REPUTATION** to open the selected number's SpamCalls.net community page without scraping or depending on its HTML.
- Carrier results are explicitly labeled as NANPA registry assignment; live current-carrier/LRN data remains a future pluggable-provider path.

## r18 fixes carried forward

- Linux/PJSIP 2.17 RX peer capture now uses `pjsip_rx_data::pkt_info.src_name` / `src_port`; the invalid `pkt_info.addr` references are removed.
- The Qt `emit` macro collision in `MainWindow::analyzeNextOut()` is removed by using a non-reserved helper name.
- CTest registers current r18/r19/2.0 feature contracts instead of stale r15/r17 release-version/hash gates.

## Packaging

Linux, FreeBSD, macOS, Termux, and Windows builders all produce or install the single `sipher` executable name. macOS still packages it in `SIPHER.app`; Windows portable packages contain only `sipher.exe` for the application itself.
