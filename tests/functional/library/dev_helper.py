
# Copyright (c) Microsoft Corporation. All rights reserved.
#
# helpful starting point when developing a library interfacing with CLI
# Start SVP and leave running, then run this script to send commands to SCP telnet CLI
# Create other telnet instances for other cores.
#
# NOTE: running with the SCP UART window in the SVP gui will not work, as there will be data corruption due to multiple telnet connections
# can either run SVP in headless mode:  launch SVP via 'runsvp -SimConfig chie_bins_single_die_dat' or dual die
# or launch SVP with the GUI and close the SCP uart window. 'runsvpgui -SimConfig chie_bins_single_die_dat' or dual die
# The SCP UART window may re-opened for manual testing, but will need to be closed again for this script to work
# In the SVP Design Hierarchy navigate to /KingsgateSVP/DIE_0/RMSS/SCP/SCP_UART,  right click, and select 'Connect Terminal to Uart'
#
# to run this script from a repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/pythia/pylibs/mts_tests/dev_helper.py
#
#
import sys
import logging

logger = logging.getLogger(__name__)

from fpfw_automation_primitives.serial.telnet import (
    Telnet_,
)

def main():
    print("This is the main entry point of dev_helper.py")

    # for manual testing redirect loggers to stdout
    logging.basicConfig(
        level=logging.DEBUG,  # Set the logging level (DEBUG, INFO, etc.)
        handlers=[logging.StreamHandler(sys.stdout)]  # Redirect to stdout
        )

    # die 0, SCP
    # in pythia environment, same as self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
    die0_scp = Telnet_(host="localhost", port="4257")
    die0_scp.open()

    die0_scp.write(write_string="?\n")

    output = die0_scp.read_until(key="hm", timeout_seconds=2)

    logger.info(output)

    die0_scp.close()

if __name__ == "__main__":
    main()