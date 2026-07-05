# TLS / SSL — One-Page Revision Sheet

> Print-and-glance summary of `TLS_SSL_Concepts.md` (§), `TLS_SSL_API.md`, `TLS_SSL_Scenarios.md` (Q).
> If any line here is fuzzy, that's your cue to open the full file.

---

## The 4 guarantees (everything exists to deliver these) — §1

| Guarantee | Defeats | Provided by |
|---|---|---|
| **Confidentiality** | eavesdropping | symmetric cipher (AES-GCM / ChaCha20) |
| **Integrity** | tampering | AEAD tag / HMAC |
| **Authenticity** | forgery/injection | AEAD tag / signature |
| **Identity** | impersonation (MITM) | certificate + CA chain |

> **The one lesson:** encryption ≠ identity. Most TLS mistakes collapse the two.

---

## The puzzle — each line *requires* the next — §18

```
Send data secretly over a hostile wire
  ▼ need fast bulk secrecy
Symmetric cipher (AES-GCM) ── needs a shared key ──┐
  ▼ but we never met & the wire is watched          │
Key-distribution problem ──────────────────────────┤
  ▼ agree a secret in the open                       │
Diffie–Hellman (ECDHE) → shared secret ────────────┘
  ▼ ...but with WHOM? DH has no identity
Digital signature → peer proves it owns a key
  ▼ ...but is that key really bank.com's?
Certificate → binds key ⇆ name, signed by...
  ▼
PKI / CA chain → ...a root my machine already trusts
  ▼ now trust the key + have the secret
Key schedule (HKDF) → expand into session keys
  ▼
AEAD records → every byte sealed & ordered
  ▼
Ephemeral keys ⇒ forward secrecy · resumption ⇒ speed
```
Derive each box by asking: *"the attacker is still watching — what's missing?"*

---

## Decode a cipher suite — §12

```
TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
     └┬──┘ └┬┘      └────┬────┘ └──┬─┘
   key exch  auth      cipher+mode  hash/PRF
   (secret) (identity) (confid.+integ.) (KDF)
```
TLS 1.3 shortens this to just `AES_128_GCM_SHA256` (kx/auth negotiated separately).

---

## Handshake at a glance (TLS 1.2) — §12 / Q1

```
Client                                   Server
  │─ ClientHello (versions,suites,rand,SNI) →│
  │← ServerHello (chosen suite, rand) ────────│
  │← Certificate (chain) ─────────────────────│  identity §10-11
  │← ServerKeyExchange (ECDHE pub, SIGNED) ───│  secret §4 + proof §9
  │← ServerHelloDone ─────────────────────────│
  │─ ClientKeyExchange (ECDHE pub) ──────────→│  secret §4
  │      both derive keys (HKDF/PRF §13)       │
  │─ ChangeCipherSpec + Finished ────────────→│  transcript MAC
  │← ChangeCipherSpec + Finished ─────────────│
  │========= AEAD application data =========== │  §8/§14
```
**TLS 1.3:** 1-RTT (key_share in ClientHello), most of handshake encrypted, ECDHE-only.

---

## Certificate validation — the client's checklist — §10-11 / Q8

1. **Signature** chains up to a **trusted root** in the store ✔
2. **Dates** valid (not-before / not-after) ✔
3. **Hostname** matches **SAN** (CN ignored) ✔
4. **Not revoked** (OCSP / CRL) ✔

Miss any → reject. `SSL_VERIFY_PEER` checks 1–2 & 4 but **NOT 3** → also call `SSL_set1_host`.

---

## Attack → the design rule it forced — Phase 5 / §D

| Attack | Fix / why the rule exists |
|---|---|
| Key leaked later decrypts past traffic | **ECDHE ephemeral** ⇒ forward secrecy §15 |
| POODLE/FREAK/Logjam (downgrade) | transcript MAC + `FALLBACK_SCSV` + 1.3 sentinel |
| CRIME/BREACH (compression length leak) | **compression removed** in 1.3 |
| Lucky13/POODLE (padding oracle) | **AEAD-only**, tag-before-plaintext §8 |
| Heartbleed | *impl bug*, not protocol — patch + rotate keys |
| GCM nonce reuse | forge + leak auth key ⇒ **never repeat nonce** §8 |
| SSL-stripping (first http:// request) | **HSTS** (+ preload) |
| Rogue/compromised CA | **pinning** (w/ backup) + **CT logs** |

---

## Golden commands — API §4/§13

```bash
# Debug a live handshake (your #1 tool)
openssl s_client -connect host:443 -servername host -showcerts

# Read a certificate
openssl x509 -in cert.pem -text -noout
openssl x509 -in cert.pem -noout -dates -ext subjectAltName

# Expiry of a live site
echo | openssl s_client -connect host:443 2>/dev/null | openssl x509 -noout -enddate

# Negotiated protocol + cipher
openssl s_client -connect host:443 </dev/null 2>/dev/null | grep -E "Protocol|Cipher"

# Does key match cert?  (two hashes must be equal)
openssl pkey -in k.key -pubout -outform DER | openssl sha256
openssl x509 -in c.crt -pubkey -noout | openssl pkey -pubin -pubout -outform DER | openssl sha256

# Verify a chain / audit posture
openssl verify -CAfile ca.crt server.crt      #  ·  testssl.sh host:443
```

---

## Universal library arc (any TLS lib) — API §0

```
ctx = new_context(method)      → configure_trust(ctx)  → configure_options(ctx)
conn = new_connection(ctx)     → attach_socket(conn,fd) → set_hostname("host")  ← SNI+SAN, never skip
handshake(conn)                → check_verify_result()==OK
read/write(conn)               → shutdown; free
```
OpenSSL `SSL_*` · mbedTLS `mbedtls_ssl_*` · wolfSSL `wolfSSL_*` — same arc, different names.

---

## Decision cheat-sheet — Scenarios

| Need | Reach for |
|---|---|
| Forward secrecy | ECDHE (ephemeral) |
| Confidentiality+integrity in one | AEAD (AES-GCM / ChaCha20-Poly1305) |
| Prove server identity | cert + CA chain |
| Prove client too | mTLS (client cert) |
| Cut reconnect cost | session resumption / PSK |
| TLS over UDP / IoT | DTLS |
| MCU without AES HW | ChaCha20-Poly1305 |
| Force HTTPS-only | HSTS (+preload) |
| Rogue-CA defense | pinning + CT |

---

## Common errors → first check — API §12 / §E

| Error | First check |
|---|---|
| `unable to get local issuer certificate` | server missing **intermediate** in chain |
| `hostname mismatch` | cert **SAN** ≠ name dialed; send SNI |
| `self signed certificate` | add it via `-CAfile` / trust store |
| `wrong version number` | speaking TLS to a **plaintext port** |
| `no shared cipher` | version/suite lists don't **overlap** |
| handshake hangs on MCU | **heap/stack** or **bad RNG seed** |
| verifies but MITM-able | missing **hostname check** (`set1_host`) |

**Never** `curl -k` / `SSL_VERIFY_NONE` to "fix" it — that deletes identity. Fix the chain.

---

## Embedded (FreeRTOS) quick notes — API §10 / §F

- Seed CSPRNG from a **hardware TRNG** — weak entropy silently breaks *everything*.
- Size **task stack + heap** for the handshake; a hang is usually RAM, not config.
- Prefer **EC keys** (smaller) + **resumption** (skip PK math on reconnect) to save RAM/battery.
- `mbedtls_ssl_set_hostname()` = SNI **and** SAN check in one call — always set it.
- UDP → **DTLS** (`TRANSPORT_DATAGRAM` + timers).

---

*One-page sheet · 2026-07-05 · full detail in Concepts/API/Scenarios files.*
