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
    "to ui": bytes([15, 0, 0]),
    "to net": bytes([16, 0, 0]),
    "loopback": bytes([17, 0, 0]),
}
