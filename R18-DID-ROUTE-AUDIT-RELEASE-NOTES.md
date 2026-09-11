# S.I.P.H.E.R. 1.0.0-r18-DID-Route-Audit

## New

- **DID Intelligence (r18 original)**: introduced carrier/line-type and reputation lookup. **SIPHER 2.0 supersedes the original IPQualityScore integration with USACallerLookup (no API key) plus a SpamCalls.net browser action.**
- **Carrier Handoff / Next-Out**: shows the expected signaling peer, DNS/SRV candidates, normalized Request-URI and observed Route/Record-Route/Via/Contact headers. When an outbound INVITE is captured, S.I.P.H.E.R. also displays the actual resolved peer address and port supplied by PJSIP.
- **Switch Audit+**: adds UDP/TCP parity and topology/information-exposure checks.
- **Legacy** menu: offline Blue Tone / Blue Box and Red Box historical lab panels. They are intentionally non-transmitting simulations and do not generate live carrier-control or coin-control tones.

## DID provider configuration

SIPHER 2.0 requires no reputation API key for the built-in US lookup. USACallerLookup provides NANPA registry carrier/line-type/location data plus FTC/community complaint signals; SpamCalls.net is opened separately in the system browser and is not scraped.

## Route-analysis boundary

The next-out analyzer can prove the S.I.P.H.E.R. signaling handoff and expose additional SIP nodes only when the network discloses them. Carrier SBC topology hiding may intentionally remove or rewrite downstream Via, Record-Route and Contact information. An IP traceroute to an SBC is not treated as the SIP/PSTN call route.

## Build dependency change

The GUI now links Qt Network in addition to Qt Core and Qt Widgets.
