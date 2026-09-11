# SIPHER 2.1 DID Intelligence Providers

## Free/no-key sources

### USACallerLookup

US 10-digit numbers use the public JSON endpoint:

`https://www.usacallerlookup.com/wp-json/ucl/v1/number/{phone}`

SIPHER can fall back to the provider's public number page if the JSON response lacks carrier/type fields or cannot be read.

### SkipCalls

US spam reputation uses:

`https://spam.skipcalls.com/check/{10-digit-number}`

The documented response includes `number` and `is_spam`, with optional status/category fields. No key is required for the documented endpoint.

## Optional carrier fallbacks

### FreeCarrierLookup / Carrier247 / Data247

The browser form at FreeCarrierLookup.com is not scraped around anti-bot controls. When `SIPHER_DATA247_API_KEY` is present, SIPHER uses the supported Data247/Carrier247 API path instead. Returned fields may include carrier, wireless/mobile/landline/VoIP type, SMS and MMS gateway addresses, OCN, MNO, city/state/timezone and last port date.

### Veriphone

When `SIPHER_VERIPHONE_API_KEY` is present, SIPHER calls `https://api.veriphone.io/v3/verify` in `static` mode. The normal DIP does not automatically use Veriphone `mode=current`, because current-carrier lookups consume more credits.

### Omkar Phone Lookup API

When `SIPHER_OMKAR_API_KEY` is present, SIPHER calls `https://carrier-lookup-api.omkar.cloud/lookup` with the number in E.164 format and the key in the `API-Key` header.

## HTML/community reputation fallbacks

SpamCalls.net and tellows remain best-effort reputation fallbacks only. They are not required for the normal no-key path. If a provider blocks automated access or no live page exists, SIPHER silently moves to the next source and records only a compact status note if no provider returns anything.

## Deferred sources

Browser-only tools without a stable/documented programmatic endpoint are not made hard dependencies. This includes tools whose public UI may change or require interactive challenges. They can be revisited later if a supported API becomes available.
