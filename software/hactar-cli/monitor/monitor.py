import sys
import os
import threading
import time
import signal
import numbers

# No support on windows
try:
    import readline
except Exception:
    readline = None

import serial
from hactar_scanning import HactarScanning
from hactar_commands import command_map, bypass_map, ui_command_map, net_command_map


class Monitor:
    def __init__(self, port, uart_args, threaded_reading=True):
        self.uart = serial.Serial(port, **uart_args)
        print(f"\033[92mOpened port: {port}, baudrate={uart_args['baudrate']}\033[0m")

        self.running = True
        self.rx_thread = None

        # Start threading
        if threaded_reading:
            self.rx_thread = threading.Thread(target=self.ReadSerial, args=(True,))
            self.rx_thread.daemon = True
            self.rx_thread.start()

        self.uart.write(command_map["disable logs"])

        self.WriteSerial()

    def ReadSerial(self, threaded=False):
        data = None
        while self.running and threaded:
            time.sleep(0.05)

            if self.uart.in_waiting:
                data = self.uart.readline().decode()
            else:
                data = None
                continue

            if readline:
                saved_input = readline.get_line_buffer()
                # return to start of line and clear current line
                sys.stdout.write("\r\033[K")
                # print log (adds newline)
                print(f"{data}", end="")
                # reprint prompt + saved user text
                sys.stdout.write(f"> {saved_input}")
                sys.stdout.flush()
            else:
                print(data)

    def WriteSerial(self):
        while self.running:
            usr_input = input("> ").lower()
            if usr_input == "exit":
                self.running = False
            else:
                if usr_input in command_map:
                    self.uart.write(command_map[usr_input])
                    continue

                found = False
                for to_whom in bypass_map:
                    if to_whom in usr_input:
                        found = True
                        self.ProcessBypassCommand(to_whom, usr_input[(len(to_whom) + 1) :])
                        break

                if not found:
                    print("Unknown command " + usr_input)

    def ProcessBypassCommand(self, to_whom, usr_input):
        to_whom_id = bypass_map[to_whom]
        command = None
        command_params = None
        if to_whom == "ui":
            for cmd in ui_command_map:
                if cmd in usr_input:
                    command = cmd
                    command_params = ui_command_map[cmd]
                    break
            if command == None:
                print("[ERROR] Malformed command")
                return

            num_params = command_params["num_params"]
            command_id = command_params["id"]
            print(to_whom, to_whom_id)
            print(command, command_id)
            print(command_params)
            params = []
            usr_value = usr_input[len(command) + 1 :]
            print("new usr value", usr_value)

            if len(usr_value) == 0 and num_params > 0:
                print(
                    f"[ERROR] Invalid number of params for command ({to_whom} {command}) expected {num_params} received {len(params)}"
                )
                return

            for i in range(num_params):
                space_idx = usr_value.find(" ")
                if space_idx == -1:
                    # Last param is what is in usr_value
                    params.append(usr_value)
                    if len(params) < num_params:
                        print(
                            f"[ERROR] Invalid number of params for command ({to_whom} {command}) expected {num_params} received {len(params)}"
                        )
                        return
                    break

                # Found a space, so take the
                param = usr_value[:space_idx]
                params.append(param)
                usr_value = usr_value[space_idx + 1]
            print(params)

            Header_Bytes = 5  # 1 type, 4 length
            # Create the length of the mgmt TLV and ui TLV

            command_len = Header_Bytes
            sub_command_len = 0
            # transmit the TLV

            for param in params:
                command_len += len(param)
                sub_command_len += len(param)

        elif to_whom == "net":
            pass

    def Close(self):
        self.running = False
        if self.rx_thread != None:
            self.rx_thread.join()
        self.uart.close()


def SignalHandler(signal, frame):
    print(f"Signal {signal}, in file {frame.f_code.co_filename} line {frame.f_lineno}")
    exit(-1)


def main(args):
    uart_config = {
        "baudrate": 115200,
        "bytesize": serial.EIGHTBITS,
        "parity": serial.PARITY_NONE,
        "stopbits": serial.STOPBITS_ONE,
        "timeout": 2,
    }

    signal.signal(signal.SIGINT, SignalHandler)
    port = args.port
    if port == "" or port == None:
        ports = HactarScanning(uart_config)

        if len(ports) == 0:
            print("No hactars found, exiting")
            return

        idx = -1
        while idx < 0 or idx >= len(ports):
            print(f"Hactars found: {len(ports)}")
            print(f"Select a port [0-{len(ports)-1}]")
            for i, p in enumerate(ports):
                print(f"{i}. {p}")

            idx = input("> ")
            if not idx.isdigit():
                print("Error: not a number entered")
                idx = -1
                continue

            idx = int(idx)

            if idx < 0 or idx >= len(ports):
                print("Invalid selection, try again")
                continue

        port = ports[idx]

    monitor = Monitor(port, uart_config)


if __name__ == "__main__":
    pass
