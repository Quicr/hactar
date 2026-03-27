import readline
import struct
import sys

import serial

Link_Sync_Word = bytes([0x4C, 0x49, 0x4E, 0x4B])

def make_tlv(type_id: int) -> bytes:
    """Create a Link TLV packet: sync_word (4) + type (2) + length (4) + payload"""

    return Link_Sync_Word + struct.pack("<H", type_id) + struct.pack("<I", 0) 

command_map = {
    "version": make_tlv(0),
    "who are you": make_tlv(1),
    "hard reset": make_tlv(2),
    "reset": make_tlv(3),
    "reset ui": make_tlv(4),
    "reset net": make_tlv(5),
    "stop ui": make_tlv(6),
    "stop net": make_tlv(7),
    "flash ui": make_tlv(8),
    "flash net": make_tlv(9),
    "enable logs": make_tlv(10),
    "enable ui logs": make_tlv(11),
    "enable net logs": make_tlv(12),
    "disable logs": make_tlv(13),
    "disable ui logs": make_tlv(14),
    "disable net logs": make_tlv(15),
    "default logging": make_tlv(16),
}

bypass_map = {
    "ui": 17,
    "net": 18,
    "loopback": 19,
}

ui_command_map = {
    "ping": {"id": 0, "num_params": 0},  # Replaces version stub - echoes payload
    "clear_config": {"id": 1, "num_params": 0},
    "set_sframe_key": {"id": 2, "num_params": 1, "encoder": "hex"},
    "get_sframe_key": {"id": 3, "num_params": 0},
    "toggle_logs": {"id": 4, "num_params": 0},
    "disable_logs": {"id": 5, "num_params": 0},
    "enable_logs": {"id": 6, "num_params": 0},
    "get_stack_info": {"id": 7, "num_params": 0},
    "repaint_stack": {"id": 8, "num_params": 0},
    "get_loopback": {"id": 9, "num_params": 0},
    "set_loopback": {"id": 10, "num_params": 1, "encoder": "ui_loopback"},  # off/raw/alaw/sframe
    "get_logs_enabled": {"id": 11, "num_params": 0},
    "set_logs_enabled": {"id": 12, "num_params": 1, "encoder": "bool"},  # 0=disabled, 1=enabled
}

net_command_map = {
    "version": {"id": 0, "num_params": 0},
    "clear_storage": {"id": 1, "num_params": 0},
    "add_wifi": {"id": 2, "num_params": 1, "encoder": "json"},  # JSON: {"ssid":"...","password":"..."}
    "get_wifi": {"id": 3, "num_params": 0},  # Returns JSON array
    "clear_wifi": {"id": 4, "num_params": 0},
    "set_relay_url": {"id": 5, "num_params": 1},
    "get_relay_url": {"id": 6, "num_params": 0},
    "get_loopback": {"id": 7, "num_params": 0},
    "set_loopback": {"id": 8, "num_params": 1, "encoder": "loopback"},  # off/raw/moq
    "get_logs_enabled": {"id": 9, "num_params": 0},
    "set_logs_enabled": {"id": 10, "num_params": 1, "encoder": "bool"},  # 0=disabled, 1=enabled
    "set_language": {"id": 11, "num_params": 1, "encoder": "language"},
    "get_language": {"id": 12, "num_params": 0},
    "set_channel": {"id": 13, "num_params": 1, "encoder": "json"},
    "get_channel": {"id": 14, "num_params": 0},
    "set_ai": {"id": 15, "num_params": 1, "encoder": "json"},
    "get_ai": {"id": 16, "num_params": 0},
    "burn_efuse": {"id": 17, "num_params": 0},
}

# Supported language tags
SUPPORTED_LANGUAGES = ["en-US", "es-ES", "de-DE", "hi-IN", "nb-NO"]

# NET loopback modes (matches NetLoopbackMode enum)
NET_LOOPBACK_MODES = {"off": 0, "raw": 1, "moq": 2}

# UI loopback modes (matches UiLoopbackMode enum)
UI_LOOPBACK_MODES = {"off": 0, "raw": 1, "alaw": 2, "sframe": 3}

def is_valid_language(lang: str) -> bool:
    return lang in SUPPORTED_LANGUAGES

def encode_command_payload(encoder: str | None, params: list[str]) -> tuple[bytes, str | None]:
    """Encode command parameters based on encoder type.

    Returns (payload_bytes, error_message).
    If error_message is not None, encoding failed.
    """
    import json

    if encoder == "language":
        lang = params[0]
        if not is_valid_language(lang):
            return bytes(), f"Invalid language '{lang}'. Supported: {', '.join(SUPPORTED_LANGUAGES)}"
        return lang.encode("utf-8"), None

    elif encoder == "json":
        # Send JSON string as-is (validate it parses)
        try:
            json.loads(params[0])  # Validate JSON
            return params[0].encode("utf-8"), None
        except json.JSONDecodeError as e:
            return bytes(), f"Invalid JSON: {e}"

    elif encoder == "hex":
        # Decode hex string to bytes
        try:
            return bytes.fromhex(params[0]), None
        except ValueError as e:
            return bytes(), f"Invalid hex: {e}"

    elif encoder == "loopback":
        # NET loopback mode: off/raw/moq -> 0/1/2
        mode = params[0].lower()
        if mode not in NET_LOOPBACK_MODES:
            return bytes(), f"Invalid loopback mode '{mode}'. Use: off, raw, moq"
        return bytes([NET_LOOPBACK_MODES[mode]]), None

    elif encoder == "ui_loopback":
        # UI loopback mode: off/raw/alaw/sframe -> 0/1/2/3
        mode = params[0].lower()
        if mode not in UI_LOOPBACK_MODES:
            return bytes(), f"Invalid loopback mode '{mode}'. Use: off, raw, alaw, sframe"
        return bytes([UI_LOOPBACK_MODES[mode]]), None

    elif encoder == "bool":
        # Boolean: 0/1/false/true -> 0x00/0x01
        val = params[0].lower()
        if val in ("0", "false", "off", "no"):
            return bytes([0]), None
        elif val in ("1", "true", "on", "yes"):
            return bytes([1]), None
        else:
            return bytes(), f"Invalid boolean '{val}'. Use: 0, 1, true, false"

    else:
        # Default encoding: length-prefixed strings if multiple params
        payload = bytes()
        for param in params:
            if len(params) > 1:
                payload += len(param).to_bytes(4, byteorder="little")
            payload += param.encode("utf-8")
        return payload, None

ST_Ack = 0x79
Reply_Ok = 0x80
Reply_Ready = 0x81
Reply_Ack = bytes([0x82])
Reply_Nack = bytes([0x83])

# TLV Response types (matching net_mgmt_link.h / ui_mgmt_link.h)
Response_Ack = 0x8000
Response_Error = 0x8001

# NET typed responses (matching net_mgmt_link.h)
Response_WifiSsids = 0x8002      # JSON array: [{"ssid":"...","password":"..."},...]
Response_RelayUrl = 0x8003      # UTF-8 URL string
Net_Response_Loopback = 0x8004  # 1 byte: loopback mode enum
Net_Response_LogsEnabled = 0x8005  # 1 byte: 0=disabled, 1=enabled
Response_Language = 0x8006      # UTF-8 language tag (e.g. "en-US")
Response_Channel = 0x8007       # JSON array of namespace parts
Response_AiConfig = 0x8008      # JSON: {"query":[...],"audio":[...],"cmd":[...]}

# UI typed responses (matching ui_mgmt_link.h)
Response_SframeKey = 0x8002        # 16 bytes: SFrame encryption key
Ui_Response_Loopback = 0x8003     # 1 byte: UiLoopbackMode
Ui_Response_LogsEnabled = 0x8004  # 1 byte: 0=disabled, 1=enabled
Response_StackInfo = 0x8005       # JSON: {"stack_base":...,"stack_top":...,"stack_size":...,"stack_used":...}

# Aliases for backwards compatibility
Response_Loopback = Net_Response_Loopback
Response_LogsEnabled = Net_Response_LogsEnabled

# Chip-specific response names
NET_RESPONSE_NAMES = {
    Response_Ack: "ACK",
    Response_Error: "ERROR",
    Response_WifiSsids: "WIFI",
    Response_RelayUrl: "RELAY_URL",
    Net_Response_Loopback: "LOOPBACK",
    Net_Response_LogsEnabled: "LOGS_ENABLED",
    Response_Language: "LANGUAGE",
    Response_Channel: "CHANNEL",
    Response_AiConfig: "AI_CONFIG",
}

UI_RESPONSE_NAMES = {
    Response_Ack: "ACK",
    Response_Error: "ERROR",
    Response_SframeKey: "SFRAME_KEY",
    Ui_Response_Loopback: "LOOPBACK",
    Ui_Response_LogsEnabled: "LOGS_ENABLED",
    Response_StackInfo: "STACK_INFO",
}

# Generic mapping (uses NET codes, for backwards compatibility)
RESPONSE_TYPE_NAMES = NET_RESPONSE_NAMES

def is_data_response(response_type: int) -> bool:
    """Check if response type carries data payload (anything except Ack/Error)."""
    return response_type >= 0x8002


def build_chip_command(chip: str, command: str, params: list[str] = None) -> tuple[bytes, str | None]:
    """Build a TLV command packet for NET or UI chip.

    Args:
        chip: "net" or "ui"
        command: Command name (e.g., "get_wifi", "get_loopback")
        params: Optional list of string parameters

    Returns:
        (packet_bytes, error_message). If error_message is not None, building failed.
    """
    if params is None:
        params = []

    chip_commands = net_command_map if chip == "net" else ui_command_map

    if command not in chip_commands:
        return b"", f"Unknown command '{command}' for {chip}"

    cmd_info = chip_commands[command]
    command_id = cmd_info["id"]
    num_params = cmd_info["num_params"]
    encoder = cmd_info.get("encoder")

    if len(params) < num_params:
        return b"", f"Not enough parameters for {command}: expected {num_params}, got {len(params)}"
    if len(params) > num_params:
        return b"", f"Too many parameters for {command}: expected {num_params}, got {len(params)}"

    # Encode payload
    payload, error = encode_command_payload(encoder, params)
    if error:
        return b"", error

    # Build inner TLV (to chip)
    inner = Link_Sync_Word
    inner += struct.pack("<H", command_id)
    inner += struct.pack("<I", len(payload))
    inner += payload

    # Build outer TLV (to MGMT, routing to chip)
    outer = Link_Sync_Word
    outer += struct.pack("<H", bypass_map[chip])
    outer += struct.pack("<I", len(inner))
    outer += inner

    return bytes(outer), None


def read_tlv_response(uart: serial.Serial, timeout: float = 2.0) -> tuple[int | None, bytes]:
    """Read a TLV response from the device.

    Scans for sync word, then reads type and payload.

    Args:
        uart: Serial port
        timeout: Read timeout in seconds

    Returns:
        (response_type, payload) or (None, b"") on timeout/error.
    """
    import time

    original_timeout = uart.timeout
    uart.timeout = timeout
    start = time.time()

    try:
        # Scan for sync word
        sync_buffer = b""
        while time.time() - start < timeout:
            byte = uart.read(1)
            if not byte:
                continue
            sync_buffer += byte
            if len(sync_buffer) > 4:
                sync_buffer = sync_buffer[-4:]
            if sync_buffer == Link_Sync_Word:
                break
        else:
            return None, b""

        # Read header: type (2 bytes) + length (4 bytes)
        header = uart.read(6)
        if len(header) < 6:
            return None, b""

        msg_type, msg_len = struct.unpack("<HI", header)

        # Read payload
        payload = b""
        if msg_len > 0:
            payload = uart.read(msg_len)

        return msg_type, payload
    finally:
        uart.timeout = original_timeout


def hactar_send_command(uart: serial.Serial, command: bytes, max_attempts: int = 5):
    uart.write(command)

    return hactar_get_ack(uart, max_attempts)

def hactar_get_ack(uart: serial.Serial, max_attempts: int = 5):
    got_ack = False
    attempts = 0
    while True:
        data = uart.read(1)

        if data == bytes([]):
            attempts += 1

        if attempts >= max_attempts:
            return got_ack

        if data == Reply_Ack:
            got_ack = True
            break
        elif data == Reply_Nack:
            got_ack = False
            break

    return got_ack

def hactar_command_completer(text, state):
    buffer = readline.get_line_buffer()
    tokens = buffer.split()

    options = []

    if len(tokens) == 0 or (len(tokens) == 1 and not buffer.endswith(" ")):
        # complete from command_map and bypass_map keys
        options = [
            cmd
            for cmd in list(command_map.keys()) + list(bypass_map.keys())
            if cmd.startswith(text)
        ]
    elif tokens[0] == "ui":
        options = [cmd for cmd in ui_command_map if cmd.startswith(text)]
    elif tokens[0] == "net":
        options = [cmd for cmd in net_command_map if cmd.startswith(text)]
    elif tokens[0] == "loopback":
        options = []  # nothing to complete here
    else:
        options = []

    if state < len(options):
        return options[state]
    return None

def hactar_command_print_matches(substitution, matches, longest_match_length):
    print()
    print("Available commands: ")
    for m in matches:
        print(f"- {m}")
    # reprint prompt + current buffer
    print("> " + readline.get_line_buffer(), end="", flush=True)
