# Hactar Command Reference

## NET Firmware Commands (via MGMT link)

| ID | Command                       | Params | Request Payload                                           | Response Type                   | Status      |
|----|-------------------------------|--------|-----------------------------------------------------------|---------------------------------|-------------|
| 0  | `Version`                     | 0      | -                                                         | -                               | **STUB**    |
| 1  | `Clear_Storage`               | 0      | -                                                         | ACK/NACK                        | Implemented |
| 2  | `Set_Ssid`                    | 2      | `[4B name_len][name][4B pwd_len][pwd]`                    | ACK                             | Implemented |
| 3  | `Get_Ssid_Names`              | 0      | -                                                         | SSID_NAMES (0x8003)             | Implemented |
| 4  | `Get_Ssid_Passwords`          | 0      | -                                                         | SSID_PASSWORDS (0x8004)         | Implemented |
| 5  | `Clear_Ssids`                 | 0      | -                                                         | ACK                             | Implemented |
| 6  | `Set_Moq_Url`                 | 1      | UTF-8 string                                              | ACK                             | Implemented |
| 7  | `Get_Moq_Url`                 | 0      | -                                                         | MOQ_URL (0x8005)                | Implemented |
| 8  | `Toggle_Logs`                 | 0      | -                                                         | ACK                             | Implemented |
| 9  | `Disable_Logs`                | 0      | -                                                         | ACK                             | Implemented |
| 10 | `Enable_Logs`                 | 0      | -                                                         | ACK                             | Implemented |
| 11 | `Disable_Loopback`            | 0      | -                                                         | ACK                             | Implemented |
| 12 | `Enable_Loopback`             | 0      | -                                                         | ACK                             | Implemented |
| 13 | `Set_Language`                | 1      | Language tag (e.g. `en-US`)                               | ACK/NACK                        | Implemented |
| 14 | `Get_Language`                | 0      | -                                                         | LANGUAGE (0x8006)               | Implemented |
| 15 | `Set_Channel`                 | 1      | JSON array: `["relay","org","channel","ptt"]`             | ACK/NACK                        | Implemented |
| 16 | `Get_Channel`                 | 0      | -                                                         | CHANNEL (0x8007)                | Implemented |
| 17 | `Set_AI`                      | 1      | JSON object: `{"query":[...],"audio":[...],"cmd":[...]}` | ACK/NACK                        | Implemented |
| 18 | `Get_AI`                      | 0      | -                                                         | AI_CONFIG (0x8008)              | Implemented |
| 19 | `Burn_Disable_USB_JTag_Efuse` | 0      | -                                                         | ACK/NACK                        | Implemented |

### Response Types

| Code     | Type             | Description                                   |
|----------|------------------|-----------------------------------------------|
| `0x8000` | ACK              | Success, no data                              |
| `0x8001` | NACK             | Failure                                       |
| `0x8002` | DATA             | Success with payload (deprecated)             |
| `0x8003` | SSID_NAMES       | Comma-separated SSID names                    |
| `0x8004` | SSID_PASSWORDS   | Comma-separated passwords                     |
| `0x8005` | MOQ_URL          | UTF-8 URL string                              |
| `0x8006` | LANGUAGE         | UTF-8 language tag (e.g. "en-US")             |
| `0x8007` | CHANNEL          | JSON array of namespace parts                 |
| `0x8008` | AI_CONFIG        | JSON: `{"query":[...],"audio":[...],"cmd":[...]}` |


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
