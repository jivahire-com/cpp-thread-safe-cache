# Copyright (c) Microsoft Corporation. All rights reserved.

import time
import paramiko
from robot.api.deco import keyword

class BmcHelperLibrary:
    def __init__(self, hostname, username="admin", password="admin", port=2200):
        self.hostname = hostname
        self.username = username
        self.password = password
        self.port = port
        self.client = None

    def connect(self):
        """Establish an SSH connection to the BMC host."""
        try:
            self.client = paramiko.SSHClient()
            self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            self.client.connect(hostname=self.hostname, port=self.port, username=self.username, password=self.password, timeout=900)
            print(f"Connected to {self.hostname}")
        except Exception as e:
            print(f"Failed to connect to {self.hostname}: {e}")
            raise AssertionError

    def disconnect(self):
        """Close the SSH connection."""
        if self.client:
            self.client.close()
            print(f"Disconnected from {self.hostname}")

    def execute_command(self, command):
        """Execute a command on the BMC host."""
        self.connect()

        print(f"Executing command: {command}")
        stdin, stdout, stderr = self.client.exec_command(f"echo 0penBmc | sudo -S su -c '{command}'")
        print(f"Command output: {stdout.read().decode('latin-1')}")
        print(f"Command error: {stderr.read().decode('latin-1')}")

        self.disconnect()

        return stdout.read().decode('latin-1'), stderr.read().decode('latin-1')

    @keyword
    def soc_reset(self):
        """Perform a cold reset of the system"""
        command = "ipmitool power cycle"
        stdout, stderr = self.execute_command(command)
        if stderr:
            print(f"Error during power cycle: {stderr}")
            raise AssertionError
        else:
            print("Power cycle command executed successfully.")

        time.sleep(15)

        command = "ipmitool power on"
        stdout, stderr = self.execute_command(command)
        if stderr:
            print(f"Error during power on: {stderr}")
            raise AssertionError
        else:
            print("Power on command executed successfully.")

    @keyword
    def set_profile(self, profile:str):
        """Set the BMC profile."""
        if profile not in ["General", "Compute"]:
            raise ValueError("Invalid profile. Choose from 'General' or 'Compute'.")

        print(f"Setting BMC profile to {profile}...")
        if profile == "General":
            command = f"ipmitool raw 0x38 0x74 0x00"
        elif profile == "Compute":
            command = f"ipmitool raw 0x38 0x74 0x01"

        stdout, stderr = self.execute_command(command)
        if stderr:
            print(f"Error setting profile: {stderr}")
            raise AssertionError
        else:
            print(f"Profile set to {profile} successfully.")

    @keyword
    def set_boot_option(self, option:str):
        """Set the boot option for the current boot. Boot-order will be reset to default after cold reset"""
        if option not in ["Pxe", "Nvme", "ConfApp", "Usb"]:
            raise ValueError("Invalid boot option. Choose from 'Pxe', 'Nvme', 'ConfApp' or 'Usb'.")

        print(f"Setting boot option to {option}...")
        if option == "Pxe":
            command = "ipmitool raw 0x00 0x08 0x05 0xA0 0x04 0x00 0x00 0x00"
        elif option == "Nvme":
            command = "ipmitool raw 0x00 0x08 0x05 0xA0 0x08 0x00 0x00 0x00"
        elif option == "ConfApp":
            command = "ipmitool raw 0x00 0x08 0x05 0xA0 0x18 0x00 0x00 0x00"
        elif option == "Usb":
            command = "ipmitool raw 0x00 0x08 0x05 0xA0 0x3C 0x00 0x00 0x00"

        stdout, stderr = self.execute_command(command)
        if stderr:
            print(f"Error setting boot option: {stderr}")
            raise AssertionError
        else:
            print(f"Boot option set to {option} successfully.")

        command = "devmem 0xf0800270 32 0x48AF8142"
        stdout, stderr = self.execute_command(command)
        if stderr:
            print(f"Error setting TX enable APNS serial: {stderr}")
            raise AssertionError
        else:
            print(f"Boot option set to {option} successfully.")

    def __del__(self):
        """Ensure the connection is closed when the object is deleted."""
        self.disconnect()