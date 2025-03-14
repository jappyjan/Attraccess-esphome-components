# DESFire Authentication Overview

This document provides an overview of the Mifare DESFire authentication process and specifications based on the article "DIY electronic RFID Door Lock with Battery Backup" from CodeProject. This focuses exclusively on the NFC authentication aspects, not the physical lock implementation.

## DESFire Card Versions

- **Mifare DESFire (Original)** - First generation cards using DES encryption (2002)

  - Vulnerable to side-channel attacks that can extract keys
  - Attack takes about 7 hours with specialized hardware
  - Uses legacy authentication not supported by many modern implementations

- **Mifare DESFire EV1** - Second generation cards (2009)

  - Improved security with no known attacks
  - Supports multiple cryptographic algorithms: DES, 2K3DES, 3K3DES, AES
  - Features application-based structure with separate security domains

- **Mifare DESFire EV2** - Third generation cards

  - Extended functionality over EV1
  - Reduced power consumption
  - Greater read distances possible

- **Mifare DESFire EV3** - Latest generation
  - Further improvements in security and functionality

## Card Organization

DESFire cards store data in a hierarchical structure:

1. **Master Application (PICC level)** - Manages the entire card

   - Protected by the PICC Master Key
   - Controls application creation/deletion and overall card settings

2. **Applications** - Independent security domains on the card

   - Each can be protected with up to 14 different keys with different permissions
   - Applications are isolated from each other (e.g., a canteen application cannot access parking application data)

3. **Files** - Data storage within applications
   - Variable size, contained within applications
   - Different access permissions can be set for reading/writing

## Authentication Modes

The article describes three possible operation modes:

1. **Classic Mode** (not recommended, insecure)

   - Uses only the card's Unique ID (UID) for recognition
   - Cards can be cloned easily with specialized equipment

2. **DESFire Default Mode**

   - Uses the card's UID and application master key for authentication
   - Personalizes cards by:
     - Changing the PICC master key to a secret key
     - Creating a new application with a file containing a 16-byte secret value
     - Protecting both with cryptographic keys derived from card UID, username, and random data

3. **DESFire Random Mode**
   - The card sends a random ID each time it's read
   - Requires authentication with the PICC master key to access the real UID
   - Most secure as it prevents tracking and unauthorized reading

## Cryptographic Keys and Security

- Supports multiple encryption types:

  - **DES** (56-bit) - Least secure, best read distance (up to 5.3cm)
  - **2K3DES** (112-bit)
  - **3K3DES** (168-bit) - Better read distance (up to 5.3cm)
  - **AES** (128-bit) - Most secure, shorter read distance (up to 4.0cm)

- Key diversification ensures each card has unique keys derived from:

  - Card UID
  - User information
  - Random data stored in the system

- No cryptographic keys are ever transmitted in plaintext
- During authentication, only encrypted random values are transferred

## Authentication Process Overview

The DESFire authentication process establishes a secure session between the reader and the card:

1. Reader and card exchange encrypted random numbers
2. Both sides verify they share the same master key
3. A session key is generated from these random values
4. All subsequent communication is encrypted with this session key
5. Communication integrity is verified using CMAC calculations

This establishes a secure channel for all further operations with the card.
