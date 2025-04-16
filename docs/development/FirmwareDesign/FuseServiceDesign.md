# FUSE distribution

This document outlines the basic flow needed for fuse distribution to be performed in Kingsgate SCP firmware.

## Table of Contents

[[_TOC_]]

## Introduction
Fusing is a mechanism for storing non-volatile control of hardware operation which can be tailored to an individual device’s manufactured characteristics.
The specific non-volatile storage for fusing is referred to as a Fuse Macro (FM). The Fuse Macro is a collection of bits that support One Time Programming (OTP).
The FM is controlled by a dedicated fuse macro controller (FMC) to form a system fuse macro (SFM) .It provides a set of standard interfaces (e.g., APB & JTAG)
for accessing the FM.

The System Fuse Controller (SFC) is a collection of SFM’s and associated logic allowing multiple fixed size FMs to look and behave to the system as a larger fuse bank.
It provides further control and status to aid in reading and writing the fuse bank.

There are several different stages and scenarios in which a part could reside in, and the fuses manipulated.  These include below
 - Unfused , Manufacturing fusing , In lab fusing , In field fusing  and finally Soft fusing.

The Fuse Service in SCP runtime firmware performs the role of Soft Fusing if overrides are valid and available. Further to this it distributes the
fuse configuration to various subsystems in SoC

### Terms

| Term | Description                  |
| ---- | ---------------------------- |
| CDED |  Compression, Decompression, Encryption, and Decryption Sub System |
| DDRSS | DDR Sub System              |
| D2DSS | Die-2-Die Sub System        |
| HSP  | Hardware Security Platform  |
| IOSS | Input Output Sub System      |
| PCIeSS | Peripheral Chip Interconnect Express Sub System                  |
| SCP  | System Control Processor     |
| SDMSS | Smart Data Mover  Sub System                                      |
| MCP  | Management Control Processor |
| MSCP_EXP | MSCP Expansion           |
| VAB   | Virtualization and Aggregation Block Sub System                   |
| FM  | Fuse Macro non-volatile storage element for fuses. |
| FMC | Fuse Macro controller |
| SFM | System Fuse Macro.  A combination of a Fuse Macro and a Fuse Macro Controller. |
| SFC | System Fuse Controller.  A subsystem comprised of multiple SFMs |
| SFC RAM | SFC internal RAM known as Fuse RAM of size 3KB |
| RMSS | Runtime Management Subsystem |
| APB | Advanced Peripheral Bus |
| HNF | Fully coherent Home Node that manages the coherent part of the system address space |
| LDO | Low Dropout Regulator for volatage scaling and gating for AP cores |
| CSS | Compute sub system |
| ITCM | Instruction tighlty coupled RAM memory |
| DTCM | Data tightly coupled RAM memory |
| SoC  | System on Chiop |
| DFT  | Design for testability |
| ICC  | Inter core communication |

### Description

A Kingsgate chiplet is made up of 3 distinct fuse systems

- HSP Fuses: This fuse controller provides fuse access for the HSP logic and is not accessible by anything in the chiplet other than the HSP core.
- BISR Fuses: These fuse controllers only contain fuses for memory BIST repair.  They are under the control of DFT.
- System Fuses: This fuse controller is responsible for the fuse storage for the chiplet beyond HSP and memory BIST repair.

The role of Fuse Service in SCP runtime firmware is to read , override if any and distribute "System Fuses".

The current plan of record for Kingsgate is to have three 8Kb Fuse Macros per chiplet .The SPI flash memory that Primary HSP interacts will contain
overrides if any for specific fuse values and these can be used to overwrite the values read from the eFuse macros of System Fuse prior to fuse
distribution. The System Fuse block will live within MSCP_EXP which is part of RMSS and would ideally use an existing clock.
The SCP runtime firmware uses HSP mailbox primitives to download the fuse override information into the MSCP_EXP RAM and interacts with SFC to apply
overrides.

Fuse Service can proceed to distribute the fuses only after the mesh has been initialized. Pls refer the latest Fuse HAS and MAS document for details,
for the broad fuse descriptors.

But in general , below are main functions of fuse distribution
- Provides memory trim to the periphery of the mesh for bridges and functionality required to get access to the IO block register space.
- Distribute fuses to HNF, Poseidon, and Compute tile debug.
- Distribute fuses for ODCMs, LDOs, known good cores, known good HNFs.
- Distributes fuses to the CSS and IO blocks. Includes PCIeSS, USBSS, DDR , CDEDSS , VAB , SDMSS etc.

In Kingsgate these above functions listed are now tied to bootup sequence phase (major and minor), and fuse programming distribution from SCP is part of Phase -3
and Phase -4 of the bootup sequence. These are explained later in the document.

|Phase Name                               |Phase Major|Phase Minor|Description               |
|-----------------------------------------|-----------|-----------|------------------------- |
| HSP_DISTRIBUTED                         | 2         | 0         | HSSS Scratch RAM (optional), MSCP_EXP RAM , SCP/MCP IRAM/DRAM, SFC Fuse RAM |
| SCP Distributed (Post HSP Distribution) | 3         | 0         | bandgap for MSCP_EXP , Silicon ID |
| SCP Distributed (Mesh init)             | 3         | 1         | HNS Disable Fuses, HNS SRAM Fuses |
| SCP Distributed (Post Mesh Init)        | 4         | 0         | CSS Fuses (including SRAM and etc)  for ODCM |
| SCP Distributed (Post Bridge Init)      | 4         | 1         | VAB, CDEDSS, SDMSS, PCIESS, DDRSS, IOSS, D2DSS Fuses |

Also another important change in Kingsgate is addition of internal SFC Fuse RAM (3KB) and DMA engine for performing read/write to Fuse macro controller.

The Fuse Service as part of its flow should also have

1. Event trace logging with multiple log levels support based on event.
2. Should have CLI (command line interface) implemented  which works as "read <fuse_offset> and <width_in_bits>"
   and print "OK - <fuse_value>"
3. Should have a public interface for reading fuse for other runtime services to read based on its need. Eg: Power services needs to read many of power fuses.
   Reason to have public interface is to have a centralized logging , event tracing and also based on overrides done for any interaction with System Fuse macros in SCP firmware.
### Dependencies

1. System Fuse Controller interaction APIs released as part of silibs for sensing , applying overrides and distribute fuses.

2. Mailbox primitives APIs for HSP interaction to fetch fuse overrides database.

### References

1. [Kingsgate Boot Hardware Programming Guide](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/Reset%20%26%20Boot/Hardware_Programming_Guide/Kingsgate%20SoC%20Boot%20Reset%20Hardware%20Programming%20Guide%20WIP.docx?d=wd4ef9afafc374a5d9393997746f3ed25&csf=1&web=1&e=xfSXWY)
2. [Kingsgate firmware architecture](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/working/KG%20FW%20Architecture.docx?d=w88e40080cd1c465cbc34218785e31421&csf=1&web=1&e=yw5eoM)
3. [System Fuse HAS](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/Doc.aspx?sourcedoc=%7B4F547F29-186B-4E6D-95E4-BFBE1A607287%7D&file=Kingsgate%20SystemFuse%20HAS%201.1.docx&action=default&mobileredirect=true&isSPOFile=1&clickparams=eyJBcHBOYW1lIjoiVGVhbXMtRGVza3RvcCIsIkFwcFZlcnNpb24iOiI0OS8yNDA1MDMwNzYxNiIsIkhhc0ZlZGVyYXRlZFVzZXIiOmZhbHNlfQ%3D%3D)
4. [System Fuse MAS](https://microsoft.sharepoint.com/:w:/r/teams/Kingsgate/Shared%20Documents/MicroArchitecture%20Specs/MAS/Kingsgate%20System%20Fuse%20MAS%20Working.docx?d=wa1adcc50719342a6b92418b0f5f64e71&csf=1&web=1&e=df3WZb)
5. [Kingsgate Fuse details](https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/DFX%20(Fuse,%20Test%20%26%20Debug)/Fuse/Kingsgate%20System%20Fuse.xlsx?d=w21e55dccf8014e4ca077e7d836176bdf&csf=1&web=1&e=mOrhgq)
6. [Silibs Fuse Documentation](https://dev.azure.com/ms-tsd/Kingsgate/_git/silibs?path=/libraries/fuse/doc/fuse.md)
7. [ICC transport Documentation](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/drivers/Interfaces/IccTransportInterface.md)

## Fuse Distribution Implementation

Note: Kingsgate will not support SoC level Warm Reset/Boot. Only M7 SCP/MCP cores support warm reset and not rest of RMSS / MSCP_EXP, hence the fuse distribution flow
      should apply only for cold boot.SCP as platform should have logic to know if its core boot is warm or cold ,and that is outside scope of this
      design .There should be a public interface in SCP firmware that could be leveraged by all services to know boot reason.

1. The Fuse service should have dependency on "dfwk" , "mesh" and "icc_hspmbx" components with these to be started before.
2. The HSP as part of boot sequence brings RMSS subsystem out of reset. This will include the MSCP_EXP subsystem containing the System Fuse Controller.
3. HSP must read the fuses required for the SCP , MCP and SFC memory trim (ITCRAM, DTCRAM, MSCP_EXP RAM and SFC Fuse RAM) directly from the Fuse Macros
   and program the appropriate registers with these values.
4. Once HSP brings SCP out of reset the runtime firmware starts execution. Here further to mesh initialization is completed fuse service could be started.
5. Invoke the Silibs API for reading internal fuse macros in SFC to the Fuse RAM through its internal DMA engine. The SFC Fuse RAM capacity is 3KB which fully accomodates
   3 Fuse macros each of 8 kilobits.
6. Invoke the HSP mailbox communication of SCP<->HSP and fetch the fuse overrides database from SPI flash to the MSCP_EXP RAM. The address space range and
   slot in MSCP_EXP RAM is completely left to implementation , as long as its ensured it doesn't overlap with others.
7. Read specific single fuse  from Silibs API for offset TESTFLOWCHECKS_SILICONMAJORREVISION_BIT_OFFSET and width TESTFLOWCHECKS_SILICONMAJORREVISION_WIDTH.
   This fuse value will determine if the silicon part is fused or not.
8.
```cpp
   If (part "not" fused && "no" overrides present)
   {
      then its error case in the flow
   }
   else if (part "not" fused && overrides present)
   {
      then invoke silibs api apply fuse "ignoring valid" passing override buffer stored in MSCP_EXP RAM
   }
   else if (part fused && "no" overrides present)
   {
      then fuse macros programmed doesn't need any override
   }
   else
   {
      invoke the silibs api for applying fuse overrides passing the override buffer stored in MSCP_EXP RAM
   }
```
9. Invoke distribute fuses silibs api for all the subsystems with phase major / minor labels that are to be distributed from SCP.
   Note:
   a) During fuse distribution there is an option to exclude specific range of fuse address of any single / multiple subsystems by providing start and end address range of fuse macros.


This is the public interface that other SCP runtime service would use for accessing the Fuse macros. This internal to its implementation prior to 
invoke of SFC macros should support event trace for fuse been read , check for parameter error case and that fuse overrides/distribution is completed

```cpp
/**
 * @brief  Read requested fuse value from System Fuse controller . For fuse offset and size information contained
 * in fuse_defines.h
 *         Stores the information to the provided address.
 *
 * @param fuse_store_addr   Desitnation to copy fuse information
 * @param fuse_bit_offset Offset into fuse array
 * @param fuse_bit_size   Width of fuses to be read (Note:Max size cannot exceed 8 bytes)
 * @return
      SUCESS  Successfully copied fuses
      ERROR   Create error codes based on error
 */
int platform_read_fuse(const uintptr_t fuse_store_addr, const uint64_t fuse_bit_offset, const uint32_t fuse_bit_size);

```

### Code Definitions

| Fuse Adddress | Details | Header to look|
| ----   | ----------- | ------------------|
| FUSES_TOP_E_FUSE_SPACE_ADDRESS | System eFUSE address base from SCP view | fuses_top_regs.h |
| FUSES_TOP_E_FUSE_SPACE_SIZE | System eFUSE space size | fuses_top_regs.h |
| SCP_TOP_SCP_EXP_ADDRESS | MSCP_EXP base address | scp_top_regs.h |
| SCP_EXP_TOP_RAM0_ADDRESS | MSCP_EXP SRAM0 offset from base address | scp_exp_top_regs.h |

System Fuse Address space : FUSES_TOP_E_FUSE_SPACE_ADDRESS + SCP_TOP_SCP_EXP_ADDRESS : 0x10B0000

Relative address in System Fuse address space
| Start Address | End Address | Details     |
|---------------|-------------|-------------|
| 0x0000        | 0x03FF      | Fuse Array 0 - Direct access to FM0 |
| 0x0400        | 0x07FF      | Fuse Array 1 - Direct access to FM1 |
| 0x0800        | 0x0BFF      | Fuse Array 2 - Direct access to FM2 |
| 0x4000        | 0x7FFF      | Fuse RAM |

The Fuse header definitions with mapping for fuse identifiers , their offset and width in bits for value.
[Fuse Definitions](https://dev.azure.com/ms-tsd/Kingsgate/_git/silibs?path=/libraries/fuse/include/fuse_defines.h)

The below provides list of APIs that are proposed to be APIs for the fuse library as well as any specific enums etc.These APIs are bound to change or remain same , hence always
refer Silibs fuse headers for latest information.Purpose of listing them is to map code flow to APIs , along with parameters.


```cpp

/*!
 * @brief Copy all the eFuse data to fuse RAM
 *
 * @retval ::SUCCESS The operation succeeded.
 * @retval ::FAILURE  the operation did not succeed
 * @return One of the standard error codes.
 */
silibs_status_t fuse_dma_copy_to_ram_blocking(void);

/**
 *  @brief This function is used to read fuse values from the eFUSE macros into desired address
 *
 *
 *  @param[in] fuse_bit_offset  Fuse bit offset in fuse array in SFC
 *  @param[in] fuse_bit_size    Total no. of bits in the fuse (fuse size)
 *
 *  @return
 *      fuse value
 */
uint64_t read_fuse(const unsigned fuse_bit_offset, const unsigned fuse_bit_size);

/**
 *  @brief This function is used to read fuse values from the eFUSE macros into desired address
 *
 *
 *  @param[in] override_buffer  Base address where the override fuse info is located
 *
 *  @return
 *      SILIBS_SUCCESS
 */

The override buffer starts with a header information as below:

typedef struct
{
    uint32_t fuse_override_descriptors_offset; // offset in override_buffer that provides list of fuse override descriptors as
	                                           // mentioned below mentioning if its valid & override
    uint32_t fuse_override_array_offset; // offset in override_buffer that provides value of fuse in bits
    uint32_t fuse_override_version;
} fuse_override_header;

typedef union
{
    struct
    {
        uint8_t valid : 1;
        uint8_t is_override : 1;
        uint8_t reserved : 6;
    };
    uint8_t data;
} fuse_override_descriptor;

The format of this buffer also represents override data fetched from SPI flash through HSP.
 
int fuse_override(const uintptr_t override_buffer);

/*!
 * @brief Apply Fuse overrides, ignoring valids. This API should be used on unfused parts to apply
          all overrides (replacement or offset still respected).
 *
 * @param[in] override_buffer memory buffer address of override data
 *
 * @retval ::SUCCESS The operation succeeded.
 * @retval ::FAILURE  the operation did not succeed
 * @return One of the standard error codes.
 */
silibs_status_t fuse_override_ignoring_valids(const uintptr_t override_buffer);

/* 
 *@brief Distribute fuses to CSRs matching the given phase
 *
 * @param[in] phase Reset phase to distribute. Up to 4 phases are currently supported.
 * @warning The addresses must be SCP-based addresses (or whatever core is distributing the fuses).  E.g. fuses
 * distributed into the system address space must account for the remapping window.
 * @retval ::SUCCESS The operation succeeded.
 * @retval ::FAILURE  the operation did not succeed
 * @return One of the standard error codes.
 */
 
typedef struct _fuse_dist_exclude_range_t_
{
    uint64_t start_addr;
    uint64_t end_addr;
} fuse_dist_exclude_range_t;

typedef enum
{
    HSP_DISTRIBUTED_MAJOR = 2, // These fuses are distributed by the HSP prior to SCP boot
    POST_SCP_INIT_MAJOR = 3,   // Major phase: 3
    POST_NITOWER_MAJOR = 4,    // Major phase: 4
    DO_NOT_DISTRIBUTE = 99     // Fuses marked with this phase are not distributed (consumed only by FW)
} FUSE_DISTRIBUTION_MAJOR_PHASE;

typedef enum
{
    MSCP_EXP_INIT_MINOR = 0,      // Major phase: 3, minor phase: 0
    POST_MSCP_EXP_INIT_MINOR = 1, // Major phase: 3, minor phase: 1

    CSS_MINOR = 0,      // Major phase: 4, minor phase: 0
    POST_CSS_MINOR = 1, // Major phase: 4, minor phase: 1

} FUSE_DISTRIBUTION_MINOR_PHASE;

silibs_status_t fuse_distribution(const FUSE_DISTRIBUTION_MAJOR_PHASE phase_maj,
                                 const FUSE_DISTRIBUTION_MINOR_PHASE phase_min,
                                 const fuse_dist_exclude_range_t *exclude_list, const uint32_t exclude_list_count);
```


Below is list of mailbox APIs that would be needed for communication with HSP. Since its runtime access ,all access
to HSP mailbox should be done through ICC mailbox driver.Listing them for mapping with code flow along with parameters to be used.

ICC transport interface definitions can be found below:
[ICC transport public headers](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/src/drivers/Interfaces/IccTransport/fpfw_icc_transport_interface.h)

ICC mbox interface component to HSP mailbox is already created in SCP firmware below. We need to use those instance references for communication.
[ICC SCP<->HSP Mbox Creation](https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.MSCP?path=/src/libs/runtime_init/icc_transport_init/mscp_hspmbx_init.c)

From the transport interface init we need "&mscp_mbx_intf.header" for further API calls to ICC transport driver. 
```cpp
   typedef struct {
       DFWK_INTERFACE_HEADER header;
       fpfw_mbox_icc_transport_device_t* mbox_dev;
   } fpfw_mbox_icc_transport_intrf_t;

   ===========================================
   static fpfw_mbox_icc_transport_intrf_t mscp_mbx_intf = {};
   fpfw_status_t status = fpfw_mbox_icc_transport_dfwk_interface_init(fpfw_init_get_handle(hspmbox_id), &mscp_mbx_intf);
   return (fpfw_init_result_t){status, &mscp_mbx_intf};
```
The handle for transport mailbox interface can be acquired by using the below api return value

```cpp
   fpfw_init_get_handle("icc_mbx");
```

Below are ICC transport interfaces that needs to be used for HSP mailbox message communication. Preferring sync interfaces based on usecase here.

```cpp
/**
 * @brief Sync request API to get max message size supported by the ICC transport interface
 *
 * @param transport_interface An interface to a driver that implements the Icc Transport Interface.
 * @param output_size Max message size
 * @return int32_t Error code
 */
fpfw_status_t fpfw_icc_transport_get_max_mesg_size_sync_req(PDFWK_INTERFACE_HEADER transport_interface, size_t* output_size);

HSP mailbox message tentative format should be as below , but one needs to get exact definition , enums from HSP headers library when available.

HSP_MAILBOX_MSG message;

message.Cmd                  = HspMailboxCmdLoadFirmware;
message.LoadFirmware.Id      = HspFirmwareIdFuseOverride;
message.LoadFirmware.Address = <address of MSCP_EXP RAM to store override>
message.LoadFirmware.Size    = <size of override buffer in bytes>;

/**
 * @brief Sync request API to try sending data over the ICC transport interface
 *
 * @param transport_interface An interface to a driver that implements the Icc Transport Interface.
 * @param send_buffer A buffer to hold the data to send.
 * @param buffer_size The size of the input buffer.
 * @return int32_t Error code
 */
fpfw_status_t fpfw_icc_transport_try_send_sync_req(PDFWK_INTERFACE_HEADER transport_interface, void *send_buffer, size_t buffer_size);


HSP_MAILBOX_MSG message;
message.ResponseStatus.status

/**
 * @brief Sync request API to try receiving data over the ICC transport interface
 *
 * @param transport_interface An interface to a driver that implements the Icc Transport Interface.
 * @param recv_buffer A buffer to hold the data received.
 * @param buffer_size The size of the input buffer.
 * @param output_recv_bytes (Output) the number of bytes written.
 * @return int32_t Error code
 */
fpfw_status_t fpfw_icc_transport_try_recv_sync_req(PDFWK_INTERFACE_HEADER transport_interface, void *recv_buffer, size_t buffer_size, size_t *output_recv_bytes);

```