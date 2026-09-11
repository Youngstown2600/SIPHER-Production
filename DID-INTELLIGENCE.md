# SIPHER 2.0 DID Intelligence

The DID Intelligence panel intentionally separates numbering/carrier data from community reputation and from live mobile-network HLR data.

## Normal DIP / LOOKUP DID

The normal lookup never uses a paid API credential automatically.

- **USACallerLookup** — US numbering-registry carrier/line type/location plus FTC/community complaint information.
- **SpamCalls.net** — community spam reputation, report count, recent activity and user-call signals parsed on a best-effort basis from the public number page.
- **tellows** — community score, reported call category and rating count parsed on a best-effort basis from the public number page.
- **c-qui.fr** — for French numbers only, original carrier allocation, request count, mnemonic/allocation information and number block where available.

Each provider fails independently. A layout change, block, timeout or unavailable record at one source does not discard results from the others.

## Enhanced HLR / Current Carrier

Neutrino HLR is deliberately separate because an HLR request can consume paid API credits. SIPHER never runs it as part of the normal DIP.

Set these environment variables before launching SIPHER:

```sh
export SIPHER_NEUTRINO_USER_ID='your-user-id'
export SIPHER_NEUTRINO_API_KEY='your-api-key'
```

Then use **ENHANCED HLR / CURRENT CARRIER**. SIPHER requests live HLR information such as origin network, current network, ported network/status, number type, HLR status and roaming status when the provider returns those fields.

## Interpretation

- Registry/original-carrier data may not represent the current serving carrier after number portability.
- Community spam reports are reputation signals, not proof that the subscriber placed a fraudulent call.
- Caller ID/ANI can be spoofed, causing an otherwise legitimate number to collect complaints.
- HTML-backed community sources are best-effort integrations and may need parser updates if the provider changes its public page layout.
