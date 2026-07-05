# TLS / SSL — Concepts, Working & Requirements (No API)

> The **theory** companion to `TLS_SSL_Learning.md` and `TLS_SSL_API.md`.
> Here there is **no code and no library** — only *what each piece is, why it is required,
> and how it works mechanically*. Read this to understand the machine; read the API file to drive it.
>
> Every section follows the same shape:
> **① Requirement** (what problem forces this to exist) → **② Working** (the mechanism) →
> **③ Where it fits** (its role in TLS). Understand the requirement first and the mechanism
> stops feeling arbitrary.

---

## Table of Contents

1. [The core problem TLS solves](#1-the-core-problem-tls-solves)
2. [Symmetric encryption](#2-symmetric-encryption)
3. [The key-distribution problem](#3-the-key-distribution-problem)
4. [Diffie–Hellman key exchange](#4-diffiehellman-key-exchange)
5. [Asymmetric encryption (RSA)](#5-asymmetric-encryption-rsa)
6. [Hash functions](#6-hash-functions)
7. [MAC and HMAC](#7-mac-and-hmac)
8. [AEAD — authenticated encryption](#8-aead--authenticated-encryption)
9. [Digital signatures](#9-digital-signatures)
10. [The identity problem → certificates](#10-the-identity-problem--certificates)
11. [PKI — chain of trust](#11-pki--chain-of-trust)
12. [Putting it together: the handshake](#12-putting-it-together-the-handshake)
13. [The key schedule](#13-the-key-schedule)
14. [The record protocol](#14-the-record-protocol)
15. [Forward secrecy](#15-forward-secrecy)
16. [Session resumption](#16-session-resumption)
17. [TLS 1.2 vs 1.3 — conceptual diff](#17-tls-12-vs-13--conceptual-diff)
18. [The whole puzzle on one page](#18-the-whole-puzzle-on-one-page)

---

## 1. The core problem TLS solves

**① Requirement.** Two parties want to talk over a network they do **not** control. Anyone on the
path (Wi-Fi, ISP, a compromised router) can **read**, **modify**, **inject**, or **impersonate**.
A useful secure channel must therefore deliver **four** guarantees simultaneously:

| Guarantee | Threat it defeats | Meaning |
|---|---|---|
| **Confidentiality** | Eavesdropping | Nobody on the path can read the data |
| **Integrity** | Tampering | Any modification is detected |
| **Authenticity** | Injection / forgery | Each message provably came from the peer |
| **Identity** | Impersonation (MITM) | You are talking to the *real* server, not a fake |

**② Working.** No single tool provides all four. TLS is the *orchestration* that assembles them:
a symmetric cipher for confidentiality, an authentication tag for integrity+authenticity, a key
exchange to agree the symmetric key, and certificates for identity.

**③ Where it fits.** TLS is a **layer** between the transport (TCP) and the application (HTTP).
The application hands TLS plaintext; TLS returns ciphertext to TCP. Neither side rewrites the app,
which is why the same TLS secures HTTP, email, MQTT, and databases.

> **Why not "just encrypt"?** Encryption alone gives *only* confidentiality. An attacker who can't
> read your data can still flip bits (no integrity), replay it, or pretend to be the server
> (no identity). Every later section exists to close one of these remaining gaps.

---

## 2. Symmetric encryption

**① Requirement.** Bulk application data must be hidden, fast, and cheap — potentially gigabytes.
This calls for a cipher where **both** sides use the **same secret key** to encrypt and decrypt.

**② Working.**
- A **block cipher** (e.g. **AES**) transforms fixed-size 16-byte blocks under a key. On its own it
  only scrambles one block, so we wrap it in a **mode of operation**:
  - **CBC** — chains blocks together (each block XORed with the previous ciphertext). Needs padding;
    historically fragile (see padding oracles).
  - **CTR** — turns the block cipher into a keystream you XOR with data; no padding.
  - **GCM** — CTR mode **plus** a built-in authentication tag → this is AEAD (§8).
- A **stream cipher** (e.g. **ChaCha20**) generates a keystream directly and XORs it with the data.

Two non-negotiable rules:
- The key must be **secret and shared** — but the two sides have never met (that's §3).
- The **nonce/IV must never repeat** under the same key, or confidentiality collapses.

**③ Where it fits.** Symmetric encryption protects **every byte of application data** after the
handshake. It's fast because CPUs have hardware AES. The entire rest of TLS exists mostly to *safely
establish the key* this cipher needs.

---

## 3. The key-distribution problem

**① Requirement.** Symmetric encryption needs both sides to share a secret key. But the two ends
are strangers connected by a wire an attacker is watching. **How do you agree on a secret while an
eavesdropper records everything you say?** Mailing keys in advance doesn't scale to billions of
strangers on the web.

**② Working — the impasse.** If you send the key in the clear, the eavesdropper has it. If you
encrypt the key, you need a key to do *that* — infinite regress. Classical symmetric crypto simply
**cannot** solve this alone. This single problem is why public-key cryptography was invented.

**③ Where it fits.** This is the hinge of the whole subject. Two different solutions exist, and TLS
has used both:
- **Key agreement** — both sides jointly *compute* a shared secret that never crosses the wire
  → **Diffie–Hellman** (§4). *Modern TLS uses this.*
- **Key transport** — one side picks the secret and *encrypts it* to the other's public key
  → **RSA** (§5). *Legacy TLS used this; removed in 1.3 because it lacks forward secrecy.*

> Everything about the handshake follows from taking §3 seriously. Keep asking "but the attacker is
> watching — how does this still work?" and the design reveals itself.

---

## 4. Diffie–Hellman key exchange

**① Requirement.** From §3: agree on a shared secret over a public channel, such that an eavesdropper
who sees the *entire* exchange still cannot compute it.

**② Working (the intuition, then the math).**

*Paint-mixing analogy:* both agree on a public colour. Each secretly adds their own colour and
sends the **mix** across (mixing is easy, un-mixing is hard). Each then adds their private colour to
the *other's* mix. Both arrive at the identical final colour — but the eavesdropper, who saw only
the two public mixes, cannot un-mix to reach it.

*The math (finite-field DH):*
- Public parameters: a large prime `p` and a base `g`.
- Alice picks secret `a`, sends `A = gᵃ mod p`.
- Bob picks secret `b`, sends `B = gᵇ mod p`.
- Alice computes `Bᵃ = gᵇᵃ mod p`. Bob computes `Aᵇ = gᵃᵇ mod p`. **Both equal `gᵃᵇ mod p`.**
- The eavesdropper sees `g, p, A, B` but computing `a` from `A` is the **discrete-log problem** —
  infeasible for large `p`. So they cannot get `gᵃᵇ`.

*Tiny worked example* (do this by hand once): `p = 23, g = 5`. Alice `a = 6` → `A = 5⁶ mod 23 = 8`.
Bob `b = 15` → `B = 5¹⁵ mod 23 = 19`. Alice: `19⁶ mod 23 = 2`. Bob: `8¹⁵ mod 23 = 2`. Shared = **2**.

**ECDHE** is the same idea over **elliptic curves** — smaller keys, faster, same discrete-log
hardness. The **E** = *ephemeral*: a fresh `a`/`b` is generated **per connection** and thrown away
after — which is exactly what gives **forward secrecy** (§15).

**③ Where it fits.** DH/ECDHE is how modern TLS establishes the shared secret that seeds every
session key. Crucially, DH gives **secrecy but not identity** — you agreed a secret with *someone*,
but with *whom*? Answering that requires signatures (§9) and certificates (§10). DH + signatures
together is the heart of the handshake.

---

## 5. Asymmetric encryption (RSA)

**① Requirement.** We need operations where a **public** key can be shared freely while a **private**
key stays secret — enabling both *encrypting to someone* and *proving who you are*.

**② Working.**
- A **key pair**: public key (published) and private key (secret), mathematically linked via a
  **trapdoor** — easy one way, infeasible to reverse. RSA's trapdoor is factoring the product of
  two large primes.
- **Encryption direction:** anyone encrypts with your **public** key; only your **private** key
  decrypts. → *key transport* (legacy TLS: client encrypts the pre-master secret to the server).
- **Signing direction:** you sign with your **private** key; anyone verifies with your **public** key.
  → *digital signatures* (§9), which is how RSA is still used in modern TLS.

**③ Where it fits.**
- **Legacy (TLS ≤1.2, now removed):** RSA *key transport* — simple, but if the server's private key
  ever leaks, **all** past recorded sessions can be decrypted (no forward secrecy). This is the
  central reason TLS 1.3 dropped it.
- **Modern:** RSA (or ECDSA/EdDSA) is used only to **sign** the handshake, proving the server owns
  the private key matching its certificate. The *secret* itself comes from ECDHE (§4), not RSA.

> Distinction to burn in: **DH agrees a secret; RSA either transports a secret *or* signs.** Modern
> TLS = ECDHE for the secret + a signature for identity. RSA-the-cipher faded; RSA-the-signature stayed.

---

## 6. Hash functions

**① Requirement.** We need a way to reduce any amount of data to a short, fixed **fingerprint** such
that (a) the same input always gives the same output, and (b) you cannot find two inputs with the
same output or reverse the output back to input. This underpins integrity, signatures, and key
derivation.

**② Working.** A cryptographic hash (e.g. **SHA-256** → 32 bytes) has these properties:
- **Deterministic** — same input, same digest.
- **Preimage resistance** — given a digest, you can't find an input that produces it.
- **Collision resistance** — you can't find two different inputs with the same digest.
- **Avalanche** — flipping one input bit changes ~half the output bits.

MD5 and SHA-1 are **broken** for collision resistance and must not be used for security.

**③ Where it fits.** Hashes appear everywhere in TLS:
- Inside **HMAC** (§7) and the **key schedule / HKDF** (§13).
- **Signatures** sign the *hash* of the data, not the data itself (§9).
- The handshake **Finished** message hashes the entire transcript so any tampering with earlier
  handshake messages is detected.

---

## 7. MAC and HMAC

**① Requirement.** Encryption hides data but does **not** prove it was unmodified or that it came
from the right party. An attacker can flip ciphertext bits. We need **integrity + authenticity**:
a tag that only someone holding the shared key could have produced over this exact message.

**② Working.**
- A **MAC** (Message Authentication Code) takes `(key, message)` → a short tag. The receiver
  recomputes the tag with the same key; if it matches, the message is intact **and** authentic
  (only key-holders can forge it).
- **HMAC** is the standard construction: `HMAC(key, msg) = H((key ⊕ opad) ‖ H((key ⊕ ipad) ‖ msg))`.
  The nested hashing defends against length-extension weaknesses of naive `H(key ‖ msg)`.
- MAC ≠ signature: a MAC uses a **shared secret** (either party can make/verify), a signature uses a
  **private/public** pair (only the owner can make; anyone can verify).

**③ Where it fits.** Older TLS combined a cipher (confidentiality) with HMAC (integrity) separately.
The *order* of these two operations turned out to be security-critical (encrypt-then-MAC is safe;
MAC-then-encrypt enabled padding oracles). Modern TLS sidesteps the whole ordering hazard by fusing
them into **AEAD** (§8).

---

## 8. AEAD — authenticated encryption

**① Requirement.** Doing "encrypt" and "authenticate" as two separate steps is error-prone —
history is littered with attacks caused by combining them wrongly (order, timing, padding). We want
**one** primitive that provides confidentiality **and** integrity/authenticity together, correctly,
by construction.

**② Working.** **AEAD** (Authenticated Encryption with Associated Data) takes
`(key, nonce, plaintext, associated_data)` and outputs `(ciphertext, auth_tag)`:
- **Plaintext** is encrypted (confidentiality).
- **Ciphertext + associated data** are covered by the **auth tag** (integrity/authenticity).
- **Associated data** is authenticated but *not* encrypted — e.g. TLS record headers that must be
  readable yet tamper-evident.
- Decryption **verifies the tag first**; if it fails, the data is rejected outright — no partial
  plaintext leaks.

Standard AEADs in TLS: **AES-GCM** (uses AES-CTR + a polynomial MAC) and **ChaCha20-Poly1305**
(a stream cipher + the Poly1305 MAC, fast on devices without AES hardware — relevant for MCUs).

The one rule: **never reuse a nonce with the same key** — GCM in particular fails catastrophically
(it can leak the authentication key).

**③ Where it fits.** AEAD is the **only** record-protection mechanism in TLS 1.3. Every application
record is one AEAD-sealed unit. This is a deliberate simplification: by removing separate cipher+MAC
combinations, TLS 1.3 eliminated an entire family of attacks.

---

## 9. Digital signatures

**① Requirement.** DH (§4) lets you share a secret with *someone*, but you don't know *who*. We need
the server to **prove it holds a specific private key** — without revealing that key — so we can
later tie that key to a name (§10). We also need to detect any tampering with the handshake.

**② Working.**
- To **sign**: hash the message (§6), then transform the hash with the **private** key → signature.
- To **verify**: anyone uses the **public** key to check the signature matches the message's hash.
- Only the private-key holder could have produced a signature that verifies — so a valid signature
  proves *possession of the private key* and that the signed data is unmodified.
- Algorithms: **RSA** signatures, **ECDSA**, **EdDSA (Ed25519)**.

**③ Where it fits.** In the handshake the server **signs** the key-exchange data (and transcript)
with the private key corresponding to the public key in its certificate. The client verifies it.
This binds "the party I did DH with" to "the party who owns this certificate's key." Signatures are
the bridge from *secrecy* (DH) to *identity* (certificates).

> Still missing: how do we know that public key belongs to `bank.com` and not an attacker? A
> signature proves key ownership, **not** name ownership. That final gap is §10.

---

## 10. The identity problem → certificates

**① Requirement.** After §9 we can verify "whoever I'm talking to owns private key *K*." But an
attacker also owns *their* key pair. Nothing so far stops a man-in-the-middle from presenting *their*
public key and signing with *their* private key. **We need to bind a public key to a human-meaningful
identity (a domain name) in a way the client can trust.**

**② Working — the certificate.** A **certificate** is a data structure that says, in effect:
*"The public key K belongs to the name bank.com,"* and is **digitally signed by a third party the
client already trusts** (a Certificate Authority, §11). Key fields:
- **Subject** — the identity (and **SAN**: the exact hostnames the cert is valid for).
- **Public key** — the key `K` being bound to that identity.
- **Issuer** — which CA vouched for it.
- **Validity period** — not-before / not-after dates.
- **CA's signature** — over all the above.

The client's validation logic:
1. Does the certificate's **signature** verify under a **trusted** CA's public key? (§11)
2. Is it **within its validity dates**?
3. Does the **hostname I dialed** match the certificate's **SAN**?
4. Has it been **revoked**? (CRL / OCSP)

Only if *all* pass does the client accept the server's key as genuinely belonging to that name.

**③ Where it fits.** The certificate is what upgrades "I share a secret with someone who owns key K"
into "I share a secret with **bank.com**." It is the answer to the identity/MITM threat from §1.
Without it, encryption merely secures your connection *to the attacker*.

---

## 11. PKI — chain of trust

**① Requirement.** A certificate is only as trustworthy as whoever signed it. But your browser can't
have a prior relationship with every website. **How do you trust a stranger's certificate without
having met its issuer?** You need a small, pre-trusted set of anchors that can vouch for everyone
else transitively.

**② Working.**
- Your OS/browser ships with a **root store**: a few hundred **root CA** public keys, trusted by
  default (**trust anchors**).
- Roots are kept offline and rarely used directly; they sign **intermediate CAs**, which sign
  **leaf** (server) certificates. This forms a **chain**: leaf → intermediate → root.
- **Validation walks the chain:** the server sends leaf + intermediates; the client verifies each
  signature up the chain until it reaches a root **already in its trust store**. If the chain
  terminates at a trusted root and every link's signature checks out, the leaf is trusted.
- **Revocation** handles "trusted, then compromised": **CRLs** (lists of revoked certs) and **OCSP**
  (ask the CA in real time). **OCSP stapling** lets the server attach a fresh "not revoked" proof so
  the client needn't contact the CA.
- **Certificate Transparency** logs make every issued certificate publicly auditable, so a
  mis-issued cert can be detected.

**③ Where it fits.** PKI is the trust backbone that makes §10's certificates meaningful at internet
scale. It converts "trust everyone individually" (impossible) into "trust a handful of roots, who
vouch for the rest" (workable). The whole system's security rests on those roots and the CAs behaving.

---

## 12. Putting it together: the handshake

**① Requirement.** Before any application data flows, the two sides must, in one negotiation:
agree which crypto to use, agree a shared secret (§4), prove the server's identity (§9–§11), and
confirm nothing was tampered with — all over the hostile wire. The **handshake** is that negotiation.

**② Working (TLS 1.2, conceptually — each message maps to a requirement).**

| Message | Purpose | Ties to |
|---|---|---|
| **ClientHello** | "I support these versions, cipher suites; here's my random; here's the hostname (SNI)" | negotiation |
| **ServerHello** | "We'll use *this* version + cipher suite; here's my random" | negotiation |
| **Certificate** | Server presents its cert chain | identity §10–§11 |
| **ServerKeyExchange** | Server's ephemeral DH public value, **signed** with its cert key | secret §4 + proof §9 |
| **CertificateRequest** *(optional)* | "Client, present *your* cert too" | mutual TLS |
| **ServerHelloDone** | "Your turn" | — |
| **ClientKeyExchange** | Client's DH public value | secret §4 |
| *(both compute)* | Derive the shared secret → session keys | key schedule §13 |
| **ChangeCipherSpec** | "Everything after this is encrypted" | switchover |
| **Finished** | A MAC over the **entire handshake transcript**, encrypted | integrity of the whole handshake |

The **Finished** message is subtle but vital: because it authenticates a hash of *every* prior
handshake byte, an attacker who tried to tamper with, say, the cipher-suite list (a downgrade attack)
would cause the Finished check to fail on both sides. The handshake protects **itself**.

After both Finished messages verify, the tunnel is open and application data flows as AEAD records.

**③ Where it fits.** The handshake is the assembly point where **every** prior concept plays its
part: negotiation, DH secret, certificate identity, signature proof, hashing, and MAC-based
transcript integrity. If you can narrate this table and say *why* each line is required, you
understand TLS.

---

## 13. The key schedule

**① Requirement.** DH (§4) yields **one** shared secret. But a session needs **several** independent
keys: one for each direction (client→server, server→client), plus IV/nonce material, plus keys for
resumption. Reusing one key everywhere is dangerous. **How do you expand one secret into many
independent keys, safely?**

**② Working.** A **Key Derivation Function** stretches and separates the secret:
- TLS 1.2 uses a **PRF** (pseudo-random function) seeded by the shared secret + both randoms to
  produce the **master secret**, then expands that into the individual keys.
- TLS 1.3 uses **HKDF** (extract-then-expand). Each derived key is tagged with a distinct **label**
  (e.g. "client handshake traffic secret") so keys for different purposes are cryptographically
  independent even though they come from one root secret.
- The two client/server **randoms** are mixed in so that even identical secrets across sessions
  yield different keys (freshness, anti-replay).

**③ Where it fits.** The key schedule sits between "we agreed a secret" (§4) and "we can now encrypt
records" (§14). TLS 1.3's HKDF-based schedule also derives *handshake* keys early, which is how it
encrypts most of the handshake itself.

---

## 14. The record protocol

**① Requirement.** The handshake and all application data must be chopped into wire-sized units,
each **individually protected** and **ordered**, so tampering, reordering, or truncation is detected.

**② Working.** The **record layer** is TLS's envelope:
- Application data is split into **records** (max ~16 KB each).
- Each record is sealed with **AEAD** (§8): encrypted + tagged, under the current session key and a
  per-record **sequence number** folded into the nonce.
- The **sequence number** (never sent, but included in the AEAD computation) means a replayed or
  reordered record fails its tag check → **anti-replay** and **ordering** for free.
- A small header (type, version, length) travels as *associated data* — visible but tamper-evident.

**③ Where it fits.** Everything after "hello" — the rest of the handshake **and** all application
data — travels as records. It's the workhorse layer; the handshake just configures the keys it uses.

---

## 15. Forward secrecy

**① Requirement.** Suppose an attacker records your encrypted traffic today and **later** steals the
server's long-term private key (subpoena, breach, bug). Can they now decrypt that stored traffic?
With old RSA key transport (§5): **yes** — catastrophic. We want past sessions to stay safe even if
the long-term key is later compromised. That property is **forward secrecy**.

**② Working.** Use **ephemeral** Diffie–Hellman (**ECDHE**): generate a fresh DH key pair **per
connection** and **discard** it immediately after. The session key derives from these ephemeral
values, **not** from the server's long-term key. The long-term key is used only to *sign* (prove
identity, §9), never to establish the secret.

Consequently, when the long-term key later leaks: it lets an attacker *impersonate* the server going
forward, but it **cannot** reconstruct the discarded ephemeral secrets, so **past** recorded sessions
remain unreadable.

**③ Where it fits.** Forward secrecy is *the* reason TLS 1.3 mandates ephemeral key exchange and
removed RSA key transport entirely. It's the clearest example of an attack (§5's key-leak scenario)
directly dictating a design rule.

---

## 16. Session resumption

**① Requirement.** A full handshake costs round trips and public-key math — expensive, especially
for many short connections (web pages, IoT devices waking on battery). If two parties **already**
did a full handshake recently, can they skip most of the work next time, safely?

**② Working.** After a successful handshake, the parties establish a **resumption secret**:
- **Session tickets / PSK:** the server hands the client an encrypted blob (a "ticket") containing
  the session state, or both cache a **Pre-Shared Key**. On reconnect, the client presents it and
  both resume with the already-agreed secret — skipping certificates and most public-key work.
- **TLS 1.3 0-RTT:** the client can even send application data *in its first message*, using the
  resumption secret — saving a full round trip.

The trade-off: **0-RTT data is replayable** (an attacker can resend the captured first flight), so it
must be limited to idempotent requests. And resumption should preserve forward secrecy carefully.

**③ Where it fits.** Resumption is a performance layer bolted onto the handshake. Conceptually it
answers "we've met before — let's not redo introductions," while §15 (forward secrecy) constrains
*how* the shortcut may reuse secrets.

---

## 17. TLS 1.2 vs 1.3 — conceptual diff

**① Requirement.** TLS 1.2 accumulated legacy options (RSA key transport, CBC, compression,
renegotiation) — each a source of attacks and complexity. TLS 1.3 is a *redesign* whose guiding
requirement was: **remove every foot-gun, keep only what's provably safe, and make it faster.**

**② Working — what changed and *why*.**

| Aspect | TLS 1.2 | TLS 1.3 | Reason |
|---|---|---|---|
| Handshake round trips | 2-RTT | **1-RTT** (0-RTT resume) | speed |
| Key exchange | RSA transport *or* (EC)DHE | **(EC)DHE only** | forward secrecy (§15) |
| Record protection | AEAD *or* CBC+HMAC | **AEAD only** | kills padding-oracle attacks (§8) |
| Compression | allowed | **removed** | kills CRIME/BREACH |
| Renegotiation | allowed | **removed** | kills renegotiation attacks |
| Handshake privacy | mostly cleartext | **most of it encrypted** | privacy |
| Cipher suite naming | 4 parts (kx-auth-cipher-hash) | **2 parts** (cipher+hash; kx/auth negotiated separately) | simplicity |

Every removal maps to a specific attack (see `TLS_SSL_Learning.md` Phase 5). TLS 1.3 is smaller
*because* insecure flexibility was deleted, not despite it.

**③ Where it fits.** Learn 1.2 first because its steps are explicit and pedagogical; then read 1.3
as "1.2 with the dangerous parts amputated and the safe path made mandatory and faster."

---

## 18. The whole puzzle on one page

Read top to bottom — each line *requires* the next:

```
I want to send data secretly to a server over a hostile network.
      │
      ▼  need bulk secrecy, fast
[2] Symmetric encryption (AES-GCM)  ── needs a shared key ──┐
      │                                                     │
      ▼  but we've never met and the wire is watched        │
[3] Key-distribution problem  ──────────────────────────────┤
      │                                                     │
      ▼  agree a secret in the open                          │
[4] Diffie–Hellman (ECDHE)  → gives a shared secret ────────┘
      │        but with WHOM? DH has no identity
      ▼
[9] Digital signatures  → server proves it owns a key
      │        but is that key really bank.com's?
      ▼
[10] Certificate  → binds key ⇆ name, signed by...
      │
      ▼
[11] PKI / CA chain  → ...a root my machine already trusts
      │        now I trust the key, and derived the secret
      ▼
[13] Key schedule (HKDF)  → expand secret into session keys
      │
      ▼
[8/14] AEAD records  → every byte of app data sealed & ordered
      │
      ▼
[15] Ephemeral keys ⇒ forward secrecy · [16] resumption ⇒ speed
```

If you can start at the top and *derive* each next box by asking **"but the attacker is still
watching — what's missing?"**, then the puzzle is whole and every design choice in TLS follows
inevitably from the one above it.

---

*Concepts file created 2026-07-05. Pair with `TLS_SSL_Learning.md` (the checklist roadmap) and
`TLS_SSL_API.md` (the practical command/function reference).*
