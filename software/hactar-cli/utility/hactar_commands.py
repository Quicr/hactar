import readline
import sys

command_map = {
    "version": bytes([0] + [0] * 4),
    "who are you": bytes([1] + [0] * 4),
    "hard reset": bytes([2] + [0] * 4),
    "reset": bytes([3] + [0] * 4),
    "reset ui": bytes([4] + [0] * 4),
    "reset net": bytes([5] + [0] * 4),
    "flash ui": bytes([6] + [0] * 4),
    "flash net": bytes([7] + [0] * 4),
    "enable logs": bytes([8] + [0] * 4),
    "enable ui logs": bytes([9] + [0] * 4),
    "enable net logs": bytes([10] + [0] * 4),
    "disable logs": bytes([11] + [0] * 4),
    "disable ui logs": bytes([12] + [0] * 4),
    "disable net logs": bytes([13] + [0] * 4),
    "default logging": bytes([14] + [0] * 4),
}


bypass_map = {
    "ui": 15,
    "net": 16,
    "loopback": 17,
}

ui_command_map = {
    "version": {"id": 0, "num_params": 0},
    "clear_config": {"id": 1, "num_params": 0},
    "set_sframe": {"id": 2, "num_params": 1},
    "get_sframe": {"id": 3, "num_params": 0},
    "toggle_logs": {"id": 4, "num_params": 0},
    "disable_logs": {"id": 5, "num_params": 0},
    "enable_logs": {"id": 6, "num_params": 0},
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
}

ST_Ack = 0x79
Reply_Ok = 0x80
Reply_Ready = 0x81
Reply_Ack = bytes([0x82])
Reply_Nack = bytes([0x83])


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
