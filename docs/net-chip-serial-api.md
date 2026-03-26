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

### Wire Format Example

To send `disable_logs` (type 9, no payload) to NET:

```
Outer TLV:
  4C 49 4E 4B        # Sync word "LINK"
  12 00              # Type 18 (ToNet), little-endian
  0A 00 00 00        # Length 10 (inner TLV size), little-endian

Inner TLV:
  4C 49 4E 4B        # Sync word "LINK"
  09 00              # Type 9 (disable_logs), little-endian
  00 00 00 00        # Length 0 (no payload), little-endian
```

Total: 20 bytes

---

## Response Format

The NET chip responds with single-byte status codes:

| Code | Name | Description |
|------|------|-------------|
| `0x82` | ACK | Command succeeded |
| `0x83` | NACK | Command failed |

For commands that return data (e.g., `get_moq_url`), the data is sent as raw bytes after the command is processed. The host should read until timeout or a known delimiter.

---

## NET Chip Commands

### Command Summary

| ID | Name | Parameters | Description |
|----|------|------------|-------------|
| 0 | Version | None | Get firmware version |
| 1 | Clear_Storage | None | Clear all stored configuration |
| 2 | Set_Ssid | ssid, password | Add WiFi credentials |
| 3 | Get_Ssid_Names | None | Get stored SSID names |
| 4 | Get_Ssid_Passwords | None | Get stored SSID passwords |
| 5 | Clear_Ssids | None | Clear all stored WiFi credentials |
| 6 | Set_Moq_Url | url | Set MOQ relay server URL |
| 7 | Get_Moq_Url | None | Get current MOQ relay URL |
| 8 | Toggle_Logs | None | Toggle debug logging |
| 9 | Disable_Logs | None | Disable debug logging |
| 10 | Enable_Logs | None | Enable debug logging |
| 11 | Disable_Loopback | None | Disable audio loopback mode |
| 12 | Enable_Loopback | None | Enable audio loopback mode |
| 13 | Set_Frontline_Config | language, channel | Set language and channel |
| 14 | Burn_Disable_USB_JTag_Efuse | None | Permanently disable USB JTAG |

### Command Details

#### 0 - Version

Get the firmware version.

- **Payload**: None
- **Response**: ACK, followed by version data

---

#### 1 - Clear_Storage

Clear all stored configuration from NVS (non-volatile storage).

- **Payload**: None
- **Response**: ACK on success, NACK on failure

---

#### 2 - Set_Ssid

Add WiFi credentials to stored list. The device will attempt to connect.

- **Parameters**: 2 (ssid_name, ssid_password)
- **Payload Format**:
  ```
  [4 bytes: ssid_name_length (u32 LE)]
  [N bytes: ssid_name (UTF-8)]
  [4 bytes: ssid_password_length (u32 LE)]
  [M bytes: ssid_password (UTF-8)]
  ```
- **Response**: ACK on success

---

#### 3 - Get_Ssid_Names

Get list of stored WiFi SSID names.

- **Payload**: None
- **Response**: Newline-separated list of SSID names (UTF-8)

---

#### 4 - Get_Ssid_Passwords

Get list of stored WiFi passwords.

- **Payload**: None
- **Response**: Newline-separated list of passwords (UTF-8)

---

#### 5 - Clear_Ssids

Clear all stored WiFi credentials.

- **Payload**: None
- **Response**: ACK on success

---

#### 6 - Set_Moq_Url

Set the MOQ relay server URL.

- **Parameters**: 1 (url)
- **Payload Format**:
  ```
  [N bytes: url (UTF-8)]
  ```
- **Response**: ACK on success
- **Note**: An empty URL clears the stored value

---

#### 7 - Get_Moq_Url

Get the current MOQ relay server URL.

- **Payload**: None
- **Response**: URL string (UTF-8)

---

#### 8 - Toggle_Logs

Toggle debug logging on/off.

- **Payload**: None
- **Response**: ACK

---

#### 9 - Disable_Logs

Disable all debug logging output.

- **Payload**: None
- **Response**: ACK

---

#### 10 - Enable_Logs

Enable debug logging output.

- **Payload**: None
- **Response**: ACK

---

#### 11 - Disable_Loopback

Disable audio loopback mode. Audio is sent to the MOQ relay.

- **Payload**: None
- **Response**: ACK

---

#### 12 - Enable_Loopback

Enable audio loopback mode. Audio is echoed locally without network transmission.

- **Payload**: None
- **Response**: ACK

---

#### 13 - Set_Frontline_Config

Set the language preference and default channel.

- **Parameters**: 2 (language, channel)
- **Payload Format**:
  ```
  [4 bytes: language_length (u32 LE)]
  [N bytes: language (UTF-8, e.g., "en-US")]
  [4 bytes: channel_length (u32 LE)]
  [M bytes: channel (UTF-8, e.g., "gardening")]
  ```
- **Response**: ACK on success

---

#### 14 - Burn_Disable_USB_JTag_Efuse

**WARNING: This operation is IRREVERSIBLE.**

Permanently disable USB JTAG debugging by burning an eFuse.

- **Payload**: None
- **Response**: ACK on success, NACK on failure

---

## Payload Encoding

### Single Parameter Commands

For commands with a single string parameter (e.g., `Set_Moq_Url`), the payload is the raw UTF-8 encoded string:

```
Payload: [UTF-8 string bytes]
```

### Multi-Parameter Commands

For commands with multiple parameters (e.g., `Set_Ssid`, `Set_Frontline_Config`), each parameter is length-prefixed:

```
Payload:
  [4 bytes: param1_length (u32 LE)]
  [param1_length bytes: param1 (UTF-8)]
  [4 bytes: param2_length (u32 LE)]
  [param2_length bytes: param2 (UTF-8)]
  ...
```

### Integer Encoding

All integers are encoded as **little-endian**.

---

## Examples

### Example 1: Disable Logs

Disable debug logging on the NET chip.

**Command**: `disable_logs` (ID: 9, no payload)

**Wire bytes** (hex):
```
4C 49 4E 4B 12 00 0A 00 00 00 4C 49 4E 4B 09 00 00 00 00 00
```

**Breakdown**:
```
4C 49 4E 4B     # Outer sync "LINK"
12 00           # Outer type 18 (ToNet)
0A 00 00 00     # Outer length 10

4C 49 4E 4B     # Inner sync "LINK"
09 00           # Inner type 9 (disable_logs)
00 00 00 00     # Inner length 0
```

**Expected Response**: `0x82` (ACK)

---

### Example 2: Set MOQ URL

Set the MOQ relay URL to `moq://relay.example.com:33435`.

**Command**: `set_moq_url` (ID: 6)
**Payload**: `moq://relay.example.com:33435` (30 bytes)

**Wire bytes** (hex):
```
4C 49 4E 4B                         # Outer sync "LINK"
12 00                               # Outer type 18 (ToNet)
28 00 00 00                         # Outer length 40 (10 + 30)

4C 49 4E 4B                         # Inner sync "LINK"
06 00                               # Inner type 6 (set_moq_url)
1E 00 00 00                         # Inner length 30

6D 6F 71 3A 2F 2F 72 65 6C 61 79    # "moq://relay"
2E 65 78 61 6D 70 6C 65 2E 63 6F    # ".example.co"
6D 3A 33 33 34 33 35                # "m:33435"
```

**Expected Response**: `0x82` (ACK)

---

### Example 3: Set WiFi Credentials

Add WiFi credentials: SSID = "MyNetwork", Password = "secret123".

**Command**: `set_ssid` (ID: 2)
**Parameters**:
- ssid_name: "MyNetwork" (9 bytes)
- ssid_password: "secret123" (9 bytes)

**Payload**:
```
09 00 00 00                         # ssid_name_length = 9
4D 79 4E 65 74 77 6F 72 6B          # "MyNetwork"
09 00 00 00                         # ssid_password_length = 9
73 65 63 72 65 74 31 32 33          # "secret123"
```

**Total payload**: 26 bytes

**Wire bytes** (hex):
```
4C 49 4E 4B                         # Outer sync "LINK"
12 00                               # Outer type 18 (ToNet)
24 00 00 00                         # Outer length 36 (10 + 26)

4C 49 4E 4B                         # Inner sync "LINK"
02 00                               # Inner type 2 (set_ssid)
1A 00 00 00                         # Inner length 26

09 00 00 00                         # ssid_name_length
4D 79 4E 65 74 77 6F 72 6B          # "MyNetwork"
09 00 00 00                         # ssid_password_length
73 65 63 72 65 74 31 32 33          # "secret123"
```

**Expected Response**: `0x82` (ACK)

---

### Example 4: Get MOQ URL

Retrieve the currently configured MOQ URL.

**Command**: `get_moq_url` (ID: 7, no payload)

**Wire bytes** (hex):
```
4C 49 4E 4B 12 00 0A 00 00 00 4C 49 4E 4B 07 00 00 00 00 00
```

**Expected Response**: Raw UTF-8 string of the URL (no framing)

---

## Implementation Notes

1. **Sync Word Scanning**: Receivers should scan for the sync word to recover framing after unexpected data (e.g., boot messages).

2. **Timeout Handling**: Use a read timeout (e.g., 100ms) when waiting for responses. Retry if no response is received.

3. **Logging Interference**: Debug logs from the NET chip may be interleaved with responses. Consider disabling logs before sending configuration commands.

4. **Connection Order**: The MGMT chip must be programmed and running before commands can be routed to the NET chip.

5. **Endianness**: All multi-byte integers use **little-endian** encoding.
