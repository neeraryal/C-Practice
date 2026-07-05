# TLS / SSL — Complete Learning Roadmap

> A bottom-up study path: crypto primitives → how they combine → the protocol that
> orchestrates them → the trust system around it → the attacks that shaped every design choice.
> Learn it in this order and each piece clicks into the puzzle instead of floating alone.
>
> **How to use this file:** work top-to-bottom. Tick `- [ ]` → `- [x]` as you go. Each phase
> ends with a **Puzzle-fit checkpoint** (can you explain *why*?) and **Hands-on** exercises —
> don't mark a phase done until you can pass its checkpoint from memory.

---

## Table of Contents

- [Guiding principle](#guiding-principle)
- [Phase 0 — Prerequisites](#phase-0--prerequisites)
- [Phase 1 — Cryptography Foundations](#phase-1--cryptography-foundations)
- [Phase 2 — PKI & Trust](#phase-2--pki--trust)
- [Phase 3 — The TLS Protocol Itself](#phase-3--the-tls-protocol-itself)
- [Phase 4 — Hands-On / Operational](#phase-4--hands-on--operational)
- [Phase 5 — Attacks & Why the Design Is What It Is](#phase-5--attacks--why-the-design-is-what-it-is)
- [Phase 6 — Advanced / Adjacent (embedded-focused)](#phase-6--advanced--adjacent-embedded-focused)
- [Suggested rhythm](#suggested-rhythm)
- [Resources](#resources)
- [Glossary quick-reference](#glossary-quick-reference)

---

## Guiding principle

TLS invents almost **no** cryptography of its own — it *composes* existing primitives to deliver
four guarantees at once:

| Guarantee | Provided by | Learned in |
|---|---|---|
| **Confidentiality** (nobody can read it) | Symmetric cipher (AES, ChaCha20) | Phase 1 |
| **Integrity + authenticity** (nobody tampered) | AEAD / HMAC | Phase 1 |
| **Key agreement** (both sides share a secret over a public wire) | Diffie–Hellman / ECDHE | Phase 1 |
| **Identity** (you're talking to who you think) | Certificates + signatures (PKI) | Phase 2 |

No single primitive gives all four — the whole protocol exists to *combine* them safely.
Keep this table in your head; every handshake step maps back to one of these rows.

---

## Phase 0 — Prerequisites

*The ground you stand on. Skip this and the rest floats in the air.*

- [ ] **Networking layering** — TCP/IP vs OSI, and *where TLS sits*: on top of TCP, below the application
- [ ] **TCP first** — a TCP connection (3-way handshake) completes *before* any TLS bytes flow
- [ ] **Ports** — 443 (HTTPS), 853 (DoT), 993/995 (mail-over-TLS)
- [ ] **HTTP basics** — request/response, so HTTPS = HTTP-over-TLS is your anchor model
- [ ] **Bits, bytes, endianness** — big-endian / network byte order
- [ ] **Encoding** — hex, Base64
- [ ] **ASN.1 / DER / PEM** — how certs & keys are *serialized* (half of "why won't this cert load" is here)

**Hands-on**
- [ ] `nc example.com 443` then watch it just sit there — prove TLS ≠ TCP; TCP connects, TLS hasn't started
- [ ] `echo "hello" | base64` and decode it back; open a `.pem` file and note the Base64 body between the `-----BEGIN-----` lines

> **Puzzle-fit checkpoint:** Explain, in one sentence each, what layer TLS operates at and why it needs a *reliable* transport (TCP) underneath it before it can begin.

---

## Phase 1 — Cryptography Foundations

*The puzzle pieces. Learn each primitive as a black box first: what it guarantees, what it does NOT, its inputs/outputs. Resist rabbit-holing into the math yet.*

### Symmetric encryption
- [ ] Block vs stream ciphers
- [ ] **AES** (block) and **ChaCha20** (stream)
- [ ] Modes of operation: **CBC**, **CTR**, **GCM**
- [ ] Why **ECB is broken** (the "ECB penguin")
- [ ] Padding (**PKCS#7**) — and why padding is a security minefield (foreshadows Phase 5)

### Hash functions
- [ ] **SHA-256 / SHA-384**
- [ ] Properties: preimage, second-preimage, **collision resistance**
- [ ] Why **MD5 and SHA-1 are dead**

### Message authentication
- [ ] **MAC → HMAC** (integrity + authenticity from a shared key)
- [ ] **AEAD** — authenticated encryption: **AES-GCM**, **ChaCha20-Poly1305**
- [ ] *Encrypt-then-MAC* vs *MAC-then-encrypt* — why order matters

### Asymmetric / public-key crypto
- [ ] **RSA** — encryption *and* signatures; the trapdoor intuition
- [ ] **Diffie–Hellman** key exchange — **the single most important idea to truly grok**
- [ ] Elliptic Curve crypto: **ECDH / ECDHE**, **ECDSA**, **EdDSA (Ed25519)**
- [ ] **Ephemeral** (the "E" in ECDHE) and **Forward Secrecy** — why ephemeral is the whole point

### Supporting primitives
- [ ] **CSPRNG / entropy** — where crypto silently dies if done wrong
- [ ] **Key Derivation Functions**: **HKDF** (central to TLS 1.3), PBKDF2
- [ ] Keep the three distinct: **encryption ≠ signature ≠ MAC**

**Hands-on**
- [ ] Encrypt a file with AES-GCM using OpenSSL; flip one ciphertext byte and watch authentication fail
- [ ] Do a Diffie–Hellman exchange by hand with tiny numbers (e.g. p=23, g=5) — compute the shared secret on both "sides" and confirm they match
- [ ] `sha256sum` a file, change one character, hash again — observe the avalanche effect

> **Puzzle-fit checkpoint:** Name which primitive delivers each row of the [guiding-principle](#guiding-principle) table, and state one thing each primitive does **not** protect against (e.g. "AES gives confidentiality but not integrity — that's why we need AEAD").

---

## Phase 2 — PKI & Trust

*Encryption without identity is useless — you'd have a perfectly secure channel to an attacker. This phase answers "who am I actually talking to?"*

### X.509 certificates
- [ ] Certificate structure & key fields: subject, issuer, validity, public key, **SAN**, key usage, extensions
- [ ] Leaf vs intermediate vs root

### Certificate Authorities & the chain of trust
- [ ] **CA hierarchy**: root → intermediate → leaf, and *why* the hierarchy exists
- [ ] **Trust stores / root stores** (OS & browser)
- [ ] **CSR** (Certificate Signing Request) — how you ask for a cert
- [ ] **Chain building** — server sends leaf + intermediates; client anchors to a trusted root

### Validation (the algorithm the client runs)
- [ ] Signature check up the chain
- [ ] Validity dates
- [ ] **Hostname / SAN matching**
- [ ] Path length & basic constraints

### Revocation
- [ ] **CRL** (certificate revocation lists)
- [ ] **OCSP** and **OCSP stapling**
- [ ] Why revocation is famously "broken" (soft-fail)

### Ecosystem
- [ ] **Certificate Transparency (CT) logs**
- [ ] **ACME / Let's Encrypt** — automated issuance
- [ ] Formats: **PEM, DER, PKCS#8** (keys), **PKCS#12 / PFX** (bundles)

**Hands-on**
- [ ] `openssl x509 -in cert.pem -text -noout` — read every field of a real cert
- [ ] `openssl s_client -connect example.com:443 -showcerts` — dump the full chain a server sends
- [ ] Build your own tiny CA: generate a root, sign a leaf cert, add the root to your machine's trust store, and serve a page your browser trusts
- [ ] Deliberately break hostname matching (issue a cert for the wrong name) and watch validation fail

> **Puzzle-fit checkpoint:** Explain exactly how PKI turns "I received a public key" into "I trust this key belongs to `example.com`," and trace how a man-in-the-middle is stopped at the validation step.

---

## Phase 3 — The TLS Protocol Itself

*Now the primitives + PKI get orchestrated. Learn **TLS 1.2 first** (explicit, pedagogical), then **TLS 1.3** (cleaner, but hides steps).*

### Structure
- [ ] **Record protocol** (the record layer) — the envelope everything travels in; fragmentation
- [ ] **Handshake protocol**, **Alert protocol**, **ChangeCipherSpec**

### TLS 1.2 full handshake — message by message
- [ ] **ClientHello** — version, client random, **cipher suite list**, extensions (**SNI**, ALPN…)
- [ ] **ServerHello** — chosen cipher suite, server random
- [ ] **Certificate**
- [ ] **ServerKeyExchange** (for ECDHE)
- [ ] **CertificateRequest** (for mutual TLS)
- [ ] **ServerHelloDone**
- [ ] **ClientKeyExchange** → the **pre-master secret**
- [ ] **Key schedule**: pre-master → **master secret** → session keys (via the **PRF**)
- [ ] **ChangeCipherSpec** → **Finished** — and *why* Finished authenticates the entire handshake transcript

### Reading a cipher suite (the moment it all unifies)
- [ ] Decode `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` into its four jobs:
  - `ECDHE` = key exchange · `RSA` = authentication · `AES_128_GCM` = cipher+mode · `SHA256` = hash/PRF

### TLS 1.3 — what changed and *why*
- [ ] **1-RTT** handshake (`key_share` sent early in ClientHello)
- [ ] **0-RTT** resumption — and its **replay risk**
- [ ] **HKDF-based key schedule**
- [ ] Removed features — and the attack each removal kills (cross-reference Phase 5):
  - RSA key transport (no forward secrecy) · static DH · CBC · renegotiation · compression
- [ ] Encrypted handshake messages (more of the handshake hidden from eavesdroppers)

### Cross-cutting
- [ ] **Extensions**: SNI, ALPN, supported_groups, signature_algorithms, key_share, **ECH/ESNI**
- [ ] **Session resumption**: session IDs, **session tickets**, **PSK** in 1.3
- [ ] **Version history**: SSL 2.0/3.0 → TLS 1.0/1.1/1.2/1.3 and the deprecation timeline

**Hands-on**
- [ ] Capture a full handshake in **Wireshark** and label every message against the checklist above
- [ ] Compare a TLS 1.2 vs a TLS 1.3 capture side by side — count the round trips
- [ ] Force a specific version: `openssl s_client -tls1_2 -connect example.com:443` vs `-tls1_3`

> **Puzzle-fit checkpoint:** Draw the TLS 1.2 handshake from memory and annotate *which Phase-1 primitive does each job*. Then explain what TLS 1.3 collapsed into 1-RTT and why it's still safe.

---

## Phase 4 — Hands-On / Operational

*Theory locks in only when you touch packets. Interleave this with Phase 3 — don't wait until the end.*

- [ ] **OpenSSL CLI**: generate keys/CSRs/self-signed certs; inspect with `openssl x509 -text`
- [ ] **`openssl s_client -connect host:443`** — walk a live handshake and read the output
- [ ] **Wireshark** — capture and read a real handshake byte by byte (*the single most illuminating exercise here*)
- [ ] **Run your own CA** end to end (ties back to Phase 2 hands-on)
- [ ] **Configure a server** (nginx / Apache): protocol versions, cipher-suite ordering, **HSTS**
- [ ] **Mutual TLS (mTLS)** — issue and require client certificates
- [ ] **TLS termination**, reverse proxies, SNI-based routing
- [ ] Audit posture with **SSL Labs** (public) and **`testssl.sh`** (local)

**Hands-on**
- [ ] Stand up an HTTPS server locally with a self-signed cert, then with your own-CA cert
- [ ] Run `testssl.sh` against it and fix every warning it raises
- [ ] Configure and test mTLS — connection succeeds *with* the client cert, is rejected without it

> **Puzzle-fit checkpoint:** Given a server config, predict what SSL Labs will grade it and why — before you run the scan.

---

## Phase 5 — Attacks & Why the Design Is What It Is

*What separates "I know the steps" from "I understand TLS." Every deprecated feature is a tombstone for an attack.*

- [ ] **Downgrade attacks**: POODLE, FREAK, Logjam — and the `TLS_FALLBACK_SCSV` defense
- [ ] **Compression attacks**: CRIME, BREACH → *why compression was removed*
- [ ] **Padding oracles**: Lucky13, POODLE → *why CBC died and AEAD won*
- [ ] **Heartbleed** — an *implementation* bug, not a protocol flaw (learn to tell these apart)
- [ ] **RC4 biases**; **weak / small-subgroup DH**
- [ ] **MITM** — and exactly how PKI + hostname validation defeats it
- [ ] **Forward secrecy revisited** — what an attacker who steals the server's key *later* can and cannot decrypt
- [ ] **Nonce / IV reuse** catastrophes (especially with GCM)

**Hands-on**
- [ ] For each removed TLS 1.3 feature from Phase 3, write one line naming the attack it prevents
- [ ] Read the Heartbleed CVE and explain why *no* protocol change was needed to fix it

> **Puzzle-fit checkpoint:** For any TLS 1.3 design choice, name the attack it prevents. When you can do this reliably, the puzzle is **complete**.

---

## Phase 6 — Advanced / Adjacent (embedded-focused)

*High-value given embedded / FreeRTOS work — this is where TLS meets constrained devices.*

- [ ] **DTLS** (TLS over UDP) — the standard for constrained / IoT / real-time; how it handles packet loss & reordering
- [ ] **Embedded TLS stacks**: **mbedTLS**, **wolfSSL**, **BearSSL** — footprint, hardware crypto offload, cert storage on an MCU
- [ ] **TLS + FreeRTOS** — FreeRTOS+TCP integration, memory constraints of a handshake on a microcontroller
- [ ] **QUIC / HTTP/3** — TLS 1.3 baked directly into the transport
- [ ] **Post-quantum TLS** — hybrid key exchange (ML-KEM / Kyber); the near-future migration
- [ ] **Library landscape**: OpenSSL, BoringSSL, LibreSSL, rustls, s2n — who uses what and why

**Hands-on**
- [ ] Build mbedTLS or wolfSSL and run its example TLS client; note the RAM/flash cost of a handshake
- [ ] Sketch how a TLS session would fit in your FreeRTOS memory budget (stack per task, heap for the handshake)

> **Puzzle-fit checkpoint:** Explain why DTLS exists instead of "just run TLS over UDP," and what a microcontroller trades away to do TLS at all.

---

## Suggested rhythm

1. **Phase 1** primitives as black boxes (don't rabbit-hole into the math yet).
2. **Phase 2** PKI.
3. **Phase 3** TLS 1.2 **while** doing **Phase 4** (OpenSSL + Wireshark in parallel).
4. **Phase 3** TLS 1.3.
5. **Phase 5** attacks — *now* revisit Phase 1 math wherever curiosity pulls you.
6. **Phase 6** for the embedded angle.

The trick: theory (odd phases) and hands-on (even phases) are meant to **interleave**, not run in sequence. Capture a real handshake the same day you first read about ClientHello.

---

## Resources

**Anchor book (structured almost exactly like this roadmap)**
- *Bulletproof TLS and PKI* — Ivan Ristić

**Primary sources (read once concepts are solid)**
- **RFC 8446** — TLS 1.3
- **RFC 5246** — TLS 1.2
- **RFC 6347** — DTLS 1.2 · **RFC 9147** — DTLS 1.3

**Interactive / visual**
- *The Illustrated TLS Connection* (tls13.xargs.org) — byte-by-byte annotated handshake
- Wireshark + a browser — your own captures beat any diagram
- SSL Labs (ssllabs.com/ssltest) and `testssl.sh`

**Crypto foundations (Phase 1)**
- *Serious Cryptography* — Jean-Philippe Aumasson
- Cryptography I (Dan Boneh, Coursera) — for the primitives

---

## Glossary quick-reference

| Term | One-liner |
|---|---|
| **AEAD** | Encrypt + authenticate in one primitive (AES-GCM, ChaCha20-Poly1305) |
| **ECDHE** | Ephemeral elliptic-curve Diffie–Hellman — key agreement with forward secrecy |
| **Forward secrecy** | Stealing the server's long-term key later can't decrypt past sessions |
| **PKI** | The CA/certificate system that binds a public key to an identity |
| **X.509** | The certificate format |
| **SAN** | Subject Alternative Name — the hostnames a cert is valid for |
| **OCSP stapling** | Server attaches a fresh "not revoked" proof so the client needn't ask the CA |
| **SNI** | ClientHello extension naming the host, so one IP can serve many certs |
| **PRF / HKDF** | The function that stretches the shared secret into actual session keys |
| **mTLS** | Both sides present certificates (client authenticates too) |
| **DTLS** | TLS adapted for UDP / unreliable transport |

---

*Roadmap created 2026-07-05. Tick boxes as you progress; a phase isn't done until you can pass its Puzzle-fit checkpoint from memory.*
