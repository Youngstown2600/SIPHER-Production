# SIPHER 2.1 — DID Intelligence Waterfall / Legacy Audio Demo

Release date: 2026-09-11

SIPHER 2.1 is a focused follow-up to 2.0. It keeps the unified `sipher` executable, r18/r19 fixes, multi-account SIP, zero-account startup, carrier handoff analysis, Switch Audit+, and the cleaned SIPHER runtime/install namespace.

## DID Intelligence 2.1

The normal **DIP / LOOKUP DID** path is now a provider waterfall instead of a fire-everything-at-once lookup. Failed or blocked providers do not get full output sections.

For US numbers the free/no-key path starts with:

- **USACallerLookup** for NANPA carrier/line-type/rate-center information plus FTC/community complaint data.
- **SkipCalls** for a simple JSON spam reputation verdict with no API key.

If the free carrier lookup does not provide carrier/type fields, SIPHER tries configured carrier fallbacks in this order:

1. **FreeCarrierLookup / Carrier247 (Data247)** using `SIPHER_DATA247_API_KEY`.
2. **Veriphone** using `SIPHER_VERIPHONE_API_KEY` in `static` mode so the normal DIP does not silently spend Current Carrier credits.
3. **Omkar Phone Lookup API** using `SIPHER_OMKAR_API_KEY`.

If SkipCalls cannot answer, SIPHER falls back to the existing **SpamCalls.net** parser and then **tellows**. HTTP 410/403 and other unavailable-provider conditions are compacted into status notes instead of taking over the results pane.

The public FreeCarrierLookup.com form is protected by anti-automation controls, so SIPHER does not attempt to bypass them. The supported Carrier247/Data247 programmatic path is used when configured. Data247 results can include mobile/landline/VoIP classification, OCN/MNO/port information, and SMS/MMS gateway addresses when the service returns those fields.

The existing explicit **ENHANCED HLR / CURRENT CARRIER** Neutrino button remains separate so a normal DIP cannot unexpectedly consume HLR credits.

## Legacy audio demo

The Legacy Blue Box and Red Box panels now produce audible **local-only demonstration tones** through the workstation speaker. The generated sequences are deliberately non-signaling/detuned demonstrations and are never bridged into SIP, RTP, DTMF, or an active call.

Playback uses a generated temporary WAV and the platform's normal audio player (`pw-play`/`paplay`/`aplay`/`ffplay` on Unix-like systems, `afplay` on macOS, or `System.Media.SoundPlayer` on Windows), with the application beep as a last-resort fallback.

## Build/version

- Product version: **2.1**
- CMake project version: **2.1.0**
- SIP User-Agent: **SIPHER/2.1**
- Public executable: **`sipher`**
- Runtime config: `~/.config/sipher`
- Runtime state: `~/.local/state/sipher`
- Installed data/docs: `share/sipher` and `share/doc/sipher`
