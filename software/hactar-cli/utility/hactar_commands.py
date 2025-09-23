# TODO finish adding more zeroes to the commands

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
    "sframe": {"id": 1, "num_params": 1},
}

net_command_map = {
    "version": {"id": 0, "num_params": 0},
    "set_ssid_name": {"id": 1, "num_params": 1},
    "set_ssid_password": {"id": 2, "num_params": 1},
    "get_ssid_names": {"id": 3, "num_params": 0},
    "get_ssid_passwords": {"id": 4, "num_params": 0},
}
