# FPGA Debug User Guide

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This is the user guide to debug MSCP firmware on the BigFPGA machines.

### Terms

| Term | Description |
| - | - |
| FPGA | Field Programmable Grid Array - prototype RTL builds of the SoC |
| SDV | Software Development Vehicle |

### References

| Reference | Link |
| - | - |
| FPGA Debug One Note | [Link](https://microsoft.sharepoint.com/teams/EchoFalls/_layouts/OneNote.aspx?id=%2Fteams%2FEchoFalls%2FShared%20Documents%2FKingsgate%20SOC%2FFPGA%20Prototyping%2FFPGA%20Enablement%2FKingsgate%20FPGA%20Enablement) |
| FPGA Debug Wiki | [Link](https://woodinvillewiki.com/x/ZomcB) |
| SVP TEAM Support Mailing Group | [Link](FPGA_SUpport@microsot.com) |
| Booked Scheduler | [Link](https://cpsbooked.azurewebsites.net/) |

## Pre-requisites

### One Time Activations

1. If you do NOT have an SVC UNIX account, ask your manager to request one for you. [RB | Portal > SVC Unix New User Request Form](https://runbook.svceng.com/portal/newuser-request.cgi?retid=35b206afd3c97f8d667a983f9ab49a0d;action=cont;nu_manager=;). Select "XBOX" as the group)
1. If you have an SVC UNIX account, but don't recall the password, reset it using this [link](https://pwchange.svceng.com)
1. Make a request to be added to the "rduunixusg" Security Group (required to access \\lakshmi.svceng.com\rdu_lab). [Link](https://idweb/identitymanagement/default.aspx)
    - Click on "Security Groups (SGs)" in the task list at the left
    - Search for:  rduunixusg
    - Run/Check the box for "RDU Silicon Infrastructure Unix Users"
    - Click "Join"
    - Fill in justification and submit (e.g. "to use Raleigh FPGA prototype tools") 
1. Make a request to be added to the "FPGA_PC_Admin" Security Group (required to access FPGA Windows servers via Remote Desktop, AND gives admin privileges). [Link](https://idweb/identitymanagement/default.aspx)
    - Click on "Security Groups (SGs)" in the task list at the left
    - Search for:  FPGA_PC_Admin
    - Run/Check the box for "FPGA_PC_Admin"
    - Click "Join"
    - Fill in justification, submit (e.g. "to use Raleigh FPGA Windows machines")
1. Create an account in the ["Booked" scheduling tool](https://cpsbooked.azurewebsites.net/)
    - To create an account, use your Microsoft email address but not your Microsoft password. Choose a different password.
    - Email FPGA_Support@microsoft.com to activate your account (Booked will not automatically activate a new account or send an email)

### Environment Setup

**Note:** Steps marked with (O)  are recommended if you need source mapping of symbols while debugging. You are free to to skip them, or modify them if your use case is different.

1. (O)  Go to `\\lakshmi.svceng.com\rdu_lab\Users\` from your Dev PC (after you are inducted into the rduunixusg SG), and create a folder for yourself using your alias.
1. (O) In this folder, clone the Kingsgate.MSCP repo and check out main branch.

## Debugging/Running SCP code

1. (O) Make the changes you need in your local Kingsgate.MSCP repo, and build your code. Run `expbins -User <alias>` command on the Dev PC to copy over the elfs and sync your changes to git.
1. Book an FPGA system using the ["Booked" scheduling tool](https://cpsbooked.azurewebsites.net/). RDP to the FPGA system using the PC name mentioned on the tool.
1. If not already done, map the R: drive on the remote FPGA PC. Run the command `subst r: \\lakshmi.svceng.com\rdu_lab`.
1. (O) Synchronize source code on the Kingsgate.MSCP repo on R Drive by entering the repo and running the following commands on a terminal. Doing this on RDP is faster.

    ```pwsh
    git checkout <dev_branch>
    git fetch --all
    git reset --hard origin/<dev_branch> 
    ```

1. To start TRACE32 sessions for HSP, SCP, MCP, AP, run the `TRACE32_USB.bat` script found under `R:\Kingsgate\Kingsgate_TRACE32\Start` folder.
1. SCP the FPGA IFWI to the BMC (.build\Debug\arm-eabi-aarch\bin\flash\kingsgate.ifwi.fpga.debug.custom.dat)
1. Use bios-updater on BMC to flash the IFWI: `bios-updater -mode fwupdate -file /tmp/kingsgate.ifwi.fpga.debug.custom.dat`
1. Reset the SoC by running the following from the TRACE32 HSP session: `cd.do R:\Kingsgate\Kingsgate_TRACE32\Prep\HSPresetSystem.cmm`.
1. (O) Load the production scp firmware symbols by running this command on the SCP TRACE32 SCP session: `Data.LOAD.Elf R:\Users\<alias>\Kingsgate.MSCP\.build\Debug\arm-eabi-aarch\bin\scp_fw.elf /nocode /StripPART "Kingsgate.MSCP"`. In this, replace `<alias>` with your alias folder name created during **Environment Setup**. This command loads scp_fw.elf symbols and strips all prefixes before Kingsgate.MSCP in any referenced paths.
1. (O) Run the command `symbol.SourcePATH.SetBaseDir R:\Users\<alias>\Kingsgate.MSCP` on the SCP Trace32 session to remap reference paths to the network drive. Again, replace `<alias>` with your alias folder name created during **Environment Setup**.
1. (O) You can now debug code on the FPGA system, and source code is updated and mapped to the elf.
1. (O) In case you need to make changes to code and test, update the source code on your Dev PC, and perform steps 2, 3, 4 again

## FPGA FAQs
