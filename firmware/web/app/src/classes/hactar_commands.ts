const commands =
{
    "hello": "WHO ARE YOU",
    "mgmt_upload": "mgmt_upload", // Only here for completion, doesn't send data
    "main_upload": "ui_upload",
    "net_upload": "net_upload",
    "reset": 10,
    "version": 11,
    "debug": 12, // TODO bytes
    "debug_main": 13,
    "debug_net": 14,
    "send_packet": 15,
};

const responses =
{
    "ack": 121,
    "ready": 128,
    "nack": 31,
    "hello_response": "HELLO, I AM A HACTAR DEVICE",
}

export default { commands, responses };