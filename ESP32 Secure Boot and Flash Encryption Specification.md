# ESP32 Secure Boot V2 and Flash Encryption Architecture

**Version:** 1.1
**Status:** Technical Specification

This document outlines the security architecture for ESP32 devices using Secure Boot V2 and Flash Encryption. It covers two provisioning workflows — **Self-Provisioning** (the device generates and burns its own keys autonomously on first boot) and **HSM-Based Signing** (keys managed externally via Azure Key Vault or a hardware HSM) — to ensure device integrity and data confidentiality.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Key Differentiation and Origins](#2-key-differentiation-and-origins)
3. [Flash Encryption — The Privacy Guard](#3-flash-encryption--the-privacy-guard)
4. [Secure Boot V2 — The Trust Guard](#4-secure-boot-v2--the-trust-guard)
5. [Self-Provisioning Flow (First Boot)](#5-self-provisioning-flow-first-boot)
6. [HSM-Based Signing Flow](#6-hsm-based-signing-flow)
7. [Security Considerations and Risks](#7-security-considerations-and-risks)

---

## 1. Executive Summary

The security of the ESP32 platform rests on two independent but complementary pillars:

- **Secure Boot V2:** Ensures that only cryptographically signed (authorized) firmware is executed. Provides **integrity and authenticity**.
- **Flash Encryption:** Ensures that firmware and sensitive data stored in external SPI flash remain confidential and cannot be physically extracted. Provides **confidentiality**.

These two features can be enabled independently but are always deployed together in a production-grade security posture. Neither feature alone is sufficient.

---

## 2. Key Differentiation and Origins

Understanding where keys originate and how they are stored is critical for maintaining a correct **Chain of Trust**.

| Feature | Flash Encryption Key | Secure Boot V2 Key |
|:--------|:---------------------|:-------------------|
| **Objective** | Confidentiality (Privacy) | Integrity (Authenticity) |
| **Algorithm** | Symmetric (AES-256-XTS) | Asymmetric (RSA-3072 + SHA-256) |
| **Origin — Self-Provisioning** | Internal TRNG on first boot | Local RSA key pair; hash burned on first boot |
| **Origin — HSM Flow** | Internal TRNG on first boot | Azure Key Vault / HSM; hash burned on first boot |
| **Storage on Chip** | Full AES-256 key in read-protected eFuse block | SHA-256 digest of the Public Key in eFuse |
| **Exportable?** | Never — hardware-bound | Public key is exportable; private key must not be |

> **Key insight:** The Flash Encryption AES key is **always self-generated** by the chip via TRNG — it never originates externally. The Secure Boot signing key can be either a local RSA key pair (Self-Provisioning flow) or one managed by an HSM (production flow). The chip stores only the **hash of the public key**, never the private key.

---

## 3. Flash Encryption — The Privacy Guard

### 3.1 Purpose

Flash Encryption protects the **confidentiality** of data stored on the external SPI flash chip. Without it, an attacker with physical access can read all flash contents directly using a cheap programmer (e.g., a CH341A), including firmware, Wi-Fi credentials, and private certificates.

### 3.2 How It Works — AES-256-XTS

The ESP32 uses **AES-256-XTS** (a tweakable block cipher mode designed for storage encryption). The tweak value is derived from the flash byte address, meaning each 16-byte sector is encrypted differently based on its physical location — making bulk plaintext attacks significantly harder.

- Encryption and decryption happen entirely in the **hardware AES accelerator** — the CPU never processes plaintext during normal operation.
- **What gets encrypted by default:**
  - Second-stage bootloader
  - Partition table
  - All `type=app` application partitions
- **What is NOT encrypted by default:**
  - NVS (Non-Volatile Storage) partitions — must be explicitly configured (see Section 7)

### 3.3 Key Generation and eFuse Burning

In both the Self-Provisioning and HSM flows, the Flash Encryption key is **always generated on-chip**:

1. The ESP32's built-in **TRNG** generates a 256-bit (32-byte) random key during the first boot.
2. The key is written into an eFuse key block (e.g., `BLOCK_KEY0`), and the key purpose register is set to `XTS_AES_256_KEY`.
3. The eFuse **read-protection bit** is set immediately — the CPU can no longer read the key value back.
4. Only the hardware AES accelerator retains internal access to the key material.

> **Critical:** This key is unique per chip and **cannot be backed up**. If the silicon is physically destroyed, all encrypted data is permanently unrecoverable.

### 3.4 Development Mode vs. Release Mode

Flash Encryption has two operating modes controlled by sdkconfig:

| Mode | UART Download Mode | JTAG | Suitable For |
|:-----|:-------------------|:-----|:-------------|
| **Development** | Remains enabled | Remains enabled | Debug and test cycles; allows re-flashing |
| **Release** | Permanently disabled | Permanently disabled | Production hardware |

> Always deploy production hardware in **Release mode**. Development mode leaves UART and JTAG open, which allows a physical attacker to dump RAM or re-flash the device.

### 3.5 Operational Impact — Flash is Chip-Bound

Because every chip generates its own unique AES key, the encrypted flash contents are **physically bound** to that specific processor die. Moving the SPI flash chip to a different ESP32 will result in an AES decryption failure and an immediate boot crash. This is a feature, not a bug — it prevents chip cloning.

---

## 4. Secure Boot V2 — The Trust Guard

### 4.1 Purpose

Secure Boot V2 ensures **code integrity and authenticity** — only firmware signed with the trusted private RSA key will be executed. It prevents an attacker from booting modified, unsigned, or injected firmware even if they have physical flash access.

### 4.2 Cryptographic Scheme

ESP32 Secure Boot V2 uses **RSA-3072 with PKCS#1 v1.5 padding** and **SHA-256** as the image digest algorithm.

- The **private key** signs the firmware binary at build time (offline).
- The **SHA-256 digest of the corresponding public key** is burned into eFuse — not the public key itself. The ROM bootloader re-computes this digest at runtime to validate the key presented in the signature block.

> Secure Boot V2 supports up to **3 independent key slots** in eFuse, enabling key rotation without re-provisioning devices already in the field.

### 4.3 Boot-Time Verification Chain

At every power-on reset, the following two-stage verification chain executes:

```
ROM Bootloader (immutable, embedded in silicon)
    │
    ├─ Loads 2nd-stage bootloader image from flash
    ├─ Reads RSA signature block appended to bootloader image
    ├─ Extracts Public Key from the signature block
    ├─ SHA-256 hashes the Public Key
    ├─ Compares hash against eFuse-stored digest
    │       Match → continue     Mismatch → HALT (boot aborted)
    └─ Verifies RSA-3072 signature of the bootloader binary
            Valid → load 2nd-stage bootloader     Invalid → HALT

2nd-Stage Bootloader
    └─ Applies the same verification process to the application image
            Valid → execute app     Invalid → HALT
```

> If Secure Boot is armed (`ABS_DONE_0` eFuse bit is set) and verification fails at any stage, the chip halts permanently for that boot attempt. It will not fall back to an unsigned image.

### 4.4 Self-Provisioning vs. HSM-Based Signing — At a Glance

| Aspect | Self-Provisioning | HSM-Based (Production) |
|:-------|:------------------|:-----------------------|
| **Private key storage** | Local `.pem` file on developer machine | Azure Key Vault / hardware HSM (non-exportable) |
| **Signing location** | `idf.py build` on local machine | CI/CD pipeline via HSM signing API |
| **Key exposure risk** | High — key can be copied, committed to git | Eliminated — key never leaves the vault |
| **Suitable for** | Development, prototypes, small volumes | Production deployments, large fleets |

---

## 5. Self-Provisioning Flow (First Boot)

Self-Provisioning is the process by which a blank ESP32 transitions autonomously from an **unsecured state** to a **fully locked, cryptographically secured state** in a single boot cycle — without any external intervention after the firmware is flashed.

### 5.1 Pre-Conditions — sdkconfig Settings

Before flashing, the following Kconfig options must be enabled in the project configuration:

```
# Secure Boot V2
CONFIG_SECURE_BOOT=y
CONFIG_SECURE_BOOT_V2_ENABLED=y
CONFIG_SECURE_BOOT_SIGNING_KEY="secure_boot_signing_key.pem"   # RSA-3072 local key

# Flash Encryption
CONFIG_SECURE_FLASH_ENC_ENABLED=y
CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y                  # use DEVELOPMENT for testing
```

The RSA key pair can be generated with:
```bash
espsecure.py generate_signing_key --version 2 secure_boot_signing_key.pem
```

The build system (`idf.py build`) automatically signs the firmware binary using this key before flashing.

### 5.2 What Is Flashed Initially

The programmer loads the following onto the **blank, unencrypted chip**:

| Flash Region | Content at This Stage |
|:-------------|:----------------------|
| Bootloader offset (`0x0`) | Plaintext, RSA-signed 2nd-stage bootloader |
| Partition table offset (`0x8000`) | Plaintext partition table |
| App partition | Plaintext, RSA-signed application image |

The firmware is **plaintext but signed** — the RSA signature blocks are embedded. Encryption has not happened yet.

### 5.3 Step-by-Step First Boot Sequence

The sequence below is executed entirely by the **2nd-stage bootloader** on the very first power-on:

```
┌──────────────────────────────────────────────────────────────────────┐
│                   FIRST BOOT — BLANK CHIP                            │
│           (eFuses unprogrammed, flash contents are plaintext)        │
└─────────────────────────────┬────────────────────────────────────────┘
                              │
                              ▼
               ┌──────────────────────────┐
               │   ROM Bootloader Starts  │
               │  (no eFuse checks yet —  │
               │   Secure Boot not armed) │
               └─────────────┬────────────┘
                             │ Loads 2nd-stage bootloader (plaintext)
                             ▼
               ┌──────────────────────────┐
               │  2nd-Stage Bootloader    │
               │  Detects: eFuses blank;  │
               │  security config present │
               └─────────────┬────────────┘
                             │
          ┌──────────────────┼──────────────────────┐
          ▼                  ▼                       ▼
   [STEP 1]            [STEP 2]                [STEP 3]
   Read own RSA        Generate AES-256        Write keys to eFuse:
   signature block     key via TRNG            - AES key → BLOCK_KEY0
   → extract           (32 random bytes,         (read-protected)
   Public Key          unique per chip)        - SHA-256(PublicKey)
   → compute                                     → Secure Boot digest
   SHA-256 digest                                slot in eFuse
                             │
                             ▼
                      [STEP 4]
               Encrypt flash in-place using
               the new AES-256 key:
               - Bootloader image
               - Partition table
               - Application partition(s)
               (original plaintext is overwritten)
                             │
                             ▼
                      [STEP 5]
               Burn permanent eFuse lock bits:
               - ABS_DONE_0      → Secure Boot V2 armed
               - JTAG_DISABLE    → Debug port permanently off
               - DIS_DOWNLOAD_MODE → UART download disabled
                 (Release mode only)
                             │
                             ▼
               ┌──────────────────────────┐
               │   REBOOT — Chip is now   │
               │   fully secured.         │
               │   Every future boot runs │
               │   the full verification  │
               │   chain (Section 4.3).   │
               └──────────────────────────┘
```

### 5.4 Key eFuse Bits and Their Effects

| eFuse Bit | Set During Provisioning? | Effect When Set |
|:----------|:------------------------|:----------------|
| `ABS_DONE_0` | Yes | Enables Secure Boot V2; ROM now enforces RSA signature verification on every boot |
| `JTAG_DISABLE` | Yes | Permanently disables the JTAG hardware debug interface |
| `DIS_DOWNLOAD_MODE` | Yes (Release mode) | Permanently disables UART serial download; device can no longer be re-flashed via esptool |
| `DIS_DOWNLOAD_ICACHE` | Yes | Disables instruction cache access during any UART download mode session |
| `KEY_PURPOSE_0 = XTS_AES_256_KEY` | Yes | Marks the eFuse key block as the Flash Encryption key and activates read-protection |

### 5.5 Post-Provisioning State and Constraints

After the first boot completes and the device reboots into the secured state:

- All flash contents are now **AES-256 encrypted** and tied to this chip's unique key.
- Every boot validates the full Secure Boot chain — unsigned or tampered firmware causes an immediate halt.
- All subsequent OTA (Over-The-Air) firmware updates **must be signed** with the same RSA private key (or a key whose digest occupies a valid eFuse slot).
- **eFuse burns are irreversible** — the security configuration cannot be undone without physically destroying the chip.
- In Release mode, the only way to update firmware is via a valid signed OTA update.

---

## 6. HSM-Based Signing Flow

For production fleets, the RSA private signing key must never reside on a developer machine or build server. Instead, it is stored in a hardware security module (HSM) or cloud key vault, and the build pipeline communicates with it remotely.

### 6.1 Key Setup (One Time)

1. Generate an RSA-3072 key pair **inside Azure Key Vault** (flagged as non-exportable — the private key cannot be downloaded).
2. Export the **public key** in `.pem` format.
3. Compute `SHA-256(public_key)` — this digest value will be burned into device eFuses at first boot (same as Self-Provisioning, but the key lives in the vault rather than a local file).

### 6.2 Build and Signing Process

```
CI/CD Pipeline or Developer Machine
    │
    ├─ idf.py build → produces firmware binary + SHA-256 image hash
    ├─ POST hash to Azure Key Vault Signing API
    ├─ Key Vault signs the hash using the stored RSA-3072 private key
    ├─ API returns RSA signature bytes
    └─ espsecure.py attaches signature block to firmware binary
           → Output: production-ready signed binary (still plaintext)
```

### 6.3 Factory Provisioning

On the production line:
1. Flash the **signed plaintext binary** onto the blank chip using a programmer.
2. Power on the chip — the **Self-Provisioning flow (Section 5) runs automatically**.
3. The chip reads the public key from the binary's embedded signature block, burns its SHA-256 digest to eFuse, generates its own AES Flash Encryption key, encrypts flash, and locks eFuses.

> **Security note:** There is a brief window between step 1 (programmer writes plaintext flash) and step 2 (first boot encrypts it) where an intercepted device could have its firmware read back in plaintext. Mitigate this by triggering first boot immediately in a controlled, air-gapped programming jig.

---

## 7. Security Considerations and Risks

| Risk | Description | Mitigation |
|:-----|:------------|:-----------|
| **Hardware Destruction ("Brick")** | Flash Encryption AES key exists only in eFuse silicon. Physical chip destruction makes all encrypted data permanently unrecoverable. | Accept as a design property. Do not store irreplaceable data solely on-device without a server-side backup. |
| **Provisioning Window** | Between factory flashing and first boot completion, firmware sits unencrypted on flash. An intercepted device could have its contents read. | Use air-gapped, controlled programming jigs that immediately trigger first boot after flashing. |
| **Signing Key Loss** | Losing the RSA private key (or access to the HSM) means the device fleet can never receive valid OTA updates. eFuse key digest slots cannot be changed once burned. | Securely back up private keys offline (or use a non-exportable HSM key with access policy backups). Use Secure Boot V2 key slots 1 and 2 for planned key rotation. |
| **Development Key Exposure** | Developers may accidentally commit `.pem` signing key files to version control, compromising the entire fleet. | Use `.gitignore` rules; prefer HSM for all signing; rotate the key immediately if exposed. |
| **JTAG / UART Left Open** | Development mode leaves JTAG and UART download enabled. A physical attacker can read RAM contents or re-flash the device. | Always use **Release mode** (`CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y`) for production hardware. |
| **NVS Not Encrypted by Default** | NVS partitions storing Wi-Fi credentials, tokens, and certificates are **not** covered by Flash Encryption unless explicitly configured. | Enable `CONFIG_NVS_ENCRYPTION=y` and provision a dedicated encrypted NVS key partition. |