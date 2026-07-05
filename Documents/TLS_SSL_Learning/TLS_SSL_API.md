# TLS / SSL — API & Tooling Reference

> The **practical** companion to `TLS_SSL_Concepts.md` (the theory) and `TLS_SSL_Learning.md`
> (the roadmap). Here are the actual commands and functions: **OpenSSL CLI**, the **OpenSSL C
> library**, and **mbedTLS / wolfSSL** for embedded (FreeRTOS-relevant).
>
> Each entry gives **Purpose · Signature/Usage · Key params · Example · Notes/Alternatives**.
> Concepts are explained in the theory file — here we just *drive the machine*.

---

## Table of Contents

0. [Mental model: where each API sits](#0-mental-model-where-each-api-sits)
1. [OpenSSL CLI — keys](#1-openssl-cli--keys)
2. [OpenSSL CLI — certificates & CSRs](#2-openssl-cli--certificates--csrs)
3. [OpenSSL CLI — inspecting & converting](#3-openssl-cli--inspecting--converting)
4. [OpenSSL CLI — debugging live connections](#4-openssl-cli--debugging-live-connections)
5. [OpenSSL CLI — run a test server](#5-openssl-cli--run-a-test-server)
6. [Build your own CA (walkthrough)](#6-build-your-own-ca-walkthrough)
7. [OpenSSL C library — client](#7-openssl-c-library--client)
8. [OpenSSL C library — server](#8-openssl-c-library--server)
9. [OpenSSL C library — verification & options](#9-openssl-c-library--verification--options)
10. [mbedTLS — embedded client (FreeRTOS)](#10-mbedtls--embedded-client-freertos)
11. [wolfSSL — quick reference](#11-wolfssl--quick-reference)
12. [Common errors & gotchas](#12-common-errors--gotchas)
13. [Cheat-sheet appendix](#13-cheat-sheet-appendix)

---

## 0. Mental model: where each API sits

| Layer | Tool/API | Use it for |
|---|---|---|
| Command line | **`openssl`** | Generate keys/certs, inspect, debug handshakes, quick servers |
| Desktop/server C | **OpenSSL libssl/libcrypto** | Add TLS to a Linux/server app |
| Embedded C | **mbedTLS**, **wolfSSL**, BearSSL | TLS on an MCU / FreeRTOS (small footprint, HW crypto) |

Every library follows the same **shape**: create a context → configure trust & options →
wrap a socket → handshake → read/write → shutdown → free. Learn that arc once; the function
names differ, the sequence doesn't.

```
  CONTEXT (long-lived, shared)
     └── CONNECTION (per socket)
            ├── set fd / BIO
            ├── HANDSHAKE   ← all of TLS_SSL_Concepts §12 happens here
            ├── read / write (AEAD records, §14)
            └── shutdown → free
```

---

## 1. OpenSSL CLI — keys

### `openssl genrsa` / `genpkey` — generate a private key
**Purpose:** create the private key a certificate will be built around.
```bash
# RSA 2048 (legacy-friendly)
openssl genrsa -out server.key 2048

# Modern, preferred: generic genpkey
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out server.key

# EC key (smaller/faster — good for embedded)
openssl ecparam -name prime256v1 -genkey -noout -out server-ec.key
# or:
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out server-ec.key

# Ed25519 (modern signature key)
openssl genpkey -algorithm ED25519 -out server-ed.key
```
**Key params:** bits (RSA ≥2048), curve (`prime256v1`=P-256, `secp384r1`=P-384).
**Notes:** `genpkey` is the modern unified command; `genrsa`/`ecparam` are older but everywhere.
Protect the file (`chmod 600`); anyone with it can impersonate you.

### `openssl rsa` / `pkey` — inspect / transform a key
```bash
openssl pkey -in server.key -text -noout        # show key details
openssl pkey -in server.key -pubout -out server.pub   # extract public key
openssl rsa  -in server.key -check              # sanity-check an RSA key
```

---

## 2. OpenSSL CLI — certificates & CSRs

### `openssl req` — create a CSR or a self-signed cert
**Purpose:** a **CSR** (Certificate Signing Request) asks a CA to issue a cert; `-x509` skips the CA
and self-signs (for testing / your own root).
```bash
# CSR (to send to a CA)
openssl req -new -key server.key -out server.csr \
  -subj "/C=NP/O=Learning/CN=example.local"

# Self-signed cert directly (test only)
openssl req -x509 -new -key server.key -days 365 -out server.crt \
  -subj "/CN=example.local" \
  -addext "subjectAltName=DNS:example.local,DNS:localhost,IP:127.0.0.1"
```
**Key params:** `-subj` (identity), `-addext subjectAltName=...` (**required** — modern clients
ignore CN and match only SAN), `-days`, `-x509` (self-sign vs CSR).
**Gotcha:** forgetting SAN is the #1 reason a hand-made cert is rejected with a hostname error.

### `openssl x509` — sign a CSR / manipulate a cert
```bash
# CA signs a CSR into a cert
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -days 365 -out server.crt \
  -extfile <(printf "subjectAltName=DNS:example.local")
```

---

## 3. OpenSSL CLI — inspecting & converting

### `openssl x509 -text` — read a certificate
**Purpose:** the single most useful inspection command — dumps every field (§10 of concepts).
```bash
openssl x509 -in server.crt -text -noout
openssl x509 -in server.crt -noout -dates          # just validity window
openssl x509 -in server.crt -noout -subject -issuer -ext subjectAltName
openssl x509 -in server.crt -noout -fingerprint -sha256
```

### `openssl verify` — validate a chain
```bash
openssl verify -CAfile ca.crt server.crt
openssl verify -CAfile root.crt -untrusted intermediate.crt leaf.crt
```

### Format conversions (PEM ⇄ DER ⇄ PKCS#12)
| From → To | Command |
|---|---|
| PEM → DER | `openssl x509 -in c.pem -outform DER -out c.der` |
| DER → PEM | `openssl x509 -inform DER -in c.der -out c.pem` |
| PEM → PKCS#12 (bundle key+cert for Windows/Java) | `openssl pkcs12 -export -inkey k.pem -in c.pem -certfile ca.pem -out bundle.p12` |
| PKCS#12 → PEM | `openssl pkcs12 -in bundle.p12 -nodes -out all.pem` |

**Notes:** **PEM** = Base64 text (`-----BEGIN...`); **DER** = raw binary; **PKCS#12/.p12/.pfx** =
password-protected bundle of key + cert(s). Concept: `TLS_SSL_Concepts.md` §10.

---

## 4. OpenSSL CLI — debugging live connections

### `openssl s_client` — the handshake debugger
**Purpose:** open a real TLS connection and print every detail — your primary field tool.
```bash
# Basic: connect and show the chain
openssl s_client -connect example.com:443 -servername example.com

# Show full certificate chain the server sends
openssl s_client -connect example.com:443 -showcerts

# Force a protocol version (test negotiation / downgrade)
openssl s_client -connect example.com:443 -tls1_2
openssl s_client -connect example.com:443 -tls1_3

# Check a specific SNI virtual host
openssl s_client -connect 1.2.3.4:443 -servername secure.site.com

# See negotiated cipher, protocol, and verify result quickly
openssl s_client -connect example.com:443 </dev/null 2>/dev/null \
  | openssl x509 -noout -subject -dates
```
**Key params:** `-connect host:port`, `-servername` (**SNI** — send it or you may get the wrong
cert), `-showcerts`, `-tls1_2`/`-tls1_3`, `-CAfile` (verify against a specific root),
`-cert`/`-key` (present a **client** cert for mTLS).
**Read the output for:** `Verify return code:` (0 = OK), `Protocol :`, `Cipher :`, and the
`Certificate chain` block. Type an HTTP request after it connects to talk to the server.

### `openssl ciphers` — list/verify cipher suites
```bash
openssl ciphers -v 'ECDHE+AESGCM'        # what does this selection expand to?
openssl ciphers -s -tls1_3               # TLS 1.3 suites
```

---

## 5. OpenSSL CLI — run a test server

### `openssl s_server` — throwaway TLS server
**Purpose:** stand up a TLS endpoint in one line to test clients / captures.
```bash
openssl s_server -accept 4433 -cert server.crt -key server.key -www
# then: openssl s_client -connect localhost:4433
#   or: curl -k https://localhost:4433

# Require a client certificate (test mTLS)
openssl s_server -accept 4433 -cert server.crt -key server.key \
  -CAfile ca.crt -Verify 1
```
**Key params:** `-accept port`, `-cert`/`-key`, `-www` (serve a status page), `-Verify n`
(require & verify a client cert to depth n → **mTLS**).

---

## 6. Build your own CA (walkthrough)

The end-to-end exercise that makes Concepts §11 tangible.
```bash
# 1) Root CA key + self-signed root cert
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:4096 -out ca.key
openssl req -x509 -new -key ca.key -days 3650 -out ca.crt \
  -subj "/CN=My Test Root CA"

# 2) Server key + CSR
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out server.key
openssl req -new -key server.key -out server.csr -subj "/CN=example.local"

# 3) CA signs the CSR (with SAN)
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -days 365 -out server.crt \
  -extfile <(printf "subjectAltName=DNS:example.local,DNS:localhost")

# 4) Verify the chain
openssl verify -CAfile ca.crt server.crt      # → server.crt: OK

# 5) Trust the root on your machine (macOS example)
sudo security add-trusted-cert -d -r trustRoot \
  -k /Library/Keychains/System.keychain ca.crt
```
Now serve `server.crt`/`server.key` and your browser trusts it — because it chains to a root you
added. Delete the root from the keychain when done.

---

## 7. OpenSSL C library — client

Minimal TLS client arc (OpenSSL 3.x). Concept map in brackets.
```c
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());        // [context]
SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);      // refuse old TLS
SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);          // [verify §10-11]
SSL_CTX_set_default_verify_paths(ctx);                   // use system root store
// or a specific CA:  SSL_CTX_load_verify_locations(ctx, "ca.crt", NULL);

int fd = /* connect a TCP socket to host:443 yourself */;
SSL *ssl = SSL_new(ctx);                                 // [connection]
SSL_set_fd(ssl, fd);
SSL_set_tlsext_host_name(ssl, "example.com");            // [SNI]  — REQUIRED
SSL_set1_host(ssl, "example.com");                       // [hostname check] — REQUIRED

if (SSL_connect(ssl) != 1) {                             // [handshake §12]
    ERR_print_errors_fp(stderr);                         // read the real reason
}
long v = SSL_get_verify_result(ssl);                     // X509_V_OK == success

SSL_write(ssl, "GET / HTTP/1.0\r\n\r\n", 18);            // [AEAD records §14]
char buf[4096]; int n = SSL_read(ssl, buf, sizeof buf);

SSL_shutdown(ssl);
SSL_free(ssl);
SSL_CTX_free(ctx);
```

| Function | Role |
|---|---|
| `SSL_CTX_new(TLS_client_method())` | Create shared context (reuse across connections) |
| `SSL_CTX_set_verify(..., SSL_VERIFY_PEER, ...)` | **Turn on** certificate verification |
| `SSL_CTX_set_default_verify_paths` / `load_verify_locations` | Choose trust anchors |
| `SSL_set_tlsext_host_name` | Send **SNI** (else server may serve wrong cert) |
| `SSL_set1_host` | Enable **hostname/SAN matching** (silent MITM hole if omitted) |
| `SSL_connect` | Perform the handshake |
| `SSL_read` / `SSL_write` | Encrypted I/O |
| `SSL_shutdown` / `SSL_free` / `SSL_CTX_free` | Clean teardown |

**Critical gotcha:** `SSL_VERIFY_PEER` alone does **not** check the hostname — you must also call
`SSL_set1_host` (or `X509_VERIFY_PARAM_set1_host`). Skipping it is a classic, silent vulnerability.

---

## 8. OpenSSL C library — server

```c
SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
SSL_CTX_use_certificate_chain_file(ctx, "server.crt"); // leaf + intermediates
SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM);
SSL_CTX_check_private_key(ctx);                         // key matches cert?

// For mutual TLS (require client certs):
// SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
// SSL_CTX_load_verify_locations(ctx, "ca.crt", NULL);

int client_fd = /* accept() a TCP connection */;
SSL *ssl = SSL_new(ctx);
SSL_set_fd(ssl, client_fd);
if (SSL_accept(ssl) != 1)                               // server-side handshake
    ERR_print_errors_fp(stderr);
// SSL_read / SSL_write ... then shutdown/free as above
```
| Function | Role |
|---|---|
| `SSL_CTX_use_certificate_chain_file` | Load leaf **+ intermediate** chain to send |
| `SSL_CTX_use_PrivateKey_file` | Load the matching private key |
| `SSL_CTX_check_private_key` | Fail fast if key ≠ cert |
| `SSL_accept` | Server side of the handshake |
| `SSL_VERIFY_FAIL_IF_NO_PEER_CERT` | Enforce **mTLS** |

---

## 9. OpenSSL C library — verification & options

| Task | Call |
|---|---|
| Require a modern minimum version | `SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)` |
| Restrict cipher suites (≤1.2) | `SSL_CTX_set_cipher_list(ctx, "ECDHE+AESGCM")` |
| Restrict TLS 1.3 suites | `SSL_CTX_set_ciphersuites(ctx, "TLS_AES_256_GCM_SHA384")` |
| Set ALPN (e.g. offer h2/http1.1) | `SSL_CTX_set_alpn_protos(...)` |
| Get negotiated protocol/cipher | `SSL_get_version(ssl)`, `SSL_get_cipher(ssl)` |
| Inspect peer certificate | `SSL_get1_peer_certificate(ssl)` → `X509_*` accessors |
| Pin a hostname explicitly | `X509_VERIFY_PARAM_set1_host(param, host, len)` |
| Turn OpenSSL errors into text | `ERR_error_string` / `ERR_print_errors_fp` |

**Error handling pattern:** most calls return `1` on success. On failure, `SSL_get_error(ssl, ret)`
tells you *why* (`SSL_ERROR_WANT_READ/WRITE` for non-blocking sockets — retry; `SSL_ERROR_SSL` — a
protocol/cert failure, then drain `ERR_get_error`).

---

## 10. mbedTLS — embedded client (FreeRTOS)

**Why mbedTLS:** small footprint, no OS assumptions, hardware-crypto hooks — the common choice on
Cortex-M / ESP32 under FreeRTOS. Same arc as OpenSSL, different names. (Ties to `TLS_SSL_Learning.md`
Phase 6.)
```c
#include "mbedtls/ssl.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/x509_crt.h"

mbedtls_ssl_context      ssl;
mbedtls_ssl_config       conf;
mbedtls_ctr_drbg_context ctr_drbg;   // CSPRNG (Concepts §6)
mbedtls_entropy_context  entropy;
mbedtls_x509_crt         cacert;     // trust anchor

mbedtls_ssl_init(&ssl);
mbedtls_ssl_config_init(&conf);
mbedtls_ctr_drbg_init(&ctr_drbg);
mbedtls_entropy_init(&entropy);
mbedtls_x509_crt_init(&cacert);

// 1) Seed the RNG — TLS is only as strong as its randomness
mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                      (const unsigned char*)"tls-client", 10);

// 2) Load trusted CA cert(s)
mbedtls_x509_crt_parse(&cacert, ca_pem, ca_pem_len);

// 3) Default config for a TLS client over a stream
mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);  // verify server
mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

// 4) Bind config + hostname (SNI + name check) + transport callbacks
mbedtls_ssl_setup(&ssl, &conf);
mbedtls_ssl_set_hostname(&ssl, "example.com");                  // SNI + SAN check
mbedtls_ssl_set_bio(&ssl, &net_ctx, my_send, my_recv, NULL);    // your socket I/O

// 5) Handshake (loop while WANT_READ/WANT_WRITE on non-blocking sockets)
int r;
while ((r = mbedtls_ssl_handshake(&ssl)) != 0)
    if (r != MBEDTLS_ERR_SSL_WANT_READ && r != MBEDTLS_ERR_SSL_WANT_WRITE) break; // error

// 6) Confirm the certificate verdict
uint32_t flags = mbedtls_ssl_get_verify_result(&ssl);   // 0 == OK

// 7) I/O
mbedtls_ssl_write(&ssl, buf, len);
mbedtls_ssl_read(&ssl, buf, sizeof buf);

// 8) Teardown
mbedtls_ssl_close_notify(&ssl);
mbedtls_ssl_free(&ssl);
mbedtls_ssl_config_free(&conf);
mbedtls_ctr_drbg_free(&ctr_drbg);
mbedtls_entropy_free(&entropy);
mbedtls_x509_crt_free(&cacert);
```
| mbedTLS call | OpenSSL analogue | Role |
|---|---|---|
| `mbedtls_ctr_drbg_seed` | (internal) | Seed CSPRNG — **essential**, TLS dies without good entropy |
| `mbedtls_x509_crt_parse` | `load_verify_locations` | Load trust anchor |
| `mbedtls_ssl_config_defaults` | `SSL_CTX_new` + presets | Base config |
| `mbedtls_ssl_conf_authmode(VERIFY_REQUIRED)` | `SSL_VERIFY_PEER` | Enforce verification |
| `mbedtls_ssl_set_hostname` | `SSL_set_tlsext_host_name`+`SSL_set1_host` | SNI **and** name check (one call!) |
| `mbedtls_ssl_set_bio` | `SSL_set_fd` | Plug in your transport |
| `mbedtls_ssl_handshake` | `SSL_connect` | The handshake |
| `mbedtls_ssl_read/write` | `SSL_read/write` | Encrypted I/O |

**FreeRTOS notes:**
- The handshake needs a chunk of **stack** and **heap** — size the task stack generously and watch
  `configTOTAL_HEAP_SIZE`; a stalled handshake is often heap exhaustion.
- Provide `my_send`/`my_recv` as thin wrappers over your FreeRTOS+TCP / lwIP sockets.
- Seeding the RNG on an MCU is the hard part — use a **hardware RNG** peripheral for entropy; a
  weak seed silently breaks all security.
- For **DTLS** (UDP), use `MBEDTLS_SSL_TRANSPORT_DATAGRAM` and set the timer callbacks.

---

## 11. wolfSSL — quick reference

Another embedded stack; OpenSSL-like arc. Useful when you want its compatibility layer or FIPS.
```c
wolfSSL_Init();
WOLFSSL_CTX *ctx = wolfSSL_CTX_new(wolfTLSv1_3_client_method());
wolfSSL_CTX_load_verify_locations(ctx, "ca.crt", NULL);   // trust anchor

WOLFSSL *ssl = wolfSSL_new(ctx);
wolfSSL_set_fd(ssl, sockfd);
wolfSSL_check_domain_name(ssl, "example.com");            // hostname/SAN check
wolfSSL_connect(ssl);                                     // handshake

wolfSSL_write(ssl, msg, len);
wolfSSL_read(ssl, buf, sizeof buf);

wolfSSL_shutdown(ssl); wolfSSL_free(ssl);
wolfSSL_CTX_free(ctx); wolfSSL_Cleanup();
```
`wolfSSL_*` maps almost 1:1 to OpenSSL. It also ships a genuine OpenSSL compat header so a lot of
`SSL_*` code compiles unchanged.

---

## 12. Common errors & gotchas

| Symptom | Likely cause | Fix |
|---|---|---|
| `unable to get local issuer certificate` | Missing intermediate in chain, or CA not trusted | Server must send **leaf+intermediates**; client must trust the root |
| `certificate verify failed: hostname mismatch` | Cert SAN doesn't include the name you dialed | Add correct `subjectAltName`; send SNI |
| `self signed certificate` | Verifying a self-signed cert with no matching trust | Add it via `-CAfile` / `load_verify_locations` |
| `wrong version number` | Speaking TLS to a plaintext port (or vice-versa) | Check port / that the server actually does TLS |
| `no shared cipher` | Client & server suites/versions don't overlap | Align `min_proto_version` and cipher lists |
| `sslv3 alert handshake failure` | Version or cipher rejected, or missing client cert (mTLS) | Check version floor; present client cert |
| Handshake hangs on MCU | Heap/stack exhaustion or bad RNG seed | Grow task stack/heap; seed CSPRNG from HW RNG |
| Verify passes but MITM possible | `SSL_VERIFY_PEER` set but **no** hostname check | Add `SSL_set1_host` / `mbedtls_ssl_set_hostname` |

**Golden rule:** never disable verification to "make it work" (`curl -k`, `SSL_VERIFY_NONE`,
`InsecureSkipVerify`). That deletes the entire identity guarantee (Concepts §10) and turns TLS back
into "encrypted to *someone*." Fix the trust chain instead.

---

## 13. Cheat-sheet appendix

**Inspect anything, fast**
```bash
openssl x509  -in cert.pem  -text -noout          # a certificate
openssl req   -in req.csr   -text -noout          # a CSR
openssl pkey  -in key.pem   -text -noout          # a private key
openssl s_client -connect host:443 -servername host   # a live server
```

**One-liners you'll reuse**
```bash
# Expiry date of a live site
echo | openssl s_client -connect host:443 2>/dev/null | openssl x509 -noout -enddate

# Does this key match this cert? (the two hashes must be equal)
openssl pkey -in server.key -pubout -outform DER | openssl sha256
openssl x509 -in server.crt -pubkey -noout | openssl pkey -pubin -pubout -outform DER | openssl sha256

# What protocol/cipher did we negotiate?
openssl s_client -connect host:443 </dev/null 2>/dev/null | grep -E "Protocol|Cipher"
```

**The universal library arc (memorize once)**
```
ctx = new_context(method)
configure_trust(ctx)          # CA / verify mode
configure_options(ctx)        # min version, ciphers
conn = new_connection(ctx)
attach_socket(conn, fd)
set_hostname(conn, "host")    # SNI + SAN check — never skip
handshake(conn)               # ← Concepts §12
check_verify_result(conn)     # ← must be OK
read/write(conn)              # ← AEAD records, Concepts §14
shutdown(conn); free(conn); free(ctx)
```

---

*API reference created 2026-07-05. Pair with `TLS_SSL_Concepts.md` (theory) and
`TLS_SSL_Learning.md` (roadmap). Concept cross-references (§N) point into the theory file.*
