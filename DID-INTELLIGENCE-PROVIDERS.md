# SIPHER 2.0 DID Intelligence Providers

## Built-in default: USACallerLookup

SIPHER uses the public read-only JSON endpoint at `https://www.usacallerlookup.com/wp-json/ucl/v1/number/{phone}` for US 10-digit numbers. No API key or account is required. The UI presents carrier/line-type/location data when returned plus FTC complaint and community-report signals.

Carrier data is numbering-registry/NANPA assignment data. A ported number can be served by a different current carrier, so this is not a live LRN/MNP dip.

## Community reputation: SpamCalls.net

The **OPEN SPAMCALLS REPUTATION** button opens `https://spamcalls.net/en/num/{number}` in the user's default browser. SIPHER does not scrape, parse, or make runtime correctness depend on SpamCalls.net HTML.

## Interpretation

Complaint and community-report data is reputation evidence, not proof that the subscriber committed fraud. Caller ID/ANI can be spoofed. SIPHER therefore labels complaint volume as a warning signal and does not identify an owner or claim guilt.

## Future live-carrier provider

The UI and output distinguish registry carrier assignment from current serving-carrier/LRN information. A future authenticated LRN/MNP provider can be added without changing the free default lookup.
