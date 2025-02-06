# This python file has methods to reset the COM ports on the FPGA. 
# It kills all putty sessions and then resets the FTDI adapters.

import subprocess
import ctypes

def is_user_admin():
    print("Checking user privileges . . . ")
    try:
        return ctypes.windll.shell32.IsUserAnAdmin() != 0
    except Exception as e:
        return False, str(e)

# List of hardware IDs for the COM ports for `DH3` machine
com_port_ids = [
    {"com_ports": ["COM4", "COM5"], "hardware_id": "FTDIBUS\COMPORT&VID_0403&PID_6048&MI_00"},
    {"com_ports": ["COM6", "COM11"], "hardware_id": "FTDIBUS\COMPORT&VID_0403&PID_6048&MI_01"},
    {"com_ports": ["COM8", "COM12"], "hardware_id": "FTDIBUS\COMPORT&VID_0403&PID_6048&MI_02"},
    {"com_ports": ["COM7", "COM10"], "hardware_id": "FTDIBUS\COMPORT&VID_0403&PID_6048&MI_03"}
]

# Function to disable a COM port using DevCon
def disable_com_port(hardware_id, com_ports):
    command = f'devcon disable "{hardware_id}"'
    subprocess.run(command, shell=True)
    print(f"Disabled ports: {', '.join(com_ports)}")

# Function to enable a COM port using DevCon
def enable_com_port(hardware_id, com_ports):
    command = f'devcon enable "{hardware_id}"'
    subprocess.run(command, shell=True)
    print(f"Enabled ports: {', '.join(com_ports)}")

# Main function to disable and then enable all COM ports in the list
def toggle_com_ports():
    # Disable all COM ports in the list
    for group in com_port_ids:
        hardware_id = group["hardware_id"]
        com_ports = group["com_ports"]
        disable_com_port(hardware_id, com_ports)  # Disable the COM ports by hardware ID
    print("=" * 40)
    
    # Enable all COM ports in the list
    for group in com_port_ids:
        hardware_id = group["hardware_id"]
        com_ports = group["com_ports"]
        enable_com_port(hardware_id, com_ports)  # Enable the COM ports back by hardware ID

# Call the function when the script is run
if __name__ == "__main__":
    print(" ")
    print(" ")
    is_admin = is_user_admin()

    if is_admin:
        print(" ")
        print("This user has administrative privileges--------------")
        print(" -- Force Resetting all COM Ports")
        print(" ")
        toggle_com_ports()
    else:
        print("This user does not have administrative privileges---------")
        print(" -- Cannot Force Reset COM Ports")
