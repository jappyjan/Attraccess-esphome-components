# Attraccess DESFire Component for ESPHome

This component provides DESFire card authentication for ESPHome, designed to work with the Attraccess Resource API. It functions as a secure proxy between DESFire cards and the Authentication API, enabling resource control with server-driven authentication.

## Features

- Secure ESP32-based proxy for DESFire authentication
- No sensitive data stored on the ESP32
- Server-driven authentication process
- Button-triggered card reading with LED status indication
- Integration with the Attraccess Resource API
- Extensible automation triggers for card events

## Security Design

This component follows these security principles:

1. **ESP32 as Secure Proxy**: The ESP32 functions solely as a communication relay
2. **No Sensitive Data Storage**: No keys, credentials, or sensitive data stored on the ESP32
3. **Server-Driven Authentication**: The authentication API controls the entire process
4. **Intent-Based Actions**: Physical button press required to initiate state change requests

## Hardware Requirements

- ESP32 development board
- PN532 NFC reader (connected via I2C)
- LED for status indication (optional)
- Button for triggering card reads (optional)
- DESFire cards (paired with backend authentication system)

## Installation

1. Add the component to your ESPHome configuration using external_components:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/jappyjan/Attraccess-esphome-components.git
      ref: main
    components: [attraccess_resource, attraccess_desfire]
```

2. Configure the I2C bus for the PN532 NFC reader:

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true
  frequency: 100kHz # Lower frequency for more reliable NFC communication
```

3. Configure the resource component:

```yaml
attraccess_resource:
  id: my_resource
  api_url: https://api.example.com # Base URL
  resource_id: "my-door"
  refresh_interval: 30s
```

4. Configure the DESFire component:

```yaml
attraccess_desfire:
  id: my_desfire_reader
  resource: my_resource # Reference to attraccess_resource component
  led_pin: GPIO2 # Status LED
  button_pin: GPIO15 # Button for triggering card reads
  reset_pin: GPIO16 # PN532 reset pin
  irq_pin: GPIO17 # PN532 IRQ pin
  api_key: !secret desfire_api_key # API key for authentication (optional)
  timeout: 30s # Timeout for card reading
```

## Working with the Component

### Authentication Flow

1. User presses the button to indicate intent
2. ESP32 sends button press event to API
3. ESP32 starts waiting for a card to be presented
4. User presents DESFire card
5. ESP32 reads card ID and sends it to API
6. API sends commands to ESP32 to execute on the card
7. ESP32 relays commands to card and sends responses back to API
8. API makes the authentication decision and sends result to ESP32
9. ESP32 indicates success/failure via LED and triggers automations

### Binary Command Protocol

The component uses a binary protocol for communication with the API:

```
+--------+--------+--------+--------+--------+--//--+--------+--------+--------+--//--+--------+
| MAGIC  | VERSION| CMD_ID |CMD_TYPE|  LEN   | DATA  | NONCE  |HMAC_LEN|  HMAC  |
+--------+--------+--------+--------+--------+--//--+--------+--------+--------+--//--+--------+
  2 bytes  1 byte   2 bytes  1 byte   2 bytes  n bytes 4 bytes  1 byte   m bytes
```

### Configuration Options

| Option        | Description                                           | Default                                 |
| ------------- | ----------------------------------------------------- | --------------------------------------- |
| `resource`    | Reference to the attraccess_resource component        | Required                                |
| `resource_id` | Override for the resource ID from attraccess_resource | (from resource)                         |
| `led_pin`     | GPIO pin for status LED                               | Optional                                |
| `button_pin`  | GPIO pin for button to trigger card reading           | Optional                                |
| `reset_pin`   | GPIO pin connected to PN532 reset                     | Optional                                |
| `irq_pin`     | GPIO pin connected to PN532 IRQ                       | Optional                                |
| `device_id`   | Unique identifier for this device                     | "resource-[resource_id]-desfire-reader" |
| `api_key`     | API key for authentication                            | Optional                                |
| `timeout`     | Timeout period for card reading                       | 30s                                     |

The API endpoint is automatically constructed from the resource component's API URL as `[api_url]/desfire/proxy`.

### Triggers

The component provides several triggers for automation:

```yaml
attraccess_desfire:
  # ... other configuration ...

  # Fires when a card is successfully authenticated
  on_card_authorized:
    - logger.log:
        level: INFO
        format: "Card authorized"

  # Fires when authentication fails
  on_card_denied:
    - logger.log:
        level: INFO
        format: "Card denied"

  # Fires when any card is read, provides the UID
  on_card_read:
    - logger.log:
        level: INFO
        format: "Card read: %s"
        args: ["uid"]
```

## API Backend Requirements

To use this component, you need a backend authentication API that:

1. Receives button press events from the ESP32
2. Processes card detection events with card UIDs
3. Sends commands to the ESP32 to communicate with the card
4. Implements the DESFire authentication protocol
5. Makes the final authentication decision
6. Manages resource state changes

The API should implement the binary command protocol described above.

## Example Complete Configuration

See the `example_desfire_config.yaml` file for a complete configuration example that includes visual feedback with indicator lights.

## Limitations

- This component implements a proxy-based approach and requires a backend authentication API
- The ESP32 only relays commands and doesn't implement the cryptographic operations
- For proper security, use TLS (HTTPS) for the API communication

## Troubleshooting

- If the PN532 is not detected, try reducing the I2C frequency (100kHz or less is recommended)
- Ensure proper wiring between ESP32 and the PN532 module
- Check that your DESFire cards are properly set up in your backend system
- For reliable operation, keep the card close to the reader during the entire authentication process
- If API communication fails, verify network connectivity and API endpoint configuration
