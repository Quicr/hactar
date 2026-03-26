# Hactar Command Reference

## NET Firmware Commands (via MGMT link)

| ID | Command                       | Params | Request Payload                                           | Response                        | Status      |
|----|-------------------------------|--------|-----------------------------------------------------------|---------------------------------|-------------|
| 0  | `Version`                     | 0      | -                                                         | -                               | **STUB**    |
| 1  | `Clear_Storage`               | 0      | -                                                         | ACK/NACK                        | Implemented |
| 2  | `Set_Ssid`                    | 2      | `[4B name_len][name][4B pwd_len][pwd]`                    | ACK                             | Implemented |
| 3  | `Get_Ssid_Names`              | 0      | -                                                         | DATA: comma-separated names     | Implemented |
| 4  | `Get_Ssid_Passwords`          | 0      | -                                                         | DATA: comma-separated passwords | Implemented |
| 5  | `Clear_Ssids`                 | 0      | -                                                         | ACK                             | Implemented |
| 6  | `Set_Moq_Url`                 | 1      | UTF-8 string                                              | ACK                             | Implemented |
| 7  | `Get_Moq_Url`                 | 0      | -                                                         | DATA: URL string                | Implemented |
| 20 | `Get_Loopback`                | 0      | -                                                         | DATA: u8 mode (0=off, 1=raw)    | Implemented |
| 21 | `Set_Loopback`                | 1      | u8 mode (0=off, 1=raw)                                    | ACK/NACK                        | Implemented |
| 22 | `Get_Logs_Enabled`            | 0      | -                                                         | DATA: u8 (0=disabled, 1=enabled)| Implemented |
| 23 | `Set_Logs_Enabled`            | 1      | u8 (0=disabled, 1=enabled)                                | ACK                             | Implemented |
| 13 | `Set_Language`                | 1      | Language tag (e.g. `en-US`)                               | ACK/NACK                        | Implemented |
| 14 | `Get_Language`                | 0      | -                                                         | DATA: language tag              | Implemented |
| 15 | `Set_Channel`                 | 1      | JSON array: `["relay","org","channel","ptt"]`             | ACK/NACK                        | Implemented |
| 16 | `Get_Channel`                 | 0      | -                                                         | DATA: JSON array                | Implemented |
| 17 | `Set_AI`                      | 1      | JSON object: `{"query":[...],"audio":[...],"cmd":[...]}` | ACK/NACK                        | Implemented |
| 18 | `Get_AI`                      | 0      | -                                                         | DATA: JSON object               | Implemented |
| 19 | `Burn_Disable_USB_JTag_Efuse` | 0      | -                                                         | ACK/NACK                        | Implemented |

### Response Types

| Code     | Type | Description          |
|----------|------|----------------------|
| `0x8000` | ACK  | Success, no data     |
| `0x8001` | NACK | Failure              |
| `0x8002` | DATA | Success with payload |


## UI Firmware Commands (via MGMT link)

| ID | Command        | Params | Request Payload    | Response                       | Status               |
|----|----------------|--------|--------------------|--------------------------------|----------------------|
| 0  | `version`      | 0      | -                  | -                              | **STUB**             |
| 1  | `clear_config` | 0      | -                  | -                              | Implemented (no ACK) |
| 2  | `set_sframe`   | 1      | 16-byte SFrame key | ACK/NACK                       | Implemented          |
| 3  | `get_sframe`   | 0      | -                  | DATA: 16-byte key (or nothing) | Implemented          |
| 4  | `toggle_logs`  | 0      | -                  | ACK                            | Implemented          |
| 5  | `disable_logs` | 0      | -                  | ACK                            | Implemented          |
| 6  | `enable_logs`  | 0      | -                  | ACK                            | Implemented          |


---

# Proposed Unified Protocol (Link-style)

This section defines a unified protocol toward which both hactar and Link codebases can migrate.
The design follows Link's architecture: TLV encoding with distinct message types for each direction.

## Design Principles

1. **TLV encoding** with sync word header for reliable framing
2. **Separate enums per direction** (e.g., `CtlToNet`, `NetToCtl`) starting at different bases
3. **snake_case naming** throughout
4. **Ping/pong for liveness** instead of unimplemented version commands
5. **JSON for structured data** (WiFi credentials, channel namespaces, AI config)
6. **Explicit get/set pairs** instead of toggle commands
7. **Typed responses** matching request types (not generic ACK/NACK/DATA)

## Message Type Bases

| Direction | Base   | Description                |
|-----------|--------|----------------------------|
| CtlToMgmt | 0x0000 | Host → MGMT chip           |
| MgmtToCtl | 0x0100 | MGMT chip → Host           |
| CtlToUi   | 0x0200 | Host → UI chip (via MGMT)  |
| UiToCtl   | 0x0300 | UI chip → Host (via MGMT)  |
| CtlToNet  | 0x0400 | Host → NET chip (via MGMT) |
| NetToCtl  | 0x0500 | NET chip → Host (via MGMT) |
| UiToNet   | 0x0600 | UI chip → NET chip (audio) |
| NetToUi   | 0x0700 | NET chip → UI chip (audio) |

## CtlToNet (0x0400)

Commands from host to NET chip.

| ID     | Name               | Payload                                           | Response Type    |
|--------|--------------------|---------------------------------------------------|------------------|
| 0x0400 | `ping`             | arbitrary bytes (echoed back)                     | `pong`           |
| 0x0401 | `circular_ping`    | arbitrary bytes (forwarded around ring)           | `circular_ping`  |
| 0x0402 | `add_wifi`         | JSON: `{"ssid":"...","password":"..."}`           | `ack`            |
| 0x0403 | `get_wifi`         | -                                                 | `wifi_list`      |
| 0x0404 | `clear_wifi`       | -                                                 | `ack`            |
| 0x0405 | `get_relay_url`    | -                                                 | `relay_url`      |
| 0x0406 | `set_relay_url`    | UTF-8 string                                      | `ack`            |
| 0x0407 | `get_loopback`     | -                                                 | `loopback`       |
| 0x0408 | `set_loopback`     | u8: 0=off, 1=raw, 2=moq                           | `ack`            |
| 0x0409 | `get_logs_enabled` | -                                                 | `logs_enabled`   |
| 0x040A | `set_logs_enabled` | u8: 0=disabled, 1=enabled                         | `ack`            |
| 0x040B | `get_language`     | -                                                 | `language`       |
| 0x040C | `set_language`     | UTF-8 string (e.g. "en-US")                       | `ack` / `error`  |
| 0x040D | `get_channel`      | -                                                 | `channel`        |
| 0x040E | `set_channel`      | JSON array: `["relay","org",...]`                 | `ack` / `error`  |
| 0x040F | `get_ai_config`    | -                                                 | `ai_config`      |
| 0x0410 | `set_ai_config`    | JSON: `{"query":[...],"audio":[...],"cmd":[...]}` | `ack` / `error`  |
| 0x0411 | `clear_storage`    | -                                                 | `ack` / `error`  |
| 0x0412 | `burn_jtag_efuse`  | -                                                 | `ack` / `error`  |

## NetToCtl (0x0500)

Responses from NET chip to host.

| ID     | Name            | Payload                                                           |
|--------|-----------------|-------------------------------------------------------------------|
| 0x0500 | `pong`          | echoed ping data                                                  |
| 0x0501 | `circular_ping` | forwarded ping data                                               |
| 0x0502 | `ack`           | -                                                                 |
| 0x0503 | `error`         | UTF-8 error message                                               |
| 0x0504 | `wifi_list`     | JSON array: `[{"ssid":"...","password":"..."},...]`               |
| 0x0505 | `relay_url`     | UTF-8 string                                                      |
| 0x0506 | `loopback`      | u8: 0=off, 1=raw, 2=moq                                           |
| 0x0507 | `logs_enabled`  | u8: 0=disabled, 1=enabled                                         |
| 0x0508 | `language`      | UTF-8 string                                                      |
| 0x0509 | `channel`       | JSON array                                                        |
| 0x050A | `ai_config`     | JSON object                                                       |

## CtlToUi (0x0200)

Commands from host to UI chip.

| ID     | Name               | Payload                            | Response Type    |
|--------|--------------------|------------------------------------| -----------------|
| 0x0200 | `ping`             | arbitrary bytes (echoed back)      | `pong`           |
| 0x0201 | `circular_ping`    | arbitrary bytes (forwarded)        | `circular_ping`  |
| 0x0202 | `get_version`      | -                                  | `version`        |
| 0x0203 | `set_version`      | u32 (big-endian)                   | `ack`            |
| 0x0204 | `get_sframe_key`   | -                                  | `sframe_key`     |
| 0x0205 | `set_sframe_key`   | 16 bytes                           | `ack` / `error`  |
| 0x0206 | `get_loopback`     | -                                  | `loopback`       |
| 0x0207 | `set_loopback`     | u8: 0=off, 1=raw, 2=alaw, 3=sframe | `ack`            |
| 0x0208 | `get_logs_enabled` | -                                  | `logs_enabled`   |
| 0x0209 | `set_logs_enabled` | u8: 0=disabled, 1=enabled          | `ack`            |
| 0x020A | `clear_storage`    | -                                  | `ack` / `error`  |
| 0x020B | `get_stack_info`   | -                                  | `stack_info`     |
| 0x020C | `repaint_stack`    | -                                  | `ack`            |

## UiToCtl (0x0300)

Responses from UI chip to host.

| ID     | Name            | Payload                                                              |
|--------|-----------------|----------------------------------------------------------------------|
| 0x0300 | `pong`          | echoed ping data                                                     |
| 0x0301 | `circular_ping` | forwarded ping data                                                  |
| 0x0302 | `ack`           | -                                                                    |
| 0x0303 | `error`         | UTF-8 error message                                                  |
| 0x0304 | `version`       | u32 (big-endian)                                                     |
| 0x0305 | `sframe_key`    | 16 bytes (or empty if not set)                                       |
| 0x0306 | `loopback`      | u8: 0=off, 1=raw, 2=alaw, 3=sframe                                   |
| 0x0307 | `logs_enabled`  | u8: 0=disabled, 1=enabled                                            |
| 0x0308 | `stack_info`    | JSON: `{"stack_base":n,"stack_top":n,"stack_size":n,"stack_used":n}` |
| 0x0309 | `log`           | UTF-8 log message (async/unsolicited)                                |

## UiToNet (0x0600)

Messages from UI chip to NET chip (audio path).

| ID     | Name            | Payload                                                      |
|--------|-----------------|--------------------------------------------------------------|
| 0x0600 | `circular_ping` | forwarded ping data                                          |
| 0x0601 | `audio_frame`   | `[channel_id: u8][sframe_header][encrypted_chunk][auth_tag]` |

## NetToUi (0x0700)

Messages from NET chip to UI chip (audio path).

| ID     | Name            | Payload                                                      |
|--------|-----------------|--------------------------------------------------------------|
| 0x0700 | `circular_ping` | forwarded ping data                                          |
| 0x0701 | `audio_frame`   | `[channel_id: u8][sframe_header][encrypted_chunk][auth_tag]` |

## Channel IDs

Used in audio frame payloads.

| ID | Name      | Description                      |
|----|-----------|----------------------------------|
| 0  | `ptt`     | Push-to-talk audio (human voice) |
| 1  | `ptt_ai`  | AI audio channel (AI-generated)  |
| 2  | `chat`    | Text chat (reserved)             |
| 3  | `chat_ai` | AI text/JSON responses           |

## Loopback Modes

### UI Loopback Modes

| Value | Name     | Description                                   |
|-------|----------|-----------------------------------------------|
| 0     | `off`    | Normal operation, audio sent to NET           |
| 1     | `raw`    | Before A-law encoding (stereo PCM to speaker) |
| 2     | `alaw`   | After encoding, before SFrame                 |
| 3     | `sframe` | Full encryption round-trip                    |

### NET Loopback Modes

| Value | Name  | Description                                      |
|-------|-------|--------------------------------------------------|
| 0     | `off` | Normal operation, audio to MoQ, filter self-echo |
| 1     | `raw` | Local bypass, audio directly back to UI          |
| 2     | `moq` | Audio to MoQ, don't filter self-echo             |

## TLV Wire Format

```
[SYNC: 2 bytes = 0x4C4B ("LK")]
[TYPE: 2 bytes, little-endian]
[LENGTH: 2 bytes, little-endian]
[VALUE: LENGTH bytes]
```

Maximum value size: 1024 bytes (configurable).

## Migration Notes

### Hactar → Unified

Phases ordered from lowest to highest risk. Each phase includes both firmware and CLI changes.

#### Phase 1: Add debugging commands (additive, no impact)
- Add `get_stack_info`, `repaint_stack` for UI chip
- Add CLI support for new commands
- Pure additions, no existing code affected

#### Phase 2: Add `ping`/`pong`/`circular_ping`, remove `Version` stub
- Add ping/pong at new message type bases
- Add circular_ping (all directions) for round-trip latency testing
- Remove unused `Version` stub (both chips)
- Add CLI support for ping and circular_ping

#### Phase 3: Simple renames (low risk)
- `Set_Moq_Url`/`Get_Moq_Url` → `set_relay_url`/`get_relay_url`
- `set_sframe`/`get_sframe` → `set_sframe_key`/`get_sframe_key`
- Update CLI to use new names

#### Phase 4: Unified WiFi commands
- Add `get_wifi` (JSON), `add_wifi` (JSON), `clear_wifi`
- Remove `Get_Ssid_Names`, `Get_Ssid_Passwords`, `Set_Ssid`, `Clear_Ssids`
- Update CLI to use new commands

#### Phase 5: Unified loopback/logs commands
- Add `get_loopback`/`set_loopback` (with mode enum)
- Add `get_logs_enabled`/`set_logs_enabled`
- Remove `Enable_Loopback`, `Disable_Loopback`
- Remove `Toggle_Logs`, `Enable_Logs`, `Disable_Logs`
- Update CLI to use new commands

#### Phase 6: Typed responses (protocol change)
- Replace generic ACK/NACK/DATA with typed response messages
- Update CLI parser to handle typed responses
- Highest risk: changes fundamental response handling

### Link → Unified

Phases ordered from lowest to highest risk. Each phase includes firmware and CLI changes.

#### Phase 1: Add logs commands (stub, no impact)
- Add `get_logs_enabled` / `set_logs_enabled` to CtlToNet, CtlToUi
- Add `logs_enabled` response types to NetToCtl, UiToCtl
- Stub implementation: always returns enabled, set is no-op
- Add CLI support

#### Phase 2: Add clear_storage (additive, no impact)
- Add `clear_storage` to CtlToNet, CtlToUi
- Add CLI support

#### Phase 3: Add language/channel/AI config (additive, no impact)
- Add `get_language` / `set_language` to CtlToNet
- Add `get_channel` / `set_channel` to CtlToNet
- Add `get_ai_config` / `set_ai_config` to CtlToNet
- Add `language`, `channel`, `ai_config` response types to NetToCtl
- Add CLI support

#### Phase 4: Add burn_jtag_efuse (additive, no impact)
- Add `burn_jtag_efuse` to CtlToNet
- Add CLI support (with confirmation prompt)

#### Phase 5: Remove GetJitterStats (medium risk, removes functionality)
- Remove `GetJitterStats` from CtlToNet
- Remove `JitterStats` from NetToCtl
- Remove CLI support
- Note: Only if jitter debugging no longer needed
