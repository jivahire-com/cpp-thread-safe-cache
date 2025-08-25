import time
import paramiko
from robot.api.deco import keyword

class RscmHelperLibrary:
    def __init__(self, rm_host, rm_user, rm_password, bmc_host, bmc_user, bmc_password, node=25, port=2200):
        self.rm_host = rm_host
        self.rm_username = rm_user
        self.rm_password = rm_password
        self.bmc_host = bmc_host
        self.bmc_username = bmc_user
        self.bmc_password = bmc_password
        self.node = node
        self.port = port
        self.client = None
        self.tunnel_client = None

    def connect(self):
        """Establish an SSH connection to the BMC host."""
        try:
            self.client = paramiko.SSHClient()
            self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            self.client.connect(hostname=self.rm_host, username=self.rm_username, password=self.rm_password, timeout=900)
            print(f"Connected to {self.rm_host}")
        except Exception as e:
            print(f"Failed to connect to {self.rm_host}: {e}")
            raise AssertionError

    def disconnect(self):
        """Close the SSH connection."""
        if self.client:
            self.client.close()
            print(f"Disconnected from {self.rm_host}")

    def create_tunnel_connection(self, tunnel_host, tunnel_user="root", tunnel_password=None):
        """Create tunnel connection to BMC through jump host"""
        try:
            # Connect to tunnel host
            self.tunnel_client = paramiko.SSHClient()
            self.tunnel_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

            if tunnel_password:
                self.tunnel_client.connect(tunnel_host, username=tunnel_user, password=tunnel_password, timeout=30)
            else:
                self.tunnel_client.connect(tunnel_host, username=tunnel_user, timeout=30)

            print(f"Connected to tunnel host {tunnel_host}")
            return True

        except Exception as e:
            print(f"Failed to connect to tunnel host: {e}")
            return False

    def close_tunnel_connection(self):
        """Close tunnel connection"""
        if self.tunnel_client:
            self.tunnel_client.close()
            self.tunnel_client = None
            print("Tunnel connection closed")

    def execute_rm_command(self, command):
        """Execute a command on the BMC host."""
        self.connect()

        print(f"Executing command: {command}")
        stdin, stdout, stderr = self.client.exec_command(f"{command}")
        print(f"Command output: {stdout.read().decode('latin-1')}")
        print(f"Command error: {stderr.read().decode('latin-1')}")

        self.disconnect()

        return stdout.read().decode('latin-1'), stderr.read().decode('latin-1')

    def execute_bmc_command(self, command):
        """Execute a command on the BMC host via SSH tunnel."""
        try:
            # Create tunnel connection
            if not self.create_tunnel_connection(self.rm_host, tunnel_user=self.rm_username, tunnel_password=self.rm_password):
                raise Exception("Failed to create tunnel connection")

            # Execute command through tunnel using SSH ProxyCommand-like approach
            transport = self.tunnel_client.get_transport()

            # Open channel to final destination through tunnel
            channel = transport.open_channel(
                'direct-tcpip',
                (self.bmc_host, self.port),  # target BMC host and port
                ('127.0.0.1', 8022)          # localhost and port (can be any unused port)
            )

            # Create SSH client through the channel
            bmc_client = paramiko.SSHClient()
            bmc_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

            # Connect through the channel
            bmc_client.connect(
                sock=channel,
                hostname=self.bmc_host,
                username=self.bmc_username,
                password=self.bmc_password,
                timeout=30
            )

            print("Connected to BMC through tunnel")

            # Execute the command
            print(f"Executing command: {command}")
            stdin, stdout, stderr = bmc_client.exec_command(f"echo 0penBmc | sudo -S su -c '{command}'")
            command_output = stdout.read().decode('latin-1')
            command_error = stderr.read().decode('latin-1')
            print(f"Command output: {command_output}")
            if command_error:
                print(f"Command error: {command_error}")

            bmc_client.close()
            return command_output, command_error

        except Exception as e:
            print(f"Error executing BMC command: {e}")
            raise

        finally:
            # Clean up tunnel connection
            self.close_tunnel_connection()

    @keyword
    def rscm_soc_reset(self):
        """Perform a cold reset of the system"""
        command = f"set system reset -i {self.node}"
        stdout, stderr = self.execute_rm_command(command)
        if stderr:
            print(f"Error during power cycle: {stderr}")
            raise AssertionError
        else:
            print("Power cycle command executed successfully.")

        time.sleep(10)

        """Power on the system after reset"""
        command = f"set system on -i {self.node}"
        stdout, stderr = self.execute_rm_command(command)
        if stderr:
            print(f"Error during power on: {stderr}")
            raise AssertionError
        else:
            print("Power on command executed successfully.")

    @keyword
    def rscm_set_profile(self, profile:str):
        """Set the BMC profile."""
        if profile not in ["General", "Compute"]:
            raise ValueError("Invalid profile. Choose from 'General' or 'Compute'.")

        print(f"Setting BMC profile to {profile}...")
        if profile == "General":
            command = f"set system cmd -i {self.node} -c raw 0x38 0x74 0x00"
        elif profile == "Compute":
            command = f"set system cmd -i {self.node} -c raw 0x38 0x74 0x01"

        stdout, stderr = self.execute_rm_command(command)
        if stderr:
            print(f"Error setting profile: {stderr}")
            raise AssertionError
        else:
            print(f"Profile set to {profile} successfully.")

    @keyword
    def rscm_set_boot_option(self, option:str):
        """Set the boot option for the current boot. Boot-order will be reset to default after cold reset"""
        if option not in ["Pxe", "Nvme", "ConfApp", "Usb"]:
            raise ValueError("Invalid boot option. Choose from 'Pxe', 'Nvme', 'ConfApp' or 'Usb'.")

        print(f"Setting boot option to {option}...")
        if option == "Pxe":
            command = f"set system cmd -i {self.node} -c raw 0x00 0x08 0x05 0xA0 0x04 0x00 0x00 0x00"
        elif option == "Nvme":
            command = f"set system cmd -i {self.node} -c raw 0x00 0x08 0x05 0xA0 0x08 0x00 0x00 0x00"
        elif option == "ConfApp":
            command = f"set system cmd -i {self.node} -c raw 0x00 0x08 0x05 0xA0 0x18 0x00 0x00 0x00"
        elif option == "Usb":
            command = f"set system cmd -i {self.node} -c raw 0x00 0x08 0x05 0xA0 0x3C 0x00 0x00 0x00"

        stdout, stderr = self.execute_rm_command(command)
        if stderr:
            print(f"Error setting boot option: {stderr}")
            raise AssertionError
        else:
            print(f"Boot option set to {option} successfully.")

        command = "devmem 0xf0800270 32 0x48AF8142"
        stdout, stderr = self.execute_rm_command(command)
        if stderr:
            print(f"Error setting boot option: {stderr}")
            raise AssertionError
        else:
            print(f"Boot option set to {option} successfully.")

    @keyword
    def rscm_enable_apns_uart(self):
        """Enable APNS Uart."""

        print(f"Switching MUX in BMX to enable APNS Uart...")
        command = f"devmem 0xf0800270 32 0x48AF8142"

        stdout, stderr = self.execute_bmc_command(command)
        print(f"APNS Uart enabled successfully.")

    def __del__(self):
        """Ensure the connection is closed when the object is deleted."""
        try:
            self.close_tunnel_connection()
        except:
            pass

        try:
            self.disconnect()
        except:
            pass