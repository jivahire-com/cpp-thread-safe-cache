# Accelerator Bringup (Init, Firmware Download, and Reset)

## Table of Contents

[[_TOC_]]

## Introduction

### Description

This document describes the init sequence for the accelerators on Kingsgate. This includes the init sequence, firmware download, as well as reset handling for the accelerators.

### Terms

| Term          | Description                                                               |
| ------        | -------------------------------                                           |
| SDM           | Smart Data Mover                                                          |
| CDED          | Compression, Decompression, Encryption, Decryption Engine                 |
| CDED-SDM      | SDM front end for the CDED                                                |
| emCPU         | Embedded CPU, a Cortex m7 core attached to either the SDM or the CDED IP  |
| TCM           | Tightly Coupled Memory                                                    |
| Accelerator   | Either SDM or CDED                                                        |
| SDMSS         | SDM Sub System                                                            |
| CDEDSS        | CDED Sub System                                                           |

### Reference Documents

| Document                                  | Link                                                                                                                                                                                                      |
| ----------------------------------------- | -----------------------------------                                                                                                                                                                       |
| Kingsgate SoC Boot Reset Programming Guide| [Link](https://microsoft.sharepoint.com/:f:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/Reset%20%26%20Boot/Hardware_Programming_Guide?csf=1&web=1&e=Hvi2Ka)                           |
| Firmware Architecture Document            | [Link](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/working/KG%20FW%20Architecture.docx?d=w88e40080cd1c465cbc34218785e31421&csf=1&web=1&e=k23znC)   |
| SDM Architecture Specification            | [Link](<https://microsoft.sharepoint.com/teams/BlueridgeNon-implementing/Shared Documents/General/Architecture/SDM/SDM HAS WIP.docx?web=1>)                                                               |
| SDM Micro-Architecture Specification      | [Link](<https://microsoft.sharepoint.com/:w:/t/EchoFalls/EUecIS3gZcBFkCpug0veDgABZ4D8j1o27YixqpzqqApogA?e=w2MAtz>)                                                                                        |
| SDM Configuration Registers               | [Link](<https://microsoft.sharepoint.com/:u:/t/ses-ipteamsdm/EWdni6ZYb8BLlvfjNJJgUkEBt4LMZ1jAhF1Uyim2IwfVng?e=RkeF1B>)                                                                                    |
| SDM CDED Firmware Architecture Specification   | [Link](<https://microsoft.sharepoint.com/:w:/r/teams/Kingsgate/Shared%20Documents/Firmware/Production%20Firmware/IDC/SDM%20%26%20CDED/Firmware%20Architecture%20Spec/SDM_CDED_FAS_WIP.docx?d=wa454e38edb314c53b87c3419e84dc61c&csf=1&web=1&e=ieCod2>) |

## Requirements

- Prior to bringing up the accelerators, the SCP shall initialize all the IPs required for the accelerators to boot up. The only exception to this is the initialization of the CDEDSS Tower, which shall be initialized by the HSP. This includes, but is not limited to:
  - Enable and configure System NI Tower
  - Enable and configure Mesh.
  - Enable and configuring VABSDMSS and VABCDEDSS and everything inside.
  - Configure isolation control on SDMSS and CDEDSS.
  - Enable and configure SDMSS, CDEDSS.
  - Initializing the config registers inside the accelerator IPs.

  For a full description of this configuration flow, please refer section 11.2 of the SDM CDED FAS (Firmware Architecture Specification) Document.

- Once all pre-initialization is done, the accelerator bringup sequence shall initialize the accelerator firmware by requesting HSP to download firmware to the respective TCMs.
- In runtime, the same module shall also be responsible for reset management of the accelerators.
- The accelerators init module shall be capable of handling the boot and reset sequences on both Dies in Kingsgate.

## Dependencies

- fpfw ICC libraries (fwlib.driver.mboxIccTransport, fwlib.service.icc_dispatcher, fwlib.service.iccBase)
- HSP firmware headers (hsp_firmware_headers)
- silibs tower, vab and pcr libraries for sdm and cded sub systems (pcr_vab, tower_sdmss, tower_vab, vab_pcr_init)

## Design

### Init Sequence

The following pre-requisites for Accelerator initialization are performed in other modules of the SoC FW and are not required to be handled in this module for Kingsgate:

- Enable and configure System NI Tower.
- Enable and configure Mesh.

Isolation control configuration needs to be performed right after VAB towers are configured. For this purpose, we piggyback on the silibs `accelip_ss_enable_ip_isolation` API, calling it with the configuration context necessary.

- Isolation control configuration is called using the fpfw_init module to sequence the init stages. **Dependencies:** It needs `std_io` to be able to print out logs, `hw_ver` for die and SoC configuration awareness, and `tower_cfg` to ensure access to the sub-system tower.
- Before calling `accelip_ss_enable_ip_isolation`, we map the accelerator sub system to the SCP ATU MAP temporarily, and later release the ATU map once configuration is complete. **Open:** Should this move to a static ATU map?
- This API is works across dies and accelerator types, which are passed on as a part of the context, and performs the isolation configuration. We iterate this API multiple times, for each die and each accelerator as necessary.
- If isolation configuration fails, we assert to stop init sequence since this is fatal for the SoC bringup.

Configuring VABSDMSS and VABCDEDSS is already performed outside this module, in the VAB configuration sequence. Hence this is not necessary as a part of accelerators init.

The accelerator IP init module piggy backs on the silibs sequence library API `accelip_ss_init` calling it with the required configuration context.

- Before calling `accelip_ss_init`, we map the accelerator sub system to the SCP ATU MAP. The same ATU mapping is also required for SDM interrupts initialization and hence is retained. **Open:** Should this move to a static ATU map?
- This API is works across dies and accelerator types, which are passed on as a part of the context, and performs the isolation configuration. We iterate this API multiple times, for each die and each accelerator as necessary.
- `accelip_ss_init` silibs API performs the following:
  - Init and configure PCR inside the Accelip and de-assert its reset pin.
  - Perform pre-pcie config.
  - Configure the emCPU inside the accelerator, disable fence, and de-assert the emCPU reset.
  - Initialize memories and configure ECC on the TCMs inside the IP.
  - Configure ECAM, RCIEP and RCEC for the IP (in that order), and finally, enable ECAM.
  **Note:** Firmware is not loaded within `accelip_ss_init` and hence, cpu_wait is also not released. This is performed separately.
- Once accelerator is initialized, SCP requests the HSP to download the firmware using a synchronous mailbox request. This is a 2 step request, first to load the ITCM and second to load the DTCM. SCP waits till firmware is completely loaded and HSP acknowledges FW download complete for both TCMs.
- Finally the emCPU cpu_wait bit is released, so that the IP can start functioning.

### Reset Sequence

The reset sequence uses various elements of the init sequence. The reset sequence is exposed as an API for the business logic to use as needed.

For the reset sequence, there is a request for silibs to expose some internal functions as public APIs. Till such time as that is completed, these functions are copied over to the SCP Production FW repo.

The following is broadly the reset sequence:

- Assert cpu_wait to halt emCPU operation.
- Enable Fence.
- Assert reset.
- Toggle ITCM Enable to reset the memory.
- Disable Fence.
- De-assert reset.
- Request HSP to download firmware to the accelerator. This is a 2 step request, first to load the ITCM and second to load the DTCM. Wait until HSP acknowledges that firmware download is complete for both TCMs.
- De-assert cpu_wait to resume emCPU operation.

For a full description of this flow, please refer the SDM CDED FAS (Firmware Architecture Specification) Document.

## API

| API           | Description                                           |
| ----------------------   | ----------------------------------------------------- |
| scp_accelerators_init(void)    | Initialize all the accelerators available on the Die|
| scp_accelerators_isolation_control() | Enable isolation control for all the VABs channeling traffic to the accelerators on the die|
| scp_accelerators_emcpu_reset(eACCELERATOR_TYPE accel_type, crash_dump_cb_t cb_fun, void *cb_ctx)    | API to reset a specific accelerator. Has the capability to register a callback with context that will be executed when reset is complete|
| scp_accel_update_default_knobs(subsystem_ctxt_t* p_ss_ctxt)     | Override default knob configurations for the accelerator|

## Variants

None
