# ESP32 DESFire Proxy Authentication System

This document outlines an architecture for implementing a secure authentication system using:

- ESP32 microcontroller
- NFC reader (PN532)
- DESFire cards
- REST API backend

## Security Design Principles

1. **ESP32 as Secure Proxy**: The ESP32 functions solely as a communication relay between the DESFire card and the authentication API
2. **No Sensitive Data Storage**: No keys, credentials, or other sensitive data are stored on the ESP32
3. **Server-Driven Authentication**: The authentication API controls the entire process and makes all decisions
4. **End-to-End Encryption**: All communication between ESP32 and API is encrypted
5. **Minimal ESP32 Intelligence**: The ESP32 only executes commands as instructed, without understanding their meaning

## System Architecture

```mermaid
flowchart LR
    subgraph "Physical Layer"
        A[DESFire Card]
        B[PN532 NFC Reader]
        C[ESP32]
    end

    subgraph "Network Layer"
        D[TLS/HTTPS Communication]
    end

    subgraph "Server Layer"
        E[Authentication API]
        F[Key Management Service]
        G[Database]
        H[Access Control Service]
    end

    A <--> B
    B <--> C
    C <--> D
    D <--> E
    E <--> F
    E <--> G
    E <--> H
```

## Authentication Flow

```mermaid
sequenceDiagram
    participant Card as DESFire Card
    participant ESP as ESP32 + NFC Reader
    participant API as Authentication API
    participant KMS as Key Management Service
    participant DB as Database

    Note over ESP: Device boot sequence
    ESP->>API: Establish TLS connection
    API->>ESP: Device authentication
    ESP->>API: Send device ID and authentication
    API->>DB: Verify device credentials
    DB->>API: Device authorized

    Note over Card,ESP: Card presented to reader
    ESP->>Card: Detect card presence
    Card->>ESP: Basic card info (Card type, UID/Random ID)
    ESP->>API: Send card identifier

    API->>DB: Look up card
    DB->>API: Return card record
    API->>KMS: Request appropriate keys
    KMS->>API: Return session-specific keys

    API->>ESP: Send command (Select Application)
    ESP->>Card: Execute command
    Card->>ESP: Response
    ESP->>API: Forward response

    API->>ESP: Send command (Authentication sequence)
    ESP->>Card: Execute command
    Card->>ESP: Encrypted challenge
    ESP->>API: Forward encrypted challenge

    API->>KMS: Process challenge with secure keys
    KMS->>API: Return processed response

    API->>ESP: Send command (Response to challenge)
    ESP->>Card: Send response to challenge
    Card->>ESP: Authentication result
    ESP->>API: Forward authentication result

    alt Authentication successful
        API->>ESP: Send commands to read necessary data
        ESP->>Card: Execute read commands
        Card->>ESP: Protected data
        ESP->>API: Forward protected data

        API->>DB: Record successful authentication
        API->>ESP: Send access granted command
        ESP->>User: Indicate access granted (LED/display)
    else Authentication failed
        API->>DB: Log failed authentication attempt
        API->>ESP: Send access denied command
        ESP->>User: Indicate access denied (LED/display)
    end
```

## ESP32 Implementation Architecture

```mermaid
flowchart TB
    subgraph "ESP32 Software Components"
        A[Main Controller] --> B[NFC Interface Module]
        A --> C[API Communication Module]
        A --> D[Command Processor]
        A --> E[Status Indicator]

        B <--> F[Hardware Driver for PN532]
        C <--> G[TLS/HTTPS Client]
        D --> B
        D --> E
    end

    H[Card] <--> F
    G <--> I[Authentication API]
```

## Security Details

### ESP32-API Communication

1. **TLS Connection**:

   - ESP32 establishes a secure TLS 1.3 connection with the API
   - Certificate pinning prevents man-in-the-middle attacks
   - Session-specific keys generated for each connection

2. **API Authentication**:

   - ESP32 authenticates to the API using a device certificate or token
   - Credentials used for authentication are unique to each device
   - Device authentication happens before any card operations

3. **Command Protocol**:
   - All commands from API to ESP32 use a secure protocol
   - Commands include integrity checking (HMAC)
   - Sequence numbers prevent replay attacks

### NFC Operations

1. **Proxy Mode Operation**:

   - ESP32 receives binary commands from API to send to card
   - ESP32 doesn't interpret cryptographic responses
   - ESP32 relays raw responses back to API

2. **Command Isolation**:
   - ESP32 executes one command at a time
   - No command state is stored between operations
   - Timeouts ensure operation doesn't stall

## ESP32 Code Structure

```
/
├── main/
│   ├── main.c                # Main application entry point
│   ├── config.h              # Configuration (non-sensitive)
│   ├── nfc_controller.c      # NFC operations management
│   ├── pn532_driver.c        # Low-level PN532 communication
│   ├── api_client.c          # API communication handler
│   ├── command_processor.c   # Process and execute API commands
│   ├── status_indicator.c    # LED/display feedback
│   └── secure_boot.c         # Secure boot procedures
├── components/
│   ├── esp_https_client/     # HTTPS client library
│   ├── nvs_flash/            # Non-volatile storage (for non-sensitive data)
│   └── esp_tls/              # TLS implementation
└── partitions.csv            # Memory partition layout
```

## Command Structure (API to ESP32)

```json
{
  "command_id": 123,
  "command_type": "card_operation",
  "operation": {
    "type": "transceive",
    "data": "base64_encoded_binary_data_to_send_to_card",
    "timeout_ms": 1000
  },
  "hmac": "signature_of_command_for_integrity"
}
```

## ESP32 Response Structure

```json
{
  "response_to": 123,
  "status": "success",
  "card_response": "base64_encoded_binary_response_from_card",
  "timing_ms": 35,
  "device_status": {
    "rssi": -65,
    "heap_free": 123456,
    "uptime_s": 3600
  },
  "hmac": "signature_of_response_for_integrity"
}
```

## Implementation Steps

1. **ESP32 Setup**:

   - Configure hardware with PN532 NFC reader
   - Implement secure boot to verify firmware
   - Configure Wi-Fi or Ethernet connectivity
   - Set up TLS client with certificate pinning

2. **Authentication API Development**:

   - Implement secure REST API endpoints
   - Create key management system
   - Develop DESFire authentication logic
   - Implement access control policies

3. **Integration Testing**:
   - Verify secure communication between ESP32 and API
   - Test complete authentication flows
   - Perform security penetration testing
   - Validate behavior with various card types

## Security Hardening Recommendations

1. **ESP32 Device Security**:

   - Enable secure boot
   - Encrypt flash memory
   - Disable UART debugging in production
   - Implement tamper detection
   - Configure watchdog timers

2. **Network Security**:

   - Use TLS 1.3 with strong cipher suites
   - Implement certificate pinning
   - Rotate device certificates periodically
   - Rate-limit authentication attempts
   - Monitor for anomalous traffic patterns

3. **API Server Security**:
   - Store cryptographic keys in a hardware security module (HSM)
   - Implement robust logging and monitoring
   - Deploy intrusion detection systems
   - Use role-based access control
   - Regular security audits and updates

## Detailed PN532-ESP32 Connection

```mermaid
flowchart TB
    subgraph "Hardware Connection"
        A[PN532 NFC Reader] <--> B[ESP32]

        subgraph "PN532 Module"
            A1[Antenna] --> A2[PN532 Chip]
            A2 --> A3[Interface Logic]
        end

        subgraph "ESP32 Module"
            B1[GPIO Pins] --> B2[SPI/I2C Controller]
            B2 --> B3[MCU Core]
            B3 --> B4[Wi-Fi/Ethernet]
        end

        A3 <--> |SPI or I2C| B1
    end

    style A1 fill:#f9f,stroke:#333,stroke-width:2px
    style B4 fill:#bbf,stroke:#333,stroke-width:2px
```

## Security Comparison

| Component             | Traditional Approach       | Proxy-based Approach       |
| --------------------- | -------------------------- | -------------------------- |
| **ESP32 Key Storage** | Stores authentication keys | No key storage             |
| **Decision Making**   | ESP32 makes auth decisions | API makes all decisions    |
| **Compromise Impact** | High - keys exposed        | Low - no secrets to expose |
| **Offline Operation** | Possible                   | Requires API connectivity  |
| **Key Rotation**      | Requires firmware update   | API handles key rotation   |
| **Security Updates**  | Requires device update     | Server-side updates        |

## Handling Connectivity Issues

```mermaid
flowchart TD
    A[Card Detected] --> B{API Connection Available?}

    B -->|Yes| C[Normal Authentication Flow]
    B -->|No| D[Store Minimal Card Data]

    D --> E[Retry API Connection]
    E --> F{Connection Restored?}

    F -->|Yes| G[Send Cached Card Data]
    F -->|No| H[Continue Retry Loop]

    G --> I{Authentication Result?}

    I -->|Success| J[Grant Delayed Access]
    I -->|Failure| K[Clear Cache & Deny]

    H --> L[If Max Retries: Clear Cache & Deny]
```

This architecture ensures that all sensitive operations and data remain on the server side, with the ESP32 functioning merely as a secured communication proxy between the DESFire card and the Authentication API.
