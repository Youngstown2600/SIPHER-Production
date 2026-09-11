# S.I.P.H.E.R. 1.0.0-r19-Multi-SIP

## Multi-account SIP engine

r19 replaces the single live PJSUA2 account with a shared endpoint that can own multiple independent SIP accounts simultaneously. Every configured account is created with registration enabled, so several identities can remain registered at the same time.

- Add, edit, remove, and select SIP accounts from **Settings → SIP Accounts…**.
- The **Line Access** page has an outbound-account selector. Switching it affects new calls only; existing calls stay bound to the account that created or received them.
- Incoming and outgoing call rows now show the associated SIP account ID.
- Accounts using the same transport type and local SIP port share the corresponding PJSIP transport; distinct transport/port combinations can coexist.
- Registration status is aggregated in the main status pill and retained per account in the account manager/history.
- STUN remains an endpoint-level PJSIP facility; SIPHER supplies the endpoint with the unique STUN server set used by loaded accounts.

## Zero-account startup

A SIP account is no longer required to start S.I.P.H.E.R. The GUI and CLI can launch with no configured identities. DID Intelligence, Carrier/Next-Out analysis where applicable, Switch Audit+, packet/capture utilities that do not require an account, activity/logging, and the offline Legacy reference panels remain accessible.

The GUI no longer opens a mandatory first-run SIP setup dialog. Existing configured `profile.conf` installations are migrated non-destructively into `accounts/legacy.conf` when the new account store is empty; the original file is left in place.

## Account storage

Multi-account profiles are stored as individual `.conf` files in an `accounts` directory beside the legacy profile location. Files continue to use the hardened `ProfileStore` atomic-save/private-permission behavior.

## CLI

The CLI also starts in zero-account mode and loads all valid account files. New commands include:

- `accounts`
- `account-use <id>`

The existing profile edit/reload workflow now operates on the active SIP account without restarting the entire endpoint, allowing the other accounts to remain registered.

## Preserved r18 functionality

r19 retains DID carrier/reputation lookup, actual next-hop/INVITE peer capture, carrier handoff analysis, Switch Audit+ transport/topology checks, packet capture, audio routing, and the offline-only Legacy Blue Box/Red Box historical panels.
