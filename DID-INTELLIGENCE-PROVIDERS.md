# SIPHER 2.0 DID Intelligence Providers

## Built-in default: USACallerLookup

SIPHER uses the public read-only JSON endpoint at `https://www.usacallerlookup.com/wp-json/ucl/v1/number/{phone}` for US 10-digit numbers. No API key or account is required. The UI presents carrier/line-type/location data when returned plus FTC complaint and community-report signals.

Carrier data is numbering-registry/NANPA assignment data. A ported number can be served by a different current carrier, so this is not a live LRN/MNP dip.

## Community reputation: SpamCalls.net

SpamCalls.net is queried and parsed inside SIPHER. When SpamCalls returns HTTP 410, SIPHER treats that as no live reputation record and shows nothing for that provider. Other failed providers are also hidden. tellows is queried using the national NANPA URL form (for example `/num/4155551212`) and is likewise displayed only when usable data is returned.

## Interpretation

Complaint and community-report data is reputation evidence, not proof that the subscriber committed fraud. Caller ID/ANI can be spoofed. SIPHER therefore labels complaint volume as a warning signal and does not identify an owner or claim guilt.

## Future live-carrier provider

The UI and output distinguish registry carrier assignment from current serving-carrier/LRN information. A future authenticated LRN/MNP provider can be added without changing the free default lookup.


## Provider fallback behavior (2.0 hotfix)

- HTTP failures are not printed as full provider sections. A provider only appears in the DID output when it returned usable data.
- SpamCalls HTTP 410 is interpreted as no live reputation page for that number and is silently skipped.
- tellows NANPA URLs use the 10-digit national number (for example `/num/4155551212`) rather than a literal `+1...` URL. A remaining 403 is treated as provider-side anti-automation blocking and SIPHER does not try to bypass it.
- USACallerLookup stays the free first pass. If it does not return carrier and line type, SIPHER can fall back to the Data247 Carrier247 API associated with FreeCarrierLookup.com. Configure `SIPHER_DATA247_API_KEY`; `SIPHER_DATA247_API_CODE` defaults to `C` and may be overridden for an account/service configuration.
- Carrier247 results can include carrier, mobile/landline/VoIP type, SMS gateway, MMS gateway, OCN and port-related fields when the account returns them.
- The public FreeCarrierLookup web form is not scraped automatically because it is protected by Cloudflare/CAPTCHA controls; the supported programmatic backend is used instead.
