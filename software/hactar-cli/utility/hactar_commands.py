command_map = {
    "version": bytes([0, 0, 0]),
    "who are you": bytes([1, 0, 0]),
    "hard reset": bytes([2, 0, 0]),
    "reset": bytes([3, 0, 0]),
    "reset ui": bytes([4, 0, 0]),
    "reset net": bytes([5, 0, 0]),
    "flash ui": bytes([6, 0, 0]),
    "flash net": bytes([7, 0, 0]),
    "enable logs": bytes([8, 0, 0]),
    "enable ui logs": bytes([9, 0, 0]),
    "enable net logs": bytes([10, 0, 0]),
    "disable logs": bytes([11, 0, 0]),
    "disable ui logs": bytes([12, 0, 0]),
    "disable net logs": bytes([13, 0, 0]),
    "default logging": bytes([14, 0, 0]),
}


bypass_map = {
    "to ui": 15,
    "to net": 16,
    "loopback": 17,
}

ui_command_map = {
    "version": 0
    "sframe": 1,
}

net_command_map = {
    "version": 0
    "set ssid": 1,
    "get ssid": 2,
}
