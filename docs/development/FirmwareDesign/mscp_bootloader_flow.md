# MSCP Bootloader Flow

## Table of Contents

[[_TOC_]]

## Introduction 

### Description
This document is intended as a summary of the bootloader flow for MCP and SCP embedded CPUs in both the primary and secondary dies.
The flow involves the HSP, SCP, MCP and, MSCP_EXP as well as mailboxes for communication between the different processors at various stages of the boot.  

### Terms

| Term | Description                  |  
| ---- | ---------------------------- |
| HSP  | Hardware Security Processor  |
| SCP  | System Control Processor     |
| MCP  | Management Control Processor |
| MSCP_EXP | MSCP Expansion           |

### References
 
1. [Kingsgate Boot Programming Sequence](https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/Reset%20%26%20Boot/Hardware_Programming_Guide/KNG%20Boot%20Programming%20Sequence.xlsx?d=w13874e3bb5dd461595aac8bfdb03446e&csf=1&web=1&e=RZKGUQ)
2. [Kingsgate Boot Hardware Programming Guide](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/Reset%20%26%20Boot/Hardware_Programming_Guide/Kingsgate%20SoC%20Boot%20Reset%20Hardware%20Programming%20Guide%20WIP.docx?d=wd4ef9afafc374a5d9393997746f3ed25&csf=1&web=1&e=xfSXWY)
3. [Kingsgate firmware architecture](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/working/KG%20FW%20Architecture.docx?d=w88e40080cd1c465cbc34218785e31421&csf=1&web=1&e=yw5eoM)

## Memories used

1. MSCP_EXP RAM is used as a scratch space to store the SCP and MCP firmware binaries.
2. DRAM is used to store compressed MCP firmware for copying to secondary MSCP_EXP RAM.

## Assumptions/Exceptions

1. The fuse and DDR configuration sequences are not mentioned as they're not part of SCP bootloader flow.
2. HSP firmware is mentioned only if it directly interacts with SCP or MCP bootloaders. Sync points between HSPs on both dies is not mentioned.
3. The SCP firmware is a combination of the SCP bootloader and the compressed SCP production firmware.
4. The MCP firmware is a combination of the MCP bootloader and the compress MCP production firmware.
5. The HSP is expected to enable ECC for MSCP_EXP RAM and zero out the contents before downloading the SCP firmware.

## Communication interfaces

1. The primary HSP has a quad-SPI access to external flash where the firmware images are stored.
2. The HSSS has a quad-SPI-SPI bridge that allows communication between primary and secondary HSPs and also allows access to the HSSS scratch RAM.
3. The RMSS has two dual-SPI links, with primary die RMSS acting as master of one SPI link and secondary RMSS acting as master of the other SPI link.
4. The RMSS SPI bridge converts SPI transactions to AHB transactions allowing access to MSCP_EXP RAM space.
5. The secondary HSP uses the secondary RMSS master SPI-link to download SCP firmware from primary MSCP_EXP RAM into secondary MSCP_EXP RAM. 
6. The Die to Die (D2D) communication is used to copy MCP firmware from DRAM into secondary MSCP_EXP RAM.

## Primary SCP Bootloader Flow

1. Primary HSP downloads the SCP image from flash into primary MSCP_EXP RAM.   
2. The primary HSP sends a message via mailbox to secondary HSP informing SCP firmware is now available in primary MSCP_EXP RAM.
3. Primary HSP will program the reset vector of primary SCP to MSCP_EXP RAM address where the SCP firmware has been downloaded and release the primary SCP out of reset.  
4. The primary SCP starts executing the SCP bootloader from primary MSCP_EXP RAM address programmed as reset vector.
5. The primary SCP bootloader executes an decompression routine, which is used to decompress the SCP firmware in primary MSCP_EXP RAM into primary SCP ITCM and DTCM.
6. Post decompression, primary SCP will start executing from its ITCM.

## Secondary SCP Bootloader Flow

1. The secondary HSP receives a mailbox message from primary HSP triggering the copy of SCP firmware into secondary MSCP_EXP RAM from primary MSCP_EXP RAM.
2. The secondary HSP programs the secondary MSCP_EXP RAM address as the reset vector for secondary SCP and releases it out of reset.
3. The secondary SCP starts executing the SCP bootloader from secondary MSCP_EXP RAM address programmed as reset vector.
4. The secondary SCP bootloader executes an decompression routine, which is used to decompress the SCP firmware in secondary MSCP_EXP RAM into secondary SCP ITCM and DTCM.
5. Once done, the secondary SCP starts executing code from its ITCM.

## Primary MCP Bootloader Flow

1. The primary HSP downloads the MCP firmware from flash into DRAM to act as a staging area for secondary HSP.
2. The primary HSP then downloads the MCP firmware from DRAM into the primary MSCP_EXP RAM and performs image authentication at this point. 
3. The primary HSP programs the reset vector for the primary MCP to primary MSCP_EXP RAM address and brings it out of reset.
5. The primary MCP starts executing the MCP bootloader from primary MSCP_EXP RAM address programmed as reset vector.
6. The primary MCP bootloader executes an decompression routine, which is used to decompress the MCP firmware in primary MSCP_EXP RAM into primary MCP ITCM and DTCM.
7. After the image is decompressed, primary MCP starts executing from its ITCM.

## Secondary MCP Bootloader Flow

1. The secondary HSP downloads the MCP firmware from DRAM into secondary MSCP_EXP RAM and authenticates it.
2. The secondary HSP programs the reset vector for the secondary MCP and brings it out of reset.
3. The secondary MCP starts executing the MCP bootloader from secondary MSCP_EXP RAM address programmed as reset vector.
4. The secondary MCP bootloader executes an decompression routine, which is used to decompress the MCP firmware in secondary MSCP_EXP RAM into secondary MCP ITCM and DTCM.
5. After the image is decompressed, secondary MCP starts executing from its ITCM.


