# Mifare Classic vs. DESFire Comparison

This document provides a detailed comparison between Mifare Classic and DESFire card technologies, focusing on security, structure, and functionality.

## Side-by-Side Comparison

| Feature                  | Mifare Classic                                                                                    | Mifare DESFire EV1/EV2/EV3                                                                                  |
| ------------------------ | ------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------- |
| **Unique Identifier**    | 4 bytes<br>UID can always be read without encryption                                              | 7 bytes<br>UID can be read without encryption in default mode<br>Requires PICC master key in random ID mode |
| **EEPROM Storage**       | On a 1kB card:<br>16 sectors of 4 blocks of 16 bytes each<br>(Blocks and sectors have fixed size) | Up to 28 applications<br>Each application can contain up to 32 files<br>Files have variable size            |
| **Keys**                 | Each sector protected with two keys (A and B)<br>Different permissions per key                    | Each application protected with up to 14 different keys<br>Different permissions per key                    |
| **Encryption**           | Proprietary (Crypto-1, 48 bit)                                                                    | DES (56 bit)<br>2K3DES (112 bit)<br>3K3DES (168 bit)<br>AES (128 bit)                                       |
| **Security Status**      | Encryption cracked in 2008<br>Cards can be cloned in seconds                                      | No known attacks<br>Side-channel attack on original DESFire (not EV1/EV2/EV3)                               |
| **Card Structure**       | Static organization with fixed sectors and blocks                                                 | Dynamic with applications and files                                                                         |
| **Read Distance**        | Up to 10cm with proper antenna                                                                    | DES: Up to 5.3cm<br>AES: Up to 4.0cm<br>(Distance depends on power consumption)                             |
| **Authentication**       | Simple sector-based key authentication                                                            | Complex cryptographic authentication with session keys                                                      |
| **Cloning Protection**   | None (easily cloned)                                                                              | Strong protection through advanced encryption                                                               |
| **ID Randomization**     | Not available                                                                                     | Available (Random ID mode)                                                                                  |
| **Key Diversification**  | Not supported natively                                                                            | Fully supported                                                                                             |
| **Standards Compliance** | Proprietary                                                                                       | ISO/IEC 14443-4                                                                                             |

## Security Vulnerabilities

### Mifare Classic

```mermaid
flowchart TD
    A[Mifare Classic Security] --> B{Crypto-1 Algorithm}
    B --> C[48-bit Key]
    B --> D[Proprietary Algorithm]
    B --> E[Security through Obscurity]

    E --> F[Algorithm Kept Secret for 14 Years]

    F --> G[2008: Algorithm Reverse-Engineered]
    G --> H[Multiple Design Flaws Discovered]

    H --> I[Card Cloning Possible in Seconds]
    H --> J[Key Recovery Attacks]

    I --> K[Chinese Clone Cards Available]
    K --> L[UID Changeable Cards]

    M[Mifare Classic Attack Methods] --> N[Direct Crypto-1 Attacks]
    M --> O[Key Stream Recovery]
    M --> P[Nested Authentication Attack]
    M --> Q[Darkside Attack]
```

### DESFire Original (Pre-EV1)

```mermaid
flowchart TD
    A[Original DESFire Security] --> B[DES Encryption]

    B --> C[Side-Channel Attack Discovered]
    C --> D[Power Consumption Analysis]
    D --> E[7 Hours Required for Key Extraction]

    F[Legacy Authentication Mode] --> G[Vulnerable to Attacks]
    F --> H[Not Supported in Modern Libraries]
```

### DESFire EV1/EV2/EV3

```mermaid
flowchart TD
    A[DESFire EV1/EV2/EV3 Security] --> B[Standard Cryptographic Algorithms]
    B --> C[AES-128]
    B --> D[3K3DES-168]

    A --> E[No Known Successful Attacks]

    A --> F[Random ID Mode]
    F --> G[Different ID Each Time]
    F --> H[Prevents Tracking]

    A --> I[Session Key Model]
    I --> J[Unique Keys for Each Session]
    I --> K[Forward Secrecy]

    A --> L[Message Authentication Code]
    L --> M[Ensures Data Integrity]
    L --> N[Prevents Tampering]
```

## Memory Structure Visualization

### Mifare Classic Memory Structure

```mermaid
flowchart TD
    A[Mifare Classic Card] --> B[Sector 0]
    A --> C[Sector 1]
    A --> D[Sector 2]
    A --> E[...]
    A --> F[Sector 15]

    B --> G[Block 0: Manufacturer Data + UID]
    B --> H[Block 1: Data]
    B --> I[Block 2: Data]
    B --> J[Block 3: Keys A/B + Access Bits]

    C --> K[Block 0: Data]
    C --> L[Block 1: Data]
    C --> M[Block 2: Data]
    C --> N[Block 3: Keys A/B + Access Bits]
```

### DESFire Memory Structure

```mermaid
flowchart TD
    A[DESFire Card] --> B[PICC Level]
    B --> C[Master Key]
    B --> D[Key Settings]

    A --> E[Application 1]
    A --> F[Application 2]
    A --> G[Application ...]

    E --> H[App 1 Key 1]
    E --> I[App 1 Key 2]
    E --> J[App 1 Key ...]

    E --> K[File 1]
    E --> L[File 2]
    E --> M[File ...]

    F --> N[App 2 Keys]
    F --> O[App 2 Files]

    K --> P[File Settings]
    K --> Q[File Data]
```

## Access Control System Architecture

```mermaid
flowchart TB
    A[Start] --> B[Card Presented]
    B --> C[Reader Activates RF Field]
    C --> D[Card Responds]

    D --> E{Card Type?}

    E -->|Mifare Classic| F[Read UID]
    F --> G[Look Up UID in Database]
    G --> H{Authorized?}

    E -->|DESFire| I[Read Card Info]
    I --> J{Operation Mode?}

    J -->|Default Mode| K[Select Application]
    K --> L[Authenticate with App Key]
    L --> M[Read Secret Value]
    M --> N{Verified?}

    J -->|Random ID Mode| O[Authenticate with PICC Key]
    O --> P[Get Real UID]
    P --> Q[Look Up UID in Database]
    Q --> R{Authorized?}

    H -->|Yes| S[Grant Access]
    H -->|No| T[Deny Access]

    N -->|Yes| S
    N -->|No| T

    R -->|Yes| S
    R -->|No| T

    S --> U[End]
    T --> U
```

## Data Flow in Operation Modes

```mermaid
flowchart TD
    A[Start] --> B{Which Operation Mode?}

    B -->|Classic Mode| C[Supports Classic Cards]
    C --> D[Reads Card UID]
    D --> E[Compares UID to Stored List]

    B -->|DESFire Default Mode| F[Supports DESFire Cards]
    F --> G[Reads Card UID]
    G --> H[Authenticates with Application Key]
    H --> I[Reads and Verifies Secret Value]

    B -->|DESFire Random Mode| J[Supports DESFire Cards]
    J --> K[Reads Random ID]
    K --> L[Authenticates with PICC Master Key]
    L --> M[Gets Real UID]
    M --> N[Compares UID to Stored List]

    E -->|Match| O[Access Granted]
    E -->|No Match| P[Access Denied]

    I -->|Verified| O
    I -->|Not Verified| P

    N -->|Match| O
    N -->|No Match| P
```

## Key Management Workflow

```mermaid
flowchart TD
    A[Administrator] --> B[Add User]
    B --> C[Present Blank Card]

    C --> D{Card Type?}

    D -->|Classic| E[Read UID]
    E --> F[Store UID + User Info]

    D -->|DESFire Default| G[Read UID]
    G --> H[Generate Diversified App Key]
    H --> I[Generate Diversified Store Value]
    I --> J[Change PICC Master Key]
    J --> K[Create Application]
    K --> L[Create File]
    L --> M[Write Secret Value]
    M --> N[Store UID + User Info]

    D -->|DESFire Random| O[Authenticate with Default Key]
    O --> P[Change PICC Master Key]
    P --> Q[Enable Random ID]
    Q --> R[Get Real UID]
    R --> S[Store UID + User Info]

    F --> T[User Management Database]
    N --> T
    S --> T

    U[Delete User] --> V[Find User in Database]
    V --> W[Delete User Entry]
    W --> X{Card Available?}

    X -->|Yes| Y[Restore Card to Factory Settings]
    X -->|No| Z[Only Remove from Database]
```

## Countermeasure Recommendations

### For Mifare Classic Users

1. **Migrate to DESFire** - The only truly secure option
2. **RFID Blocking Wallet** - Prevent unauthorized card reading
3. **Regular Monitoring** - Check access logs frequently
4. **Secondary Authentication** - Combine with PIN or biometrics

### For DESFire Users

1. **Use Random ID Mode** - Prevents tracking and unauthorized reading
2. **Use AES Encryption** - Strongest available option
3. **Key Diversification** - Ensure each card has unique keys
4. **Regular Key Updates** - Change keys periodically
5. **Avoid Using Legacy DES** - Use 3K3DES or AES instead
