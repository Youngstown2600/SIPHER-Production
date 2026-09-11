# S.I.P.H.E.R. 1.0.0-r18-DID-Route-Audit

## Linux/PJSIP 2.17 receive-peer hotfix — 2026-09-10

- Fixed the r18 SIP wire monitor build against the required PJSIP 2.17 API. Incoming request/response peer capture now uses `pjsip_rx_data::pkt_info.src_name` and `src_port`; the invalid `pkt_info.addr` reference has been removed.
- Outbound next-hop capture remains unchanged and continues to use `pjsip_tx_data::tp_info.dst_name` / `dst_port`.
- Added a source regression guard that fails if the invalid `pkt_info.addr` spelling is reintroduced.

## New

- **DID Intelligence**: enter a DID/telephone number and retrieve carrier/line-type/validity plus spam, recent-abuse, risk and fraud-score indicators from IPQualityScore when an API key is configured.
- **Carrier Handoff / Next-Out**: shows the expected signaling peer, DNS/SRV candidates, normalized Request-URI and observed Route/Record-Route/Via/Contact headers. When an outbound INVITE is captured, S.I.P.H.E.R. also displays the actual resolved peer address and port supplied by PJSIP.
- **Switch Audit+**: adds UDP/TCP parity and topology/information-exposure checks.
- **Legacy** menu: offline Blue Tone / Blue Box and Red Box historical lab panels. They are intentionally non-transmitting simulations and do not generate live carrier-control or coin-control tones.

## DID provider configuration

Set `SIPHER_IPQS_API_KEY` before launch or paste the key into the DID Intelligence tab. The application sends the key in the `IPQS-KEY` HTTP header rather than embedding it in the request URL.

## Route-analysis boundary

The next-out analyzer can prove the S.I.P.H.E.R. signaling handoff and expose additional SIP nodes only when the network discloses them. Carrier SBC topology hiding may intentionally remove or rewrite downstream Via, Record-Route and Contact information. An IP traceroute to an SBC is not treated as the SIP/PSTN call route.

## Build dependency change

The GUI now links Qt Network in addition to Qt Core and Qt Widgets.
