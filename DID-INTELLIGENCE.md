# SIPHER 2.1 DID Intelligence

SIPHER's DID Intelligence panel separates three different questions that are often incorrectly treated as one lookup: **numbering/carrier data**, **current serving carrier/HLR data**, and **spam/reputation data**.

## Normal DIP / LOOKUP DID

For a US 10-digit number, SIPHER starts two independent no-key lookups:

1. **USACallerLookup** — numbering-registry carrier, line type, rate center/location, and FTC/community complaint information.
2. **SkipCalls** — a no-auth JSON spam-label signal.

The UI always shows the normalized E.164 number first. Only providers that return useful data get full result sections. If all live sources fail, SIPHER shows one compact provider-status line instead of a screen full of HTTP errors.

### Carrier fallback waterfall

If USACallerLookup does not return usable carrier/type fields, SIPHER tries configured providers in order:

- **Carrier247/Data247** — set `SIPHER_DATA247_API_KEY`; optional `SIPHER_DATA247_API_CODE` defaults to `C`. This is the supported programmatic backend associated with FreeCarrierLookup.com and can return carrier/type plus SMS/MMS gateway fields and optional OCN/MNO/port metadata depending on the account/service response.
- **Veriphone** — set `SIPHER_VERIPHONE_API_KEY`. SIPHER uses `/v3/verify` with `mode=static` in the normal DIP to avoid silently spending higher-cost Current Carrier credits.
- **Omkar Phone Lookup API** — set `SIPHER_OMKAR_API_KEY`. SIPHER uses the documented `/lookup` JSON endpoint.

The public FreeCarrierLookup.com browser form is intentionally **not** automated around its Cloudflare/CAPTCHA protections. SIPHER does not attempt to bypass provider anti-bot controls.

### Reputation fallback waterfall

For US numbers SIPHER asks SkipCalls first. If it cannot provide a verdict, SIPHER tries the existing internal SpamCalls.net parser and then tellows. Provider failures such as SpamCalls HTTP 410 or tellows HTTP 403 are treated as unavailable-source conditions and are not expanded into large error blocks.

A `Reported spam: NO` result means only that the queried reputation source does not currently label the number as spam. Caller ID can be spoofed, and an innocent subscriber can accumulate complaints.

## Explicit current-carrier / HLR lookup

The **ENHANCED HLR / CURRENT CARRIER** button remains explicit and uses Neutrino only when both environment variables are configured:

```sh
export SIPHER_NEUTRINO_USER_ID='...'
export SIPHER_NEUTRINO_API_KEY='...'
```

Because HLR/current-carrier services can consume paid credits, they never run automatically as part of a normal DIP.

## Optional API keys

```sh
# FreeCarrierLookup / Carrier247 programmatic backend
export SIPHER_DATA247_API_KEY='...'
# Optional if your Data247 account uses a different service code
export SIPHER_DATA247_API_CODE='C'

# Veriphone static carrier/line-type fallback
export SIPHER_VERIPHONE_API_KEY='...'

# Omkar carrier/line-type fallback
export SIPHER_OMKAR_API_KEY='...'
```

Do not commit API keys to GitHub or hard-code them into the SIPHER source tree.
