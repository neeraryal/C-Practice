# TLS / SSL — Scenario-Based Questions & Troubleshooting

> 42 "what happens if…" scenarios, interview questions, and real troubleshooting walkthroughs.
> Each has an **Answer** (what a strong engineer says), the **Why** (the reasoning being tested),
> and a **Trap** (the wrong-but-tempting response), with cross-references into the theory
> (`TLS_SSL_Concepts.md` §N) and tooling (`TLS_SSL_API.md`).
>
> Use this to *test* understanding once you've studied — if you can answer these cold, the puzzle
> is whole.

---

## Table of Contents

- [A. Handshake & Negotiation](#a-handshake--negotiation)
- [B. Certificates & PKI](#b-certificates--pki)
- [C. Keys, Secrets & Forward Secrecy](#c-keys-secrets--forward-secrecy)
- [D. Attacks & Defenses](#d-attacks--defenses)
- [E. Debugging & Operations](#e-debugging--operations)
- [F. Embedded / DTLS / FreeRTOS](#f-embedded--dtls--freertos)
- [G. Design & Architecture](#g-design--architecture)
- [Quick Decision Cheat Sheet](#quick-decision-cheat-sheet)

---

## A. Handshake & Negotiation

### Q1. Walk me through what happens between `TCP connected` and `first byte of HTTP` on an HTTPS request.

**Answer:** TCP is already up. Then the TLS handshake (Concepts §12): **ClientHello** (offered
versions, cipher suites, client random, SNI) → **ServerHello** (chosen version + suite, server
random) → server **Certificate** chain → **key exchange** (server's ephemeral ECDHE public value,
**signed** with its cert key) → client's ECDHE public value → both derive the same shared secret →
**key schedule** expands it into session keys → **Finished** messages (a MAC over the whole
transcript) verify on both sides → tunnel open → *now* the HTTP request flows as an AEAD record.

**Why:** They want to hear each message tied to a *purpose*: negotiate, agree a secret, prove
identity, verify integrity of the handshake itself.

**Trap:** Saying "the server sends its public key and the client encrypts data with it." That's
legacy RSA key transport (removed in 1.3) and conflates key *transport* with key *agreement* (§5).

---

### Q2. In TLS 1.3 the client sends application data before the handshake fully completes. How, and what's the risk?

**Answer:** That's **0-RTT** resumption (Concepts §16). On a *resumed* connection the client already
has a resumption secret (PSK), so it can encrypt early data in its first flight — saving a round
trip. **Risk:** 0-RTT data is **replayable** — an attacker can capture and resend that first flight.
So it must be restricted to **idempotent** requests (e.g. a GET, never a "transfer $100").

**Why:** Tests whether you understand the performance-vs-safety trade of 0-RTT, not just that it's
"faster."

**Trap:** Claiming 0-RTT has full forward secrecy and no downsides.

---

### Q3. Client offers TLS 1.0–1.3; server supports only 1.2–1.3. What version is used and how is downgrade abuse prevented?

**Answer:** They negotiate **1.3** (highest common). An active attacker could strip the ClientHello
to force a lower version, but the **Finished** message MACs the entire transcript (§12), and TLS 1.3
adds a **downgrade sentinel** in the server random — so a forced downgrade is detected and the
handshake aborts.

**Why:** Version negotiation + how the protocol protects *its own* negotiation.

**Trap:** "Whatever the client asks for" — the *server* picks from the client's offer; the client
doesn't dictate.

---

### Q4. Two sites share one IP address. How does the server know which certificate to present?

**Answer:** **SNI** (Server Name Indication) — a ClientHello extension carrying the hostname in the
clear, *before* the certificate is chosen (Concepts §10, API `SSL_set_tlsext_host_name`). The server
uses it to select the matching virtual host's cert.

**Why:** SNI is why virtual hosting over TLS works at all.

**Trap:** Forgetting SNI is sent early and unencrypted (which is why **ECH/Encrypted Client Hello**
exists — to hide it).

---

### Q5. A client and server can't complete a handshake: `no shared cipher`. What's happening?

**Answer:** The client's offered cipher suites (or versions) and the server's configured ones don't
**overlap** (Concepts §12 negotiation). Common causes: server pinned to modern AEAD suites while the
client is ancient, or a version-floor mismatch (server requires ≥1.2, client maxes at 1.0).

**Why:** Negotiation is an *intersection*; empty intersection = failure.

**Fix:** Align `min_proto_version` and cipher lists (API §9); inspect with
`openssl s_client -tls1_2` / `openssl ciphers -v`.

---

### Q6. What exactly does the `Finished` message protect, and why is it encrypted?

**Answer:** It carries a MAC/hash over the **entire handshake transcript** so far (§12). Both sides
compute it independently; if any earlier handshake byte was tampered with (a stripped extension, a
downgraded suite), the values differ and the connection aborts. It's sent under the freshly derived
keys, proving both sides actually derived the *same* secret.

**Why:** This is *the* mechanism by which the handshake authenticates itself.

**Trap:** Thinking integrity only starts with application data — the handshake protects itself first.

---

## B. Certificates & PKI

### Q7. `openssl s_client` shows `Verify return code: 21 (unable to get local issuer certificate)`, but the browser is fine. Why the difference?

**Answer:** The **server isn't sending the intermediate** cert. Browsers often cache intermediates
or fetch them via AIA; `openssl` and many libraries don't — they need the server to present
**leaf + intermediates** (Concepts §11). Fix: configure the server to send the full chain
(API `SSL_CTX_use_certificate_chain_file` with the bundle).

**Why:** Chain-building responsibility sits with the *server*; relying on client leniency is a
latent bug that bites non-browser clients (curl, mobile, MCUs).

**Trap:** "Add the leaf to the client's trust store" — wrong; you fix the server's chain.

---

### Q8. A cert validates its signature and dates fine, but the connection is still rejected. Name a reason.

**Answer:** **Hostname/SAN mismatch** (Concepts §10) — the name you dialed isn't in the certificate's
`subjectAltName`. Modern clients ignore CN entirely and match only SAN. Other candidates:
**revoked** (OCSP/CRL), or a **name-constrained** intermediate.

**Why:** Signature validity ≠ identity validity. A valid cert *for the wrong name* is still an MITM.

**Trap:** Assuming a cryptographically valid signature means the cert is acceptable. See also
`TLS_SSL_API.md` §12 gotcha: `SSL_VERIFY_PEER` without `SSL_set1_host`.

---

### Q9. Why can't an attacker who owns a valid certificate for `evil.com` use it to MITM `bank.com`?

**Answer:** Because validation checks that the **hostname matches the cert's SAN** (§10). The
attacker's cert says `evil.com`; when the client dialed `bank.com`, the SAN check fails. Owning *a*
valid cert doesn't let you impersonate *another* name.

**Why:** Separates "this is a real cert" from "this is the real cert *for this name*."

**Trap:** Thinking any CA-signed cert works for any site.

---

### Q10. The server's private key was stolen. What's the blast radius, and does forward secrecy help?

**Answer:** Going **forward**, the attacker can impersonate the server and must be stopped by
**revoking** the cert and reissuing. For **past** recorded traffic: with **ECDHE** (forward secrecy,
§15) those sessions stay safe because the session keys came from discarded ephemeral values, not the
long-term key. Without forward secrecy (old RSA key transport), all recorded past sessions are now
decryptable.

**Why:** The whole point of forward secrecy, framed as an incident.

**Trap:** "Just rotate the key and we're fine" — ignores already-captured ciphertext.

---

### Q11. What is OCSP stapling and what problem does it solve?

**Answer:** Revocation checking normally makes the *client* contact the CA's OCSP responder — slow
and a privacy leak (the CA learns who you visit). **OCSP stapling** has the **server** fetch a
short-lived signed "not revoked" proof and **attach ("staple")** it to the handshake, so the client
gets a fresh revocation status with no extra round trip and no CA contact (§11).

**Why:** Understanding revocation's real-world performance/privacy issues.

**Trap:** Confusing stapling (server-provided) with the client querying OCSP directly.

---

### Q12. Why is a self-signed certificate rejected, and when is that actually fine?

**Answer:** It's rejected because its chain terminates at **itself**, not at a root in the client's
**trust store** — there's no trusted third party vouching (§11). It's fine when *you* control both
ends and explicitly add that cert/root to the trust store (internal services, dev, device fleets
with a pinned CA). See `TLS_SSL_API.md` §6 (build-your-own-CA).

**Why:** Trust is about *anchoring to something pre-trusted*, not about crypto validity.

**Trap:** "Self-signed = insecure." It's untrusted-by-default, not weak; with pinning it can be
*more* secure than public CAs.

---

### Q13. What does Certificate Transparency add that CAs alone don't?

**Answer:** Public, append-only **CT logs** record every issued certificate, so a **mis-issued** or
fraudulent cert (a CA wrongly signing `google.com`) is publicly detectable and auditable (§11).
Browsers can require proof of CT logging (SCTs).

**Why:** Defends against a *misbehaving or compromised CA* — a threat plain PKI doesn't cover.

**Trap:** Thinking CT encrypts or hides anything — it's about *auditability*, the opposite.

---

## C. Keys, Secrets & Forward Secrecy

### Q14. One Diffie–Hellman exchange yields one shared secret. Why does TLS derive multiple keys from it, and how?

**Answer:** A session needs **independent** keys — separate keys per direction (client→server,
server→server), IV/nonce material, and resumption secrets — so a weakness in one context can't
compromise another. The **key schedule** (§13) uses a KDF (**HKDF** in 1.3) with distinct **labels**
to expand the one secret into many cryptographically independent keys, mixing in both randoms for
freshness.

**Why:** Key separation is a core hygiene principle; reusing one key everywhere is dangerous.

**Trap:** "Both sides just use the DH secret as the AES key." Raw DH output is never used directly.

---

### Q15. What does "ephemeral" mean in ECDHE and why does it matter more than the cipher choice?

**Answer:** Ephemeral = a **fresh** DH key pair generated **per connection** and **discarded** after
(§4, §15). It matters because it delivers **forward secrecy**: even if the server's long-term key
leaks later, discarded ephemeral secrets can't be reconstructed, so past sessions stay private. A
strong cipher without forward secrecy still exposes all recorded traffic if the key later leaks.

**Why:** Tests whether you value forward secrecy as a property, not just "AES-256 = strong."

**Trap:** Equating key strength with forward secrecy; they're independent.

---

### Q16. If an attacker records an entire ECDHE handshake, why can't they compute the session key?

**Answer:** They see `g, p` and both public values `A=gᵃ`, `B=gᵇ`, but recovering `a` or `b` is the
**discrete-log problem** — infeasible for large parameters (§4). Without a private exponent they
can't compute `gᵃᵇ`, so the shared secret — and every key derived from it — stays out of reach.

**Why:** The math intuition behind why passive eavesdropping fails against DH.

**Trap:** Thinking seeing the whole exchange is enough; the hardness is in the one-way function.

---

### Q17. What breaks catastrophically if a nonce/IV is reused with the same key in AES-GCM?

**Answer:** Nonce reuse in GCM can leak the **authentication key** (the GHASH key), letting an
attacker **forge** messages, and it also breaks confidentiality of the affected records (§8). It's
one of the sharpest foot-guns in AEAD.

**Why:** Reinforces the "never repeat a nonce under one key" rule from §8/§14.

**Trap:** Treating IV reuse as a minor confidentiality dent; in GCM it's an integrity catastrophe.

---

### Q18. Why did TLS 1.3 remove RSA key transport entirely?

**Answer:** RSA key transport (client encrypts the pre-master secret to the server's public key,
§5) has **no forward secrecy** — one leaked long-term key decrypts all past recorded sessions. TLS
1.3 mandates **ephemeral (EC)DHE** so that property holds by construction (§15, §17).

**Why:** A concrete example of an attack scenario dictating a protocol design decision.

**Trap:** "RSA is broken." RSA *signatures* are fine and still used; it's RSA *key transport's* lack
of forward secrecy that was removed.

---

## D. Attacks & Defenses

### Q19. Explain a downgrade attack and TLS's defense.

**Answer:** An active attacker tampers with the ClientHello to make the parties negotiate a weaker,
attackable version/cipher (POODLE, FREAK, Logjam — Learning Phase 5). Defenses: the **Finished**
transcript MAC (§12) detects handshake tampering; `TLS_FALLBACK_SCSV` signals "I'm deliberately
retrying lower" so a server can reject a forced fallback; TLS 1.3 adds a downgrade sentinel.

**Why:** Ties an attack class to the specific mechanisms that stop it.

**Trap:** Assuming higher-version support alone prevents downgrade — you need the anti-downgrade
signaling too.

---

### Q20. Why did TLS 1.3 remove compression?

**Answer:** TLS-level compression enabled **CRIME/BREACH**: because compression size depends on
content, an attacker who injects guesses and watches the compressed length can recover secrets
(session cookies) byte by byte (Phase 5). Removing compression removes the side channel.

**Why:** Shows a subtle *side-channel* attack, not a crypto break.

**Trap:** Thinking compression is harmless because the data is encrypted — the *length* leaks.

---

### Q21. What class of attack killed CBC mode in TLS, conceptually?

**Answer:** **Padding-oracle** attacks (Lucky13, POODLE): MAC-then-encrypt with CBC padding let an
attacker learn plaintext by observing whether padding was valid (via errors or timing) (§7, §8,
Phase 5). Modern TLS uses **AEAD** only, which verifies the tag before touching plaintext and has no
separate padding oracle.

**Why:** Explains *why* AEAD replaced cipher+MAC, not just that it did.

**Trap:** Blaming AES; the flaw was the *mode + MAC ordering + padding*, not the cipher.

---

### Q22. Heartbleed — was it a protocol flaw or an implementation bug? Why does the distinction matter?

**Answer:** An **implementation bug** in OpenSSL (a missing bounds check in the heartbeat extension
leaked memory) — the TLS *protocol* was fine (Phase 5). It matters because the fix was a code patch
and key rotation, **not** a protocol change; it shows that a perfect protocol can still be undone by
a buggy library.

**Why:** Distinguishing protocol design from implementation is a mark of real understanding.

**Trap:** "TLS was broken." The spec wasn't; one library was.

---

### Q23. How does TLS stop a man-in-the-middle who sits between client and server and relays everything?

**Answer:** The MITM can relay ciphertext but can't impersonate the server: to complete the
handshake it must present a **certificate for the requested hostname signed by a trusted CA** and
**prove possession of the matching private key** (sign the key exchange, §9–§10). It has neither, so
either the SAN check or the signature check fails and the client aborts.

**Why:** Ties the whole PKI + signature stack to the concrete threat it exists for (§1, §10).

**Trap:** "Encryption stops MITM." Encryption alone would just be encrypted-to-the-attacker;
*identity* stops MITM.

---

### Q24. A developer sets `curl -k` / `SSL_VERIFY_NONE` to "fix" a cert error in production. What did they actually do?

**Answer:** They **disabled identity verification** — deleting the entire anti-MITM guarantee (§10).
The channel is still encrypted, but to *anyone* who can intercept, including an attacker with a
self-signed cert. It "works" precisely because it now accepts *any* certificate.

**Why:** The most common real-world security own-goal; tests judgment, not just knowledge.

**Trap:** Treating it as a harmless workaround. Fix the **trust chain** instead (API §12 golden rule).

---

## E. Debugging & Operations

### Q25. `wrong version number` error when connecting. First thing you check?

**Answer:** You're likely speaking **TLS to a plaintext port** (or vice versa) — e.g. hitting port 80
with HTTPS, or a service that isn't actually doing TLS on that port. Confirm with
`openssl s_client -connect host:port` and check what the port really speaks (API §12).

**Why:** Classic mismatch; the error text is misleading if you assume it's a version negotiation
issue.

**Trap:** Chasing TLS version config when the port simply isn't TLS.

---

### Q26. How do you check when a live site's certificate expires, from the command line?

**Answer:**
```bash
echo | openssl s_client -connect host:443 -servername host 2>/dev/null \
  | openssl x509 -noout -enddate
```
(API §13 cheat-sheet.) For full detail use `-dates` (both not-before/not-after) or inspect the whole
cert with `-text`.

**Why:** Bread-and-butter ops task; expiry is the #1 cause of TLS outages.

**Trap:** Reading the file on disk when the *deployed* cert may differ — always check the live
endpoint.

---

### Q27. How do you confirm a private key actually matches a certificate?

**Answer:** Compare the public-key hashes — they must be identical (API §13):
```bash
openssl pkey -in server.key -pubout -outform DER | openssl sha256
openssl x509 -in server.crt -pubkey -noout | openssl pkey -pubin -pubout -outform DER | openssl sha256
```
(Classically people compare the modulus for RSA; the pubkey-hash method works for EC too.)

**Why:** A mismatched key/cert is a frequent deploy failure (`key values mismatch`).

**Trap:** Assuming files named together belong together.

---

### Q28. You need to see exactly which protocol and cipher suite a connection negotiated. How?

**Answer:**
```bash
openssl s_client -connect host:443 -servername host </dev/null 2>/dev/null \
  | grep -E "Protocol|Cipher"
```
Or in code: `SSL_get_version(ssl)` and `SSL_get_cipher(ssl)` (API §9). In Wireshark, read the
ServerHello.

**Why:** Verifying real negotiated parameters vs what you *think* is configured.

**Trap:** Trusting config files over the actual handshake result.

---

### Q29. A cert works on desktop browsers but fails on an older Android device / IoT client. Likely cause?

**Answer:** Usually a **missing intermediate** (Q7) or a **trust-store difference** — the older
device doesn't have the newer root/intermediate CA, or requires the full chain because it won't
fetch intermediates (§11). Also possible: the device rejects a modern-only cipher/version.

**Why:** "Works in my browser" is not "works everywhere"; non-browser clients are stricter.

**Trap:** Assuming the cert is universally fine because Chrome accepts it.

---

### Q30. What's the fastest way to audit a server's TLS posture without writing code?

**Answer:** **SSL Labs** (`ssllabs.com/ssltest`, public sites) or **`testssl.sh`** (local/internal).
Both grade protocol versions, cipher suites, cert chain, forward secrecy, and known-vuln exposure
(Learning Phase 4).

**Why:** Knowing the standard tooling saves hours.

**Trap:** Hand-checking each suite when a scanner does it comprehensively.

---

## F. Embedded / DTLS / FreeRTOS

### Q31. Why does DTLS exist instead of "just running TLS over UDP"?

**Answer:** TLS assumes a **reliable, ordered** stream (TCP). UDP loses and reorders packets, which
would break TLS's record sequencing and handshake. **DTLS** adapts TLS for datagrams: explicit
sequence numbers, retransmission timers for handshake messages, and tolerance of loss/reordering —
so you get TLS security over UDP (Learning Phase 6, Concepts §14).

**Why:** Understanding the transport assumption TLS makes (§0/§1) and why UDP violates it.

**Trap:** "TLS runs on any transport." It needs reliability; DTLS is the adaptation.

---

### Q32. On an MCU under FreeRTOS the mbedTLS handshake hangs or resets. Two most likely causes?

**Answer:** (1) **Heap/stack exhaustion** — the handshake allocates sizable buffers and does
public-key math; the task stack or `configTOTAL_HEAP_SIZE` is too small. (2) **Bad RNG seeding** —
`mbedtls_ctr_drbg_seed` fed weak entropy, or a socket callback (`mbedtls_ssl_set_bio`) mis-handling
non-blocking `WANT_READ/WANT_WRITE` (API §10).

**Why:** Ties the abstract handshake cost to concrete embedded constraints.

**Trap:** Debugging TLS config when the real issue is RAM budget or entropy.

---

### Q33. Why is good randomness (CSPRNG) more critical on an MCU than almost anywhere else, and how do you get it?

**Answer:** TLS security collapses if the "random" ephemeral keys and nonces are predictable (§4,
§6, §8). MCUs often lack rich entropy sources at boot, so a naive seed can be guessable. Use a
**hardware RNG** peripheral (TRNG) to seed `mbedtls_ctr_drbg_seed`; never seed from a constant, a
timer alone, or the device serial (API §10 FreeRTOS notes).

**Why:** Entropy is the silent single point of failure in embedded TLS.

**Trap:** Assuming any "random-looking" seed is fine.

---

### Q34. Why prefer ChaCha20-Poly1305 over AES-GCM on some embedded targets?

**Answer:** **ChaCha20-Poly1305** is fast and constant-time **in software**, so on MCUs **without
AES hardware acceleration** it outperforms AES-GCM and avoids cache-timing pitfalls (§8). On chips
*with* AES hardware, AES-GCM is usually faster.

**Why:** Cipher choice is hardware-dependent, not "always pick AES."

**Trap:** Treating AES as universally fastest regardless of the platform.

---

### Q35. How would you fit a TLS session into a tight FreeRTOS memory budget?

**Answer:** Size the **task stack** for the handshake's deepest call chain (verify with high-water
marks); ensure **heap** covers per-connection TLS state + record buffers; consider **max fragment
length** / smaller record buffers; reuse a single connection (avoid repeated full handshakes); use
**session resumption** (§16) to skip the costly public-key work on reconnect; pick an EC key (smaller
than RSA). (API §10, Learning Phase 6.)

**Why:** Real embedded engineering — balancing security against KB of RAM.

**Trap:** Ignoring that one TLS connection can dwarf the rest of the app's RAM.

---

### Q36. A battery IoT device reconnects every few minutes. How do you cut the TLS cost per wake?

**Answer:** **Session resumption** via tickets/PSK (§16): the first connect does a full handshake;
subsequent wakes resume with the cached secret, skipping certificates and most public-key math —
big energy/latency savings. Optionally 0-RTT for idempotent sends (mind replay, Q2).

**Why:** Applies resumption to a concrete power-constrained scenario.

**Trap:** Doing a full handshake every wake and draining the battery.

---

## G. Design & Architecture

### Q37. When would you use mutual TLS (mTLS) instead of server-only TLS?

**Answer:** When the **server must authenticate the client** cryptographically — service-to-service
auth, IoT devices proving identity to a backend, zero-trust internal networks. Both sides present
certs; the server sets `SSL_VERIFY_FAIL_IF_NO_PEER_CERT` (API §8). It replaces/augments passwords or
API keys with certificate identity.

**Why:** Knowing the *use case* for client certs, not just that they exist.

**Trap:** Using mTLS for public consumer sites (cert distribution to end users is impractical).

---

### Q38. Where should TLS terminate — at a load balancer or at the app server? Trade-offs?

**Answer:** **Terminate at the LB/reverse proxy** for central cert management, offloaded crypto, and
easier scaling — but traffic behind the LB is plaintext unless you re-encrypt. **Terminate at the
app** (or end-to-end / re-encrypt to the backend) when internal traffic must stay encrypted
(compliance, zero-trust). Many use **TLS passthrough** or **re-encryption** for sensitive backends.

**Why:** A real architecture decision balancing manageability vs internal confidentiality.

**Trap:** Assuming "TLS at the edge" means the whole path is encrypted.

---

### Q39. What is HSTS and what attack does it close?

**Answer:** **HTTP Strict Transport Security** — a response header telling browsers to *only* use
HTTPS for this domain for a set period. It closes **SSL-stripping** (an MITM downgrading the initial
`http://` request before the redirect to HTTPS). With HSTS (and preloading) the browser refuses
plaintext from the start (Learning Phase 4).

**Why:** Understanding that the *first* plaintext request is a real attack window.

**Trap:** Thinking a redirect from HTTP→HTTPS is sufficient; the initial request is still
interceptable without HSTS.

---

### Q40. Certificate pinning: what is it, and what's the danger?

**Answer:** **Pinning** hardcodes the expected server cert/public key (or CA) in the client, so even
a valid-but-unexpected CA-issued cert is rejected — strong defense against a rogue/compromised CA
(Learning Phase 4). **Danger:** if you rotate keys/CAs without shipping a client update, you **brick**
connectivity. Mitigate with backup pins and careful rotation.

**Why:** A powerful but operationally risky control; tests judgment about maintainability.

**Trap:** Pinning a single leaf cert with no backup — one rotation and every client breaks.

---

### Q41. How does HTTP/3 change the TLS picture?

**Answer:** HTTP/3 runs over **QUIC**, which builds **TLS 1.3 directly into the transport** (over
UDP) rather than layering TLS on top of TCP (Learning Phase 6). The handshake and transport are
integrated, cutting round trips and eliminating head-of-line blocking; TLS 1.3 is mandatory (no
plaintext QUIC).

**Why:** Awareness that TLS is moving *into* the transport, not sitting above it.

**Trap:** Thinking HTTP/3 is "HTTP/2 over TLS"; it's a different transport (QUIC/UDP).

---

### Q42. Post-quantum: what's the concern for TLS and what's the transitional fix?

**Answer:** A future quantum computer could break the **discrete-log/RSA** assumptions behind ECDHE
and RSA (§4–§5), and adversaries may **record now, decrypt later**. The transitional fix is **hybrid
key exchange** — combine a classical ECDHE with a post-quantum KEM (**ML-KEM / Kyber**) so the
session is safe if *either* holds (Learning Phase 6).

**Why:** Forward-looking awareness; "harvest now, decrypt later" makes it urgent even pre-quantum.

**Trap:** "Quantum is decades away, ignore it" — recorded traffic today is the exposure.

---

## Quick Decision Cheat Sheet

| Situation | Reach for | Concept |
|---|---|---|
| Need forward secrecy | **ECDHE** (ephemeral) | §15 |
| Confidentiality + integrity in one | **AEAD** (AES-GCM / ChaCha20-Poly1305) | §8 |
| Prove server identity | **Certificate + CA chain** | §10–§11 |
| Prove client identity too | **mTLS** (client cert) | Q37 |
| Cut reconnect cost | **Session resumption / PSK** | §16 |
| Fastest 1st request (idempotent) | **0-RTT** (mind replay) | §16, Q2 |
| TLS over UDP / IoT | **DTLS** | Q31 |
| MCU without AES HW | **ChaCha20-Poly1305** | Q34 |
| Rogue-CA defense | **Pinning** (+ backup pins) | Q40 |
| Force HTTPS-only | **HSTS** (+ preload) | Q39 |
| Central cert mgmt | **TLS termination at LB** (re-encrypt if needed) | Q38 |
| Debug a live handshake | `openssl s_client` + **Wireshark** | API §4 |

**The recurring trap across all of these:** confusing **encryption** (confidentiality) with
**identity** (who you're talking to). Most TLS mistakes — `-k`, missing hostname checks, "self-signed
is insecure", "encryption stops MITM" — come from collapsing those two. Keep them separate (Concepts
§1) and the scenarios answer themselves.

---

*Scenarios file created 2026-07-05. Completes the set with `TLS_SSL_Learning.md` (roadmap),
`TLS_SSL_Concepts.md` (theory), and `TLS_SSL_API.md` (tooling).*
