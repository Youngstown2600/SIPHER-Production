# S.I.P.H.E.R. r16 modernization contract

S.I.P.H.E.R. r16 is a presentation-layer modernization of the complete r15 Full VoIP PCAP release.

## GUI

The Qt front end now uses a persistent navigation rail, contextual page titles, a compact registration surface, modern cards, rounded controls, cleaner tables, and shared theme styling. Existing dialogs inherit the same application stylesheet automatically. The supplied blue S.I.P.H.E.R. logo is embedded as a Qt resource and the GUI/CLI now share 36 themes.

Navigation preserves the existing functional pages:

- Phone & Capture
- Active Calls
- SIP Inspector
- Queue Test
- PBX Audit
- Profile
- Activity

## CLI

The ANSI dashboard keeps the same nine-page operator model, keyboard navigation, guided workflows, responsive layouts, and themes while changing presentation to rounded Unicode cards, the CONTROL DECK page rail, and the `❯` input prompt. The exact 99-column S.I.P.H.E.R. banner is shown only when the terminal is wide enough; narrower terminals use a compact text brand instead of cropping the logo.

## Preservation gate

`tests/core_r15_preservation_test.sh` compares SHA-256 hashes for `src/core`, behavior-bearing public headers, and compatibility scripts against the r15 baseline. `Version.h` is intentionally excluded because r16 updates release branding.

`tests/r16_modern_ui_source_test.sh` locks the new GUI/CLI presentation contract.
