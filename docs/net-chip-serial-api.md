# NET Chip Serial Configuration API

This document describes the serial protocol for configuring the Hactar NET chip (ESP32-S3) via USB. Commands are sent through the Management (MGMT) chip, which routes them to the NET chip.

## Table of Contents

1. [UART Configuration](#uart-configuration)
2. [Link TLV Protocol](#link-tlv-protocol)
3. [Message Routing](#message-routing)
4. [Response Format](#response-format)
5. [NET Chip Commands](#net-chip-commands)
6. [Payload Encoding](#payload-encoding)
7. [Examples](#examples)

---

## UART Configuration

Connect to the Hactar device via USB serial with the following settings:

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Flow Control | None |

---

## Link TLV Protocol

All messages use the **Link TLV (Type-Length-Value)** framing format:

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Sync Word (0x4C494E4B)                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Type (u16 LE)        |        Length (u32 LE)        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          (Length cont.)       |                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
|                         Value (payload)                       |
|                             ...                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field | Size | Encoding | Description |
|-------|------|----------|-------------|
| Sync Word | 4 bytes | Big-endian | Magic value `0x4C494E4B` (ASCII "LINK") |
| Type | 2 bytes | Little-endian | Message type code |
| Length | 4 bytes | Little-endian | Byte length of Value field |
| Value | 0..N bytes | Raw | Message payload |

### Sync Word

The sync word enables receivers to recover framing. It is the ASCII string "LINK":

```
[0x4C, 0x49, 0x4E, 0x4B]
```

---

## Message Routing

Commands to the NET chip are **tunneled through the MGMT chip** using nested TLV messages.

### Routing Types (MGMT)

| Type ID | Name | Description |
|---------|------|-------------|
| 17 | ToUi | Route payload to UI chip |
| 18 | ToNet | Route payload to NET chip |
| 19 | Loopback | Echo payload back (testing) |

### Message Structure

To send a command to the NET chip, construct a nested TLV:

```
+--------------------------------------------------+
| Outer TLV (to MGMT)                              |
|   Sync: "LINK"                                   |
|   Type: 18 (ToNet)                               |
|   Length: size of inner TLV                      |
|   Value:                                         |
|     +------------------------------------------+ |
|     | Inner TLV (to NET)                       | |
|     |   Sync: "LINK"                           | |
|     |   Type: <command_id>                     | |
|     |   Length: size of payload                | |
|     |   Value: <command payload>               | |
|     +------------------------------------------+ |
+--------------------------------------------------+
```

---

## Response Format

The NET chip responds with **TLV-formatted responses** using high type values:

### Response Types

| Type | Name | Description |
|------|------|-------------|
| `0x8000` | ACK | Command succeeded (no data) |
| `0x8001` | ERROR | Command failed |
| `0x8002` | WIFI_SSIDS | JSON array of WiFi credentials |
| `0x8003` | RELAY_URL | UTF-8 URL string |
| `0x8004` | LOOPBACK | 1 byte: loopback mode (0=Off, 1=Raw, 2=Moq) |
| `0x8005` | LOGS_ENABLED | 1 byte: 0=disabled, 1=enabled |
| `0x8006` | LANGUAGE | UTF-8 language tag (e.g., "en-US") |
| `0x8007` | CHANNEL | JSON array of namespace parts |
| `0x8008` | AI_CONFIG | JSON object with query/audio/cmd arrays |

### Response TLV Format

```
[4 bytes: Sync Word "LINK"]
[2 bytes: Response Type (little-endian)]
[4 bytes: Payload Length (little-endian)]
[N bytes: Payload (if any)]
```

---

## NET Chip Commands

### Command Summary

| ID | Name | Parameters | Response Type |
|----|------|------------|---------------|
| 0 | version | None | ACK + version string |
| 1 | clear_storage | None | ACK/ERROR |
| 2 | add_wifi | JSON: `{"ssid":"...","password":"..."}` | ACK/ERROR |
| 3 | get_wifi | None | WIFI_SSIDS |
| 4 | clear_wifi | None | ACK |
| 5 | set_relay_url | UTF-8 URL | ACK |
| 6 | get_relay_url | None | RELAY_URL |
| 7 | get_loopback | None | LOOPBACK |
| 8 | set_loopback | 1 byte: mode | ACK/ERROR |
| 9 | get_logs_enabled | None | LOGS_ENABLED |
| 10 | set_logs_enabled | 1 byte: 0/1 | ACK |
| 11 | set_language | UTF-8 language tag | ACK/ERROR |
| 12 | get_language | None | LANGUAGE |
| 13 | set_channel | JSON array | ACK/ERROR |
| 14 | get_channel | None | CHANNEL |
| 15 | set_ai | JSON config | ACK/ERROR |
| 16 | get_ai | None | AI_CONFIG |
| 17 | burn_efuse | None | ACK/ERROR |

### Command Details

#### 0 - version

Get the firmware version.

- **Payload**: None
- **Response**: ACK, followed by version data

---

#### 1 - clear_storage

Clear all stored configuration from NVS (non-volatile storage).

- **Payload**: None
- **Response**: ACK on success, ERROR on failure

---

#### 2 - add_wifi

Add WiFi credentials. The device will attempt to connect.

- **Payload**: JSON object
  ```json
  {"ssid": "MyNetwork", "password": "secret123"}
  ```
- **Response**: ACK on success, ERROR on invalid JSON or missing fields

---

#### 3 - get_wifi

Get list of stored WiFi credentials.

- **Payload**: None
- **Response**: WIFI_SSIDS - JSON array
  ```json
  [{"ssid": "Network1", "password": "pass1"}, {"ssid": "Network2", "password": "pass2"}]
  ```

---

#### 4 - clear_wifi

Clear all stored WiFi credentials.

- **Payload**: None
- **Response**: ACK

---

#### 5 - set_relay_url

Set the MOQ relay server URL.

- **Payload**: UTF-8 URL string (e.g., `moq://relay.example.com:33435`)
- **Response**: ACK
- **Note**: An empty URL clears the stored value

---

#### 6 - get_relay_url

Get the current MOQ relay server URL.

- **Payload**: None
- **Response**: RELAY_URL - UTF-8 string

---

#### 7 - get_loopback

Get current loopback mode.

- **Payload**: None
- **Response**: LOOPBACK - 1 byte
  - `0x00` = Off (normal operation)
  - `0x01` = Raw (not supported)
  - `0x02` = Moq (self-echo via relay)

---

#### 8 - set_loopback

Set loopback mode.

- **Payload**: 1 byte - NetLoopbackMode
  - `0x00` = Off
  - `0x02` = Moq
- **Response**: ACK on success, ERROR if mode not supported
- **Note**: Raw mode (0x01) is not supported on NET

---

#### 9 - get_logs_enabled

Get debug logging state.

- **Payload**: None
- **Response**: LOGS_ENABLED - 1 byte
  - `0x00` = disabled
  - `0x01` = enabled

---

#### 10 - set_logs_enabled

Enable or disable debug logging.

- **Payload**: 1 byte
  - `0x00` = disable
  - `0x01` = enable
- **Response**: ACK

---

#### 11 - set_language

Set language preference for AI features.

- **Payload**: UTF-8 language tag
- **Supported**: `en-US`, `es-ES`, `de-DE`, `hi-IN`, `nb-NO`
- **Response**: ACK on success, ERROR on invalid language

---

#### 12 - get_language

Get current language setting.

- **Payload**: None
- **Response**: LANGUAGE - UTF-8 string (e.g., "en-US")

---

#### 13 - set_channel

Set the channel namespace configuration.

- **Payload**: JSON array of namespace parts
  ```json
  ["relay.example.com", "org", "channel", "ptt"]
  ```
- **Response**: ACK on success, ERROR on invalid JSON

---

#### 14 - get_channel

Get current channel configuration.

- **Payload**: None
- **Response**: CHANNEL - JSON array

---

#### 15 - set_ai

Set AI service configuration.

- **Payload**: JSON object
  ```json
  {
    "query": ["relay.example.com", "ai", "query"],
    "audio": ["relay.example.com", "ai", "audio"],
    "cmd": ["relay.example.com", "ai", "cmd"]
  }
  ```
- **Response**: ACK on success, ERROR on invalid JSON

---

#### 16 - get_ai

Get current AI configuration.

- **Payload**: None
- **Response**: AI_CONFIG - JSON object

---

#### 17 - burn_efuse

**WARNING: This operation is IRREVERSIBLE.**

Permanently disable USB JTAG debugging by burning an eFuse.

- **Payload**: None
- **Response**: ACK on success, ERROR on failure

---

## Payload Encoding

### JSON Commands

Commands using JSON (add_wifi, set_channel, set_ai) send the JSON as raw UTF-8:

```
Payload: [UTF-8 JSON string bytes]
```

### Boolean/Enum Commands

Commands with single-byte values (set_loopback, set_logs_enabled) send the raw byte:

```
Payload: [1 byte value]
```

### String Commands

Commands with string values (set_relay_url, set_language) send raw UTF-8:

```
Payload: [UTF-8 string bytes]
```

---

## Examples

### Example 1: Get Logs Enabled

Check if debug logging is enabled.

**Command**: `get_logs_enabled` (ID: 9, no payload)

**Wire bytes** (hex):
```
4C 49 4E 4B 12 00 0A 00 00 00 4C 49 4E 4B 09 00 00 00 00 00
```

**Expected Response** (logs enabled):
```
4C 49 4E 4B     # Sync "LINK"
05 80           # Type 0x8005 (LOGS_ENABLED)
01 00 00 00     # Length 1
01              # Value: enabled
```

---

### Example 2: Add WiFi

Add WiFi credentials.

**Command**: `add_wifi` (ID: 2)
**Payload**: `{"ssid":"MyNetwork","password":"secret123"}` (42 bytes)

**Expected Response**: ACK (type 0x8000, length 0)

---

### Example 3: Get WiFi

Retrieve stored WiFi credentials.

**Command**: `get_wifi` (ID: 3, no payload)

**Expected Response** (type 0x8002 WIFI_SSIDS):
```json
[{"ssid":"MyNetwork","password":"secret123"}]
```

---

## Implementation Notes

1. **Typed Responses**: All data responses use specific type codes for self-describing payloads.

2. **Sync Word Scanning**: Receivers should scan for the sync word to recover framing after unexpected data.

3. **Timeout Handling**: Use a read timeout (e.g., 100ms) when waiting for responses.

4. **Logging Interference**: Debug logs may be interleaved with responses. Consider disabling logs during configuration.

5. **Endianness**: All multi-byte integers use **little-endian** encoding.
