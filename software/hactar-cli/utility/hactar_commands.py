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
    "version": {"id": 0, "num_params": 0},
    "clear_config": {"id": 1, "num_params": 0},
    "set_sframe": {"id": 2, "num_params": 1},
    "get_sframe": {"id": 3, "num_params": 0},
    "toggle_logs": {"id": 4, "num_params": 0},
    "disable_logs": {"id": 5, "num_params": 0},
    "enable_logs": {"id": 6, "num_params": 0},
    "get_stack_info": {"id": 7, "num_params": 0},
    "repaint_stack": {"id": 8, "num_params": 0},
}

net_command_map = {
    "version": {"id": 0, "num_params": 0},
    "clear_storage": {"id": 1, "num_params": 0},
    "set_ssid": {"id": 2, "num_params": 2},
    "get_ssid_names": {"id": 3, "num_params": 0},
    "get_ssid_passwords": {"id": 4, "num_params": 0},
    "clear_ssids": {"id": 5, "num_params": 0},
    "set_moq_url": {"id": 6, "num_params": 1},
    "get_moq_url": {"id": 7, "num_params": 0},
    "toggle_logs": {"id": 8, "num_params": 0},
    "disable_logs": {"id": 9, "num_params": 0},
    "enable_logs": {"id": 10, "num_params": 0},
    "disable_loopback": {"id": 11, "num_params": 0},
    "enable_loopback": {"id": 12, "num_params": 0},
    "set_language": {"id": 13, "num_params": 1, "encoder": "language"},
    "get_language": {"id": 14, "num_params": 0},
    "set_channel": {"id": 15, "num_params": 1, "encoder": "json"},
    "get_channel": {"id": 16, "num_params": 0},
    "set_ai": {"id": 17, "num_params": 1, "encoder": "json"},
    "get_ai": {"id": 18, "num_params": 0},
    "burn_efuse": {"id": 19, "num_params": 0},
}

# Supported language tags
SUPPORTED_LANGUAGES = ["en-US", "es-ES", "de-DE", "hi-IN", "nb-NO"]

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

# TLV Response types (matching net_mgmt_link.h)
Response_Ack = 0x8000
Response_Nack = 0x8001
Response_Data = 0x8002

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
