#!/usr/bin/env python3

import argparse
import os
import sys
import subprocess

GB = "\033[32;1m"
RB = "\033[31;1m"
ENDC = "\033[m"

expected_tag = "5.4.2"
user = os.environ.get("SUDO_USER")

user_home = f"/Users/{user}"


def green_print(str):
    print(f"{GB}{str}{ENDC}")


def RunAsUser(username, command):
    try:
        subprocess.check_call(f"sudo -u {user} {command}", shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {e}")


def ElevatePrivileges():
    if os.geteuid() != 0:
        print(f"{RB}Not running as root. Re-running with sudo!{ENDC}")
        try:
            subprocess.check_call(["sudo", sys.executable] + sys.argv)
        except subprocess.CalledProcessError as e:
            print(f"{RB}Failed to gain elevated privileges{ENDC}")
            sys.exit(1)
        sys.exit(0)
    else:
        green_print("Already have elevated privileges")


def BaseProgramsInstall(args):
    green_print("Install base programs")
    subprocess.check_call(f"brew install make openocd libusb", shell=True)


def STMProgramsInstall(args):
    green_print("Installing required software for STM chips")
    subprocess.check_call(
        f"brew install gcc-arm-embedded stlink",
        shell=True,
    )


def ESPProgramsInstall(args):
    green_print("Installing required software for ESP32 chips")
    subprocess.check_call(
        f"brew install cmake ninja dfu-util",
        shell=True,
    )

    green_print("Cloning esp-idf")

    esp_path = f"{user_home}/esp-idf"
    if args.esp_idf_install_path != "":
        esp_path = args.esp_idf_install_path

    subprocess.run(
        f"git clone -b v5.4.2 --recursive https://github.com/espressif/esp-idf.git {esp_path} > /dev/null 2>&1",
        stdout=subprocess.PIPE,
        shell=True,
    )

    green_print("Verifying esp repo")

    result = subprocess.run(
        f"git -C {esp_path} remote get-url origin",
        stdout=subprocess.PIPE,
        text=True,
        shell=True,
    )
    origin = str(result.stdout).strip()
    if origin != "https://github.com/espressif/esp-idf.git":
        raise Exception(
            f"{RB}ERROR- esp path repo is not from espressif - {esp_path}{ENDC}"
        )

    result = subprocess.run(
        f"git -C {esp_path} show --summary | grep 5.4.2",
        stdout=subprocess.PIPE,
        text=True,
        shell=True,
    )

    # print(result.stdout)
    summary = str(result.stdout)
    tag = summary[summary.find(expected_tag) :].strip()

    if tag != expected_tag:
        raise Exception(
            f"{RB}ERROR- expected tag: {expected_tag}, actual tag: {tag}{ENDC}"
        )

    green_print("Install ESP32 environment")
    RunAsUser(user, f"{esp_path}/install.sh all")

    green_print("Adding alias 'idf-get' to .zprofile")
    with open(f"{user_home}/.zprofile", "a") as file:
        file.write(f'alias idf-get=" . {esp_path}/export.sh"')

    green_print("Done, make sure to open a new terminal or source your .zprofile")


def main():
    global all_yes
    try:
        parser = argparse.ArgumentParser()

        parser.add_argument(
            "--esp_idf_install_path",
            help="Alternate esp idf installation directory",
            default="",
            required=False,
        )

        args = parser.parse_args()

        BaseProgramsInstall(args)
        STMProgramsInstall(args)
        ESPProgramsInstall(args)
    except Exception as ex:
        print(ex)


if __name__ == "__main__":
    ElevatePrivileges()
    main()
