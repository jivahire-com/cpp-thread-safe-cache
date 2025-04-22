# MSCP Bootloader Implementation

## Table of Contents

[[_TOC_]]

## Introduction

 The boot loader is a common library that is to be utilized by the cortex-m7 CPUs of both the MCP and SCP sub systems. The final boot loader image is a combination of the boot loader code along with the compressed runtime firmware embedded into the image. The boot loader executes from the RMSS MSCP_EXP RAM, clear the ITCM/DTCM, decompress the run time firmware into the ITCM/DTCM and finally jump into the entry point for the run time firmware.

### Acronyms

| Term | Description                  |
| ---- | ---------------------------- |
| DTCM | Data Tightly Couple Memory (RAM to hold data) |
| ITCM | Instruction Tightly Coupled Memory (RAM to hold instructions) |
| HSP  | Hardware Security Processor  |
| HPG  | Hardware Programming Guide   |
| MCP  | Management Control Processor |
| MSCP_EXP | MSCP Expansion           |
| RMSS | Runtime Management Sub System |
| SCP  | System Control Processor     |

### Scope

1. This document only covers the SCP/MCP execution from the moment the cortex-M7 CPUs are brought out of reset to the point of jumping to the entry point for executing the run time firmware.
2. The dual die SCP/MCP boot-flow coordination and the internals of HSP-SCP and HSP-MCP mailbox protocol are considered out of scope for this document.

### Description

The Kingsgate boot flow is composed of 7 different phases from 0 to 7 as explained in the Kingsgate Boot HPG.
The SCP cortex-M7 is brought out of reset as part of Phase-2 of boot by the HSP and the MCP cortex-M7 is brought out of reset by the HSP as part of phase-5 of the boot flow.
The compression and decompression of the run time firmware is done by using the ZLIB library hosted on github (open source).

### Memory Layout
Considering SCP and MCP reset is in a sequence from HSP , we would be using only MSCP_EXP RAM 0 for their respective bootloader executable placeholder.
NOTE: The cortex-M7 reset vector is mapped to 0x0120_0080 in MSCP_EXP RAM 0 , where the SCP/MCP bootloader reset handler execution entry point is present.

```txt

Boot loader MSCP_EXP RAM memory map:
0x0120_0000 +--------------------------------+
            |                                |
            |         BOOT_METADATA          |
            |                                |
0x0120_0020 +--------------------------------+
0x0120_0021 +--------------------------------+
            |           RESERVED             |
0x0120_007F +--------------------------------+
0x0120_0080 +--------------------------------+
            |        BOOT LOADER             |
            |        TEXT SECTION            |
            |                                |
            +--------------------------------+<----+
            |  Main Image Embed Header       |     |
            +--------------------------------+     |
            |                                |     |
            | Main Image Compressed ITCM BIN |     +
            |                                |     |
            +--------------------------------+     +<--- MAIN_IMAGE MAX
            | Main Image Compressed DTCM BIN |     |     SIZE: 800 KB
            |                                |     |
            +--------------------------------|<----+
            |    BOOT LOADER DATA SECTION    |
            |                                |
            +--------------------------------+ 0x012F_FFFF (Cannot exceed this address for bootloader elf i.e 1MB size)
```

### References

1. [Kingsgate Boot Reset HAS 1.0](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/HAS%201.0/Boot%20and%20Reset/KNG%20SoC%20Boot%20Reset%20HAS%20v1p0.docx?d=wcc73899fe86b4b93bb1527345dd6c1a1&csf=1&web=1&e=D8oqTt)
2. [Kingsgate Boot Reset HAS 1.1](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/HAS%201.0/Boot%20and%20Reset/KNG%20SoC%20Boot%20Reset%20HAS%20v1p1%20(%20WIP).docx?d=w3ad45eb436a74b638d3d8e1ad278d14c&csf=1&web=1&e=2z46Rx)
3. [Kingsgate RMSS HAS](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/HAS%201.0/RMSS/KingsGateRMSS%20HAS%20v0p1.4.docx?d=w16370ee610d64064b14bb49a93125509&csf=1&web=1&e=9gPbVD)
4. [Boot Flow Diagram](https://microsoft.sharepoint.com/:u:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/HAS%201.0/Boot%20and%20Reset/KNG%20SoC%20Boot%20Reset%20HAS%20Block%20Diagrams.vsdx?d=w0f98c30620ff434e920dbeb974043e3e&csf=1&web=1&e=QyuJyx)
5. [Kingsgate Boot Programming Sequence](https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/Reset%20%26%20Boot/Hardware_Programming_Guide/KNG%20Boot%20Programming%20Sequence.xlsx?d=w13874e3bb5dd461595aac8bfdb03446e&csf=1&web=1&e=RZKGUQ)
6. [Kingsgate Boot Hardware Programming Guide](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/Reset%20%26%20Boot/Hardware_Programming_Guide/Kingsgate%20SoC%20Boot%20Reset%20Hardware%20Programming%20Guide%20WIP.docx?d=wd4ef9afafc374a5d9393997746f3ed25&csf=1&web=1&e=xfSXWY)
7. [Kingsgate firmware architecture](https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/working/KG%20FW%20Architecture.docx?d=w88e40080cd1c465cbc34218785e31421&csf=1&web=1&e=yw5eoM)
8. [Zlib github URL](https://github.com/madler/zlib)
9. [Kingsgate Address Map r1p08](https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/SOC%20Top/Kingsgate%20SOC%20Address%20Map%20r1p08.xlsx?d=wf529ba31f2ff40938a958aa7380e2093&csf=1&web=1&e=rgzxvl)

## Implementation

### Generating combined ELF

1. The main boot loader elf is compiled with a 800 KB empty main-image section which is used to hold the compressed ITCM, DTCM binaries of runtime firmware. This custom section is carved out as custom section in text section of bootloader elf, by adding to memory layout of linker script.
2. We parse the run time SCP/MCP elf and split it into two binaries comprising of all text sections for the ITCM binary and the various data sections for the DTCM binary, both these binaries are finally compressed using gzip.
3. The information about the location offsets, compressed and uncompressed sizes of the compressed ITCM, DTCM binaries are generated and the header, compressed ITCM/DTCM binaries are then inserted into main-image section of the boot loader elf to generate the final elf which is the boot loader + main image header + compressed ITCM/DTCM of the run time firmware.
 Here as part of generating runtime combined embed bootloader elf , various checks need to be performed for corner cases of possible overflows for both compressed & uncompressed ITCM and DTCM sections and on failure,shouldn't generate combined elf.


1.Definining the layout for main image section:
```c
 Add a custom section in linker script for memory layout as below:
    /* Main Image */
    .mainimage (READONLY) : {
        . = ALIGN(4);
        _mainimage_s = .;
        KEEP(*(.mainimage))
        _mainimage_e = .;
    } > CODE
```

2. Defining the size of main image section to be done in C code as below:
```c
   extern char _mainimage_s;//represents the start address of mainimage
   extern char _mainimage_e;//represents the end address of mainimage

   uint8_t mainImage[MAIN_IMAGE_SIZE] __attribute__((section(".mainimage"))) = {0};
```
3.Main image embed header:
```c
typedef struct embed_image_section_s {
    uint32_t compressed_offset;   // Offset from beginning of the embed_image_header to the first byte of the compressed image
    uint32_t compressed_size;     // Compressed size of the image
    uint32_t uncompressed_size;   // Uncompressed size of the image
    uint32_t uncompressed_crc32;  // CRC32 checksum of the uncompressed image
} embed_image_section_t;

typedef struct embed_image_header_s {
    uint32_t embed_image_header_tag;  // 0xEBDD4EAD magic data for authenticity
    embed_image_section_t itc_ram;  // Location and size of the iram image
    embed_image_section_t dtc_ram;  // Location and size of the dram image
} embed_image_header_t;
```

### Firmware download

1. The HSP maintains boot meta data as below in MSCP_EXP RAM0 at address location 0x0120_0000 , that helps in knowing the boot reason for SCP / MCP.
```c
typedef struct kingsgate_boot_metadata_s {
    //
    // Metadata version
    //
    int8_t meta_data_version;
    //
    // ResetReason (warm or cold boot)
    //
    int8_t reset_reason; //BITMASK_WARM_BOOT          (0b10000000)
    //
    //
    int8_t boot_mode;
    //
    // Reserved
    //
    int8_t reserved;
} kingsgate_boot_metadata_t;
```
2. SCP FW Download: The embedded SCP boot loader firmware is downloaded by the HSP FW from SPI flash to the MSCP_EXP RAM0 before releasing SCP cortex-M7 reset.
3. MCP FW Download: On request from SCP runtime firmware the embedded MCP boot loader firmware is copied by HSP from SPI flash to DRAM , and later HSP SPRT in second staging copies it to MSCP_EXP RAM0 . Once this download is complete, the HSP release the MCP cortex-M7 out of reset.

### Boot loader execution

1. The cortex-M7 SCP/MCP CPU on reset executes the reset handler pointed to by reset vector in MSCP_EXP RAM 0 address location 0x0120_0080.\
    a. The reset handler clears the entire ITCM to 0.\
    b. Read the boot meta data from MSCP_EXP RAM0 shared address from HSP.
```c
       if (boot reason is cold boot)
       {
          clears the entire DTCM to 0
       }
```
    c. The reset handler then invokes the C runtime startup function.
2. The C runtime function is expected to perform the following functions.\
    a. Copy the boot loader data section from MSCP_EXP RAM0 to the lower 32KB of DTCM (location address mentioned in memory map).This is for bootloader's own requirement
       of its "data" section access.\
    b. Invoke the C main() function of bootloader executable.

3. The main function references boot config as mentioned below that provides all required information for the loading of runtime image.

```c
   typedef struct kingsgate_boot_config_s {
      size_t      data_src_base;
      size_t      data_src_end;

      size_t      itc_ram_base;
      uint32_t    itc_ram_size;
      size_t      dtc_ram_base;
      uint32_t    dtc_ram_size;

      size_t      boot_meta_base;

      mscp_cpu_t  cpu_type;
   } kingsgate_boot_config_t;

   kingsgate_boot_config_t boot_config = {
     .data_src_base = (size_t)&_mainimage_s,//Custom section where compressed runtime FW resides
     .data_src_end = (size_t)&_mainimage_e,

    .itc_ram_base = (size_t)MSCP_ITCM_RAM_BASE,//Base address of ITCM
    .itc_ram_size = MSCP_ITCM_RAM_SIZE,//Size of ITCM
    .dtc_ram_base = (size_t)MSCP_DTCM_RAM_BASE,//Base address of DTCM
    .dtc_ram_size = MSCP_DTCM_RAM_SIZE,//Size of DTCM

    .boot_meta_base = (size_t)MSCP_BOOT_RAM_BASE,//boot meta data updated by HSP

    .cpu_type = MSCP_CPU_TYPE,//This helps to know in chosing if context is SCP or MCP bootloader
  };
```
Invokes load_image API with the above boot config as an argument, the API is expected to do the following.\
a. Initialize the mailbox communication with HSP using the appropriate mailbox (SCP-HSP/MCP-HSP).\
b. Validate the boot config entries to ensure they're valid.\
c. Send boot status codes via mailbox communication to HSP of various stages of the flow. These codes help to know sync points and if any error encountered on failing\
   to start runtime firmware.\
d. Invokes the unpack image API. This api is responsible to locate main image section , fetch its header and locate the offsets for compressed ITCM and DTCM.\
   Further to that it invokes zlib decompress API to do the actual decompression of the main-image binaries into ITCM and DTCM accordingly.\
e. On succesful decompression , reset vector and stack of M7 is programmmed to ITCM base address , which will be entry point of runtime firmware.And it will be  branched
   to new execution point , and at this point bootloader execution flow is complete and runtime firmware execution starts.\
On any failure cases if HSP to MSCP mailbox communication is available , it sends boot status code for failure reason to HSP and further to that it enters WFI loop.


Here is the boot status codes currently used in kingsgate , however these are bound to change with ongoing discussions to effectively manage it.
```c
    // SCP range is from 0x40 - 0x5F, and MCP is 0x60 - 0x7F
    typedef enum _mscp_boot_status_code_t
    {
        //! progress codes for SCP
        MSCP_BOOT_STATUS_CODE_SCP_OK = 0x0,
        MSCP_BOOT_STATUS_CODE_SCP_START,
        MSCP_BOOT_STATUS_CODE_SCP_COLD_BOOT,
        MSCP_BOOT_STATUS_CODE_SCP_WARM_BOOT,
        MSCP_BOOT_STATUS_CODE_SCP_IRQ_DISABLED,
        MSCP_BOOT_STATUS_CODE_SCP_MESH_INIT_END,
        MSCP_BOOT_STATUS_CODE_SCP_TOWER_INIT_END,
        MSCP_BOOT_STATUS_CODE_SCP_ACCEL_INIT_END,
        MSCP_BOOT_STATUS_CODE_SCP_DDR_INIT_END,
        //! Error codes for SCP
        MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG,
        //! Keep last for SCP, do not use as post code
        MSCP_BOOT_STATUS_CODE_SCP_MAX,

        //! progress codes for MCP
        MSCP_BOOT_STATUS_CODE_MCP_OK,
        MSCP_BOOT_STATUS_CODE_MCP_START,
        MSCP_BOOT_STATUS_CODE_MCP_COLD_BOOT,
        MSCP_BOOT_STATUS_CODE_MCP_WARM_BOOT,
        MSCP_BOOT_STATUS_CODE_MCP_IRQ_DISABLED,
        //! Error codes for MCP, do not use as post code
        MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG,
        //! Keep last for MCP
        MSCP_BOOT_STATUS_CODE_MCP_MAX,

        //! Keep last entry in this enum, do not use as post code
        MSCP_BOOT_STATUS_CODE_MAX,
    } mscp_boot_status_code_t;

    // currently for all the config error below we send
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_BOOT_CONFIG

    // Tbd, possible mscp boot statuses for boot status code flow
    // to provide more details (to be added): 
    // This is sent after mailbox init if the SCP/MCP boot loader is executing
    MSCP_BOOT_STATUS_CODE_SCP/MCP_START
    // This code is sent if boot_config->data_src_base == 0
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_DATA_SRC_BASE
    // This code is sent if boot_config->data_src_end <= boot_config->data_sec_base
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_IMAGE_SIZE
    // This code is sent if boot_config->itc_ram_base == 0
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_ITCRAM_BASE
    // This code is sent if boot_config->itc_ram_size == 0
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_ITCRAM_SIZE
    // This code is sent if boot_config->dtc_ram_base == 0
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_DTCRAM_BASE
    // This code is sent if boot_config->dtc_ram_size == 0
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_DTCRAM_SIZE
    // This code is sent if boot_config->boot_meta_base == 0
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_BOOT_META
    // This code is sent to inform HSP the SCP IRQ is disabled 
    MSCP_BOOT_STATUS_CODE_SCP/MCP_IRQ_DISABLED
    // This code is sent if is boot metadata indicates it is a warm boot
    MSCP_BOOT_STATUS_CODE_SCP/MCP_WARM_BOOT 
    // This code is sent if is boot metadata indicates it is a cold boot
    MSCP_BOOT_STATUS_CODE_SCP/MCP_COLD_BOOT
    // This code is sent if call to unpack_image fails (decompression failure)
    MSCP_BOOT_STATUS_CODE_SCP/MCP_E_SIZE
```

## Bootloader interfaces 

The APIs used and their definitions are as below

```c
/**
 *  @brief This function validates the boot config, decompresses the firmware image
 *         and returns the boot entry address.
 *
 *  @note This is a boot loader function and is expected to run with interrupts disabled
 * 
 *  @param[in] boot_config
 *  Pointer of type kingsgate_boot_config_t, contains info needed to validate and unpack
 *  firmware image
 *
 *  @return
 *      returns pointer to boot address - is NULL if it is a failure
 */
void* load_image(kingsgate_boot_config_t* boot_config);

/**
 *  @brief This function is used unpack the instruction and data sections of the
 *         compressed firmware image. It takes source and destination as arguments
 *         and invokes zlib decompress APIs.
 *
 *  @note This function is not thread safe.
 *  @note This function is not ISR safe.
 *
 *  @param[in] source_buffer Base address containing the image header and compress image
 *  @param[in] source_buffer_size Total firmware image size
 *  @param[in] itcram_base Base address of ITCM
 *  @param[in] itcram_size Size of ITCM
 *  @param[in] dtcram_base Base address of DTCM
 *  @param[in] dtcram_size Size of DTCM
 *
 *  @return
 *      Returns true on success, false on failure
 */

bool unpack_image(size_t    source_buffer,
                  uint32_t  source_buffer_size,
                  size_t    itcram_base,
                  uint32_t  itcram_size,
                  size_t    dtcram_base,
                  uint32_t  dtcram_size);

/**
 *  @brief Function that decompresses given section into memory
 *
 *
 *  @param[in] source               Compressed source buffer
 *  @param[in] source_size          Compressed source size
 *  @param[in] destination          Destination buffer for uncompressed data
 *  @param[in] destination_size     Destination buffer max size
 *
 *  @return
 *      Returns true if decompress succeeded or false if it failed
 */
bool decompress(void* source, uint32_t source_size, void* destination, uint32_t destination_size);

/**
 * @brief This function takes the address to call and address of the INITVTOR register
 * of the cortex-M7 CPU as inputs, sets up the stack pointer and jumps to the address mentioned.
 *
 * @param[in] address   - Entry point address of the firmware to jump to
 * @param[i] vtor       - Address of the INITVTOR register in the cortex-M7 core register map
 *
 */
void launch_image(void* address, volatile uint32_t* vtor);
```

## Zlib APIs to be used

```c
Below are zlib library apis for decompression

z_stream stream = {0};
stream.next_in = source; // represents the compressed data location and size
stream.avail_in = source_size;

stream.next_out = destination;//represents the destination for decompression
stream.avail_out = destination_size;

ZLIB_WINDOW_SIZE = 16 + MAX_WBITS; // recommend this value for optimal usage of memory by zlib allocator

/**
 * @brief Sets up the context for decompression in zlib
 * @param[in] zlib stream context
 * @param[in] zlib window size
 * @return Z_OK on success
**/
int inflateInit2(z_stream *stream, int window_size); // Can replace with 32 if needed

/**
 * @brief performs decompression in zlib
 * @param[in] zlib stream context
 * @param[in] decompression stages
 * @return Z_STREAM_END on success
 */
int inflate(z_stream *stream, Z_FINISH); 
```


Here are the Mailbox APIs that are used for communication with HSP from SCP/MCP

```c
/**
 * @brief This function will initializes the mailbox context as per the user provided config
 * 
 * @param pConfig input, user provided config for the mailbox of type FPFW_MBX_REG_CONFIG
 * @param pMbxCtx output, ctx for the specific instance of mbx
 * @return FPFW_MBX_SUCCESS if success, else error
 */
int32_t FpFwMailboxInit(PFPFW_MBX_REG_CONFIG pConfig, PFPFW_MBX_PRIMITIVE_CTX pMbxCtx);

/**
 * @brief This function will flush receiving FIFO and clear the err_bit
 * 
 * @param pMbxCtx mbx ctx populated in FpFwMailboxInit
 * @return int32_t, 0 if success or specific return code of type FPFW_MBX_STATUS
 */
int32_t FpFwMailboxFlushFIFO(PFPFW_MBX_PRIMITIVE_CTX pMbxCtx);

/**
 * @brief This function will send a complete mailbox message
 * 
 * @param pMbxCtx  mbx ctx populated in FpFwMailboxInit
 * @param pMessage buffer provided by client containing data to send, 
 *                  buffer size as per the underlying fifo depth
 * @return int32_t, 0 if success or specific return code of type FPFW_MBX_STATUS
 */
int32_t FpFwMailboxSend(PFPFW_MBX_PRIMITIVE_CTX pMbxCtx, PFPFW_MBX_PAYLOAD pMessage);

/**
 * @brief This function will receive a complete mailbox message
 * 
 * @param pMbxCtx mbx ctx populated in FpFwMailboxInit
 * @param pMessage buffer provided by client to store received data, 
 *                  buffer size as per the underlying fifo depth
 * @return int32_t, 0 if success or specific return code of type FPFW_MBX_STATUS
 */
int32_t FpFwMailboxReceive(PFPFW_MBX_PRIMITIVE_CTX pMbxCtx, PFPFW_MBX_PAYLOAD pMessage);
```

NOTE: Please refer Kingsgate SOC sharepoint for latest address map, the below mappings are valid as of r1p08 revision of the address map (check references).

## SCP Address Map

| Start Address | End Address | Size (MB) | Target     | Description            | Access Control Programmability|
| ------------- | ----------- | --------- | ---------- | ------------------     | -------------- |
| 0x0000_0000   | 0x0007_FFFF | 512kiB    | ITCM       | SCP Instruction RAM    | Statically Assigned Non-secure/SCP-Private | 
| 0x0008_0000   | 0x00FF_FFFF | 15.5MiB   |	ITCM       | Reserved (SCP Instruction RAM) | -- |
| 0x0100_0000   | 0x017F_FFFF | 8MiB      | MSCP_EXP   | See MSCP_EXP Address Map |	See MSCP_EXP Address Map |
| 0x0180_0000   | 0x0FFF_FFFF | 232MiB    |	MSCP_EXP   | Reserved |	-- |
| 0x1000_0000	| 0x1FFF_FFFF | 256MiB    | SCP-PvtInt | Reserved | -- |
| 0x2000_0000   | 0x2007_FFFF | 512kiB    |	DTCM       | SCP Data RAM | Statically Assigned Non-secure/SCP-Private |
| 0x4600_0000   | 0x4600_FFFF |	64kiB     |	SCP-PvtInt | SCP to HSP Mailbox | Statically Assigned Non-secure/SCP-Private |

## MCP Address MAP

| Start Address | End Address | Size (MB) | Target     | Description            | Access Control Programmability|
| ------------- | ----------- | --------- | ---------- | ------------------     | -------------- |
| 0x0000_0000   | 0x0007_FFFF | 512kiB    | ITCM       | MCP Instruction RAM    | Statically Assigned Non-secure/MCP-Private | 
| 0x0008_0000   | 0x00FF_FFFF | 15.5MiB   |	ITCM       | Reserved (MCP Instruction RAM) | -- |
| 0x0100_0000   | 0x017F_FFFF | 8MiB      | MSCP_EXP   | See MSCP_EXP Address Map |	See MSCP_EXP Address Map |
| 0x0180_0000   | 0x0FFF_FFFF | 232MiB    |	MSCP_EXP   | Reserved |	-- |
| 0x1000_0000	| 0x1FFF_FFFF | 256MiB    | MCP-PvtInt | Reserved | -- |
| 0x2000_0000   | 0x2007_FFFF | 512kiB    |	DTCM       | MCP Data RAM | Statically Assigned Non-secure/MCP-Private |
| 0x4564_0000   | 0x4564_FFFF |	64kiB     |	MCP-PvtInt | MCP to HSP Mailbox | Statically Assigned Non-secure/MCP-Private |

## MSCP_EXP Address Map

| Start Address | End Address | Size (MB) | Target     | Description            | Access Control Programmability|
| ------------- | ----------- | --------- | ---------- | ------------------     | -------------- |
| 0x0120_0000 | 0x012F_FFFF | 1024kiB |	MSCP_EXP/RAM0 |	SRAM0 |	Statically Assigned Non-secure/(HSSS,SCP,MCP)-Private |
| 0x0130_0000 |	0x013F_FFFF | 1024kiB |	MSCP_EXP/RAM1 |	SRAM1 |	Statically Assigned Non-secure/(HSSS,SCP,MCP)-Private |
