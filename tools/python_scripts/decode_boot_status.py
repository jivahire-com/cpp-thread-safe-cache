# Copyright (c) Microsoft Corporation. All rights reserved.

"""
Decode boot status codes from Rack Manager BIOS code output.

Usage:
    python decode_boot_status.py <input_string>
    python decode_boot_status.py -f <file>
    echo "<bios code>" | python decode_boot_status.py
"""

import sys
import argparse
import re

# ---------------------------------------------------------------------------
# Enums transcribed from boot_status.h
# ---------------------------------------------------------------------------

COMPONENT_GROUP = {
    0x00: "HSP",
    0x01: "SCP",
    0x02: "MCP",
    0x04: "CDED",
    0x05: "SDM",
    0x06: "AP",
}

COMPONENT_SUBGROUP = {
    0x00: "GENERIC",
    0x01: "BOOTLOADER",
    0x02: "MESH",
    0x03: "TOWER",
    0x04: "ACCEL",
    0x05: "DDR",
    0x06: "CRASH_DUMP",
    0x07: "TELEMETRY",
    0x08: "POWER",
    0x09: "HEALTH_MONITOR",
}

COMPONENT_INSTANCE = {
    0x00: "SCP_PRIMARY",
    0x01: "SCP_SECONDARY",
    0x02: "MCP_PRIMARY",
    0x03: "MCP_SECONDARY",
    0x04: "CDED_PRIMARY",
    0x05: "CDED_SECONDARY",
    0x06: "SDM_PRIMARY",
    0x07: "SDM_SECONDARY",
}

# mscp_boot_status_code_t - sequential enum starting at 0
MSCP_BOOT_STATUS_CODES = [
    # 0x00
    "UNUSED",
    # SCP progress codes
    "SCP_OK",                              # 0x01
    "SCP_START",                           # 0x02
    "SCP_COLD_BOOT",                       # 0x03
    "SCP_WARM_BOOT",                       # 0x04
    "SCP_IRQ_DISABLED",                    # 0x05
    "SCP_I3C_INIT_START",                  # 0x06
    "SCP_I3C_INIT_ERROR",                  # 0x07
    "SCP_I3C_INIT_END",                    # 0x08
    "SCP_MESH_INIT_END",                   # 0x09
    "SCP_TOWER_INIT_END",                  # 0x0a
    "SCP_ACCEL_INIT_END",                  # 0x0b
    "SCP_DDR_INIT_END",                    # 0x0c
    "SCP_SPI_INIT_END",                    # 0x0d
    "SCP_D2DMBX_INIT_END",                # 0x0e
    "SCP_PRE_GTIMER_INIT_END",            # 0x0f
    "SCP_CFGMGR_INIT_END",                # 0x10
    "SCP_CFGMGR_OVERRIDE_CHECK_END",      # 0x11
    "SCP_CFGMGR_OVERRIDE_APPLY_END",      # 0x12
    "SCP_UART_INIT_END",                   # 0x13
    "SCP_STDIO_INIT_END",                  # 0x14
    "SCP_POST_GTIMER_INIT_END",            # 0x15
    "SCP_PRE_CSS_INIT_END",                # 0x16
    "SCP_GPIO_INIT_END",                   # 0x17
    "SCP_CD_INIT_END",                     # 0x18
    "SCP_PRE_FUSE_INIT_END",              # 0x19
    "SCP_D2DCNTR_INIT_END",               # 0x1a
    "SCP_POST_CSS_INIT_END",               # 0x1b
    "SCP_VAB_INIT_END",                    # 0x1c
    "SCP_SHARED_MEM_INIT_END",             # 0x1d
    "SCP_D2D_MHU_INIT_END",               # 0x1e
    "SCP_POST_FUSE_INIT_END",             # 0x1f
    "SCP_COREINFO_INIT_END",              # 0x20
    "SCP_HW_SEM_INIT_END",                # 0x21
    "SCP_VIRT_IRQ_INIT_END",              # 0x22
    "SCP_PEX_INIT_END",                   # 0x23
    "SCP_HM_INIT_END",                    # 0x24
    "SCP_APCORE_INIT_END",                # 0x25
    "SCP_SOS_INIT_END",                   # 0x26
    "SCP_SDM_MHU_INIT_END",               # 0x27
    "SCP_CDED_MHU_INIT_END",              # 0x28
    "SCP_MCSP2MSCP_MHU_INIT_END",         # 0x29
    "SCP_WS_INIT_END",                    # 0x2a
    "SCP_POWER_INIT_END",                 # 0x2b
    "SCP_RAS_INIT_END",                   # 0x2c
    "SCP_ETC_INIT_END",                   # 0x2d
    "SCP_ETD_INIT_END",                   # 0x2e
    "SCP_MTS_INIT_START",                 # 0x2f
    "SCP_MTS_INIT_END",                   # 0x30
    "SCP_IB_TELM_INIT_END",              # 0x31
    "SCP_MSCP_APRT_MHU_INIT_END",        # 0x32
    "SCP_MSCP_APS_MHU_INIT_END",         # 0x33
    "SCP_MSCP_APNS_MHU_INIT_END",        # 0x34
    "SCP_MSCP_TFA_MHU_INIT_END",         # 0x35
    "SCP_MSCP_IOSS_INIT_END",            # 0x36
    "SCP_MSCP_PCIE_INIT_END",            # 0x37
    "SCP_POWER_TLM_SCP_INIT_END",        # 0x38
    "SCP_POWER_TLM_CORE_INIT_END",       # 0x39
    "SCP_POWER_SCMI_INIT_END",           # 0x3a
    "SCP_RTOSMON_INIT_END",              # 0x3b
    "SCP_SEL_INIT_START",                # 0x3c
    "SCP_SEL_INIT_END",                  # 0x3d
    "SCP_UTC_SYNC_INIT_START",           # 0x3e
    "SCP_UTC_SYNC_INIT_END",             # 0x3f
    "SCP_WD_INIT_END",                   # 0x40
    "SCP_SENSOR_FIFO_START",             # 0x41
    "SCP_SENSOR_FIFO_END",              # 0x42
    # Boot status after runtime init
    "SOS_PCIE_PHY_END",                  # 0x43
    "SOS_MSCP_MANIFEST_END",            # 0x44
    "SOS_STARTUP_ACCEL_MANIFEST_END",   # 0x45
    "SOS_MCP_END",                       # 0x46
    "SOS_SDM_ITCM_END",                 # 0x47
    "SOS_SDM_DTCM_END",                 # 0x48
    "SOS_CDED_ITCM_END",                # 0x49
    "SOS_CDED_DTCM_END",                # 0x4a
    "SOS_CLUSTER_CORE_END",             # 0x4b
    "SOS_AP_SOC_POWER_END",             # 0x4c
    "SOS_AP_SOC_POWER_POST_SYNC_END",   # 0x4d
    "SOS_AP_SOC_POWER_POST_SCF_SYNC_END",  # 0x4e
    "SOS_BL31_END",                      # 0x4f
    "SOS_STMM_END",                      # 0x50
    "SOS_BL33_END",                      # 0x51
    "SOS_HAFNIUM_END",                   # 0x52
    "SOS_RMM_END",                       # 0x53
    "SOS_RP_DATA_END",                   # 0x54
    "SOS_RP_EXE_END",                    # 0x55
    "SOS_DIE_CFG_END",                   # 0x56
    "SOS_PCIE_LTSSM_END",               # 0x57
    "SOS_SDM_BOOT_END",                  # 0x58
    "SOS_CDED_BOOT_END",                 # 0x59
    "SOS_PRIMARY_APCORE_BOOT_END",       # 0x5a
    # DDR detailed progress codes
    "SCP_DDR_PCR_INIT_START",            # 0x5b
    "SCP_DDR_PCR_INIT_END",             # 0x5c
    "SCP_DDR_MANAGER_INIT_START",        # 0x5d
    "SCP_DDR_TRAINING_START",            # 0x5e
    "SCP_DDR_TRAINING_END",             # 0x5f
    "SCP_DDR_SDL_LOAD_END",             # 0x60
    "SCP_DDR_MEMORY_MAP_END",           # 0x61
    "SCP_DDR_HSP_NOTIFY_END",           # 0x62
    "SCP_DDR_MANAGER_INIT_END",         # 0x63
    # Error codes
    "SCP_E_SPI_BRIDGE_CONFIG",           # 0x64
    "SCP_E_SPI_CTRL_CONFIG",             # 0x65
    "SCP_E_BOOT_CONFIG",                 # 0x66
    "SCP_MAX",                           # 0x67
    # MCP progress codes
    "MCP_OK",                            # 0x68
    "MCP_START",                         # 0x69
    "MCP_COLD_BOOT",                     # 0x6a
    "MCP_WARM_BOOT",                     # 0x6b
    "MCP_IRQ_DISABLED",                  # 0x6c
    "MCP_VARSVC_INIT_END",               # 0x6d
    "MCP_CFGMGR_INIT_END",              # 0x6e
    "MCP_SPI_INIT_END",                  # 0x6f
    "MCP_D2DMBX_INIT_END",              # 0x70
    "MCP_PRE_GTIMER_INIT_END",          # 0x71
    "MCP_POST_GTIMER_INIT_END",         # 0x72
    "MCP_PRE_CSS_INIT_END",             # 0x73
    "MCP_POST_CSS_INIT_END",            # 0x74
    "MCP_GPIO_INIT_END",                # 0x75
    "MCP_PRE_FUSE_INIT_END",            # 0x76
    "MCP_POST_FUSE_INIT_END",           # 0x77
    "MCP_D2DCNTR_INIT_END",             # 0x78
    "MCP_MESH_INIT_END",                # 0x79
    "MCP_UART_INIT_END",                # 0x7a
    "MCP_STDIO_INIT_END",               # 0x7b
    "MCP_CD_INIT_END",                   # 0x7c
    "MCP_VAB_INIT_END",                  # 0x7d
    "MCP_SHARED_MEM_INIT_END",          # 0x7e
    "MCP_D2D_MHU_INIT_END",             # 0x7f
    "MCP_COREINFO_INIT_END",            # 0x80
    "MCP_HW_SEM_INIT_END",              # 0x81
    "MCP_VIRT_IRQ_INIT_END",            # 0x82
    "MCP_PEX_INIT_END",                 # 0x83
    "MCP_HM_INIT_END",                  # 0x84
    "MCP_APCORE_INIT_END",              # 0x85
    "MCP_SOS_INIT_END",                 # 0x86
    "MCP_WS_INIT_END",                  # 0x87
    "MCP_POWER_INIT_END",               # 0x88
    "MCP_RAS_INIT_END",                 # 0x89
    "MCP_E_BOOT_CONFIG",                # 0x8a
    "MCP_MCTP_INIT_START",              # 0x8b
    "MCP_MCTP_INIT_END",                # 0x8c
    "MCP_PLDM_START",                   # 0x8d
    "MCP_PLDM_END",                     # 0x8e
    "MCP_MTS_INIT_START",               # 0x8f
    "MCP_MTS_INIT_END",                 # 0x90
    "MCP_SENSOR_FIFO_START",            # 0x91
    "MCP_SENSOR_FIFO_END",              # 0x92
    "MCP_SEL_INIT_START",               # 0x93
    "MCP_SEL_INIT_END",                 # 0x94
    "MCP_UTC_SYNC_INIT_START",          # 0x95
    "MCP_UTC_SYNC_INIT_END",            # 0x96
    "MCP_MAX",                           # 0x97
    "BOOT_STATUS_CODE_MAX",             # 0x98
]

# LED status codes from led_status_codes_t
LED_STATUS_CODES = {
    0x00: "SCP_OK",
    0x01: "SCP_MESH_INIT_START",
    0x02: "SCP_TOWER_INIT_START",
    0x03: "SCP_ACCEL_INIT_START",
    0x04: "SCP_DDR_INIT_START",
    0x05: "SCP_BOOT_COMPLETE",
    0x06: "MCP_OK",
    0x07: "MCP_BOOT_COMPLETE",
    0x08: "SDM_OK",
    0x09: "SDM_BOOT_COMPLETE",
    0x0a: "CDED_OK",
    0x0b: "CDED_BOOT_COMPLETE",
    0x60: "SCP_ACCEL_FAILED",
    0x61: "SCP_E_DDR0_TRAINING",
    0x62: "SCP_E_DDR1_TRAINING",
    0x63: "SCP_E_DDR2_TRAINING",
    0x64: "SCP_E_DDR3_TRAINING",
    0x65: "SCP_E_DDR4_TRAINING",
    0x66: "SCP_E_DDR5_TRAINING",
    0x67: "SDM_HW_FAULT",
    0x68: "CDED_HW_FAULT",
}

# HSP boot_status_codes.h LED values (from HSP artifacts, common known values)
# These are the values programmed into bits 24-31 of boot_status_ex
# when using GEN_BOOT_STATUS_EX_LED_CODE via post_led_status()
HSP_LED_CODES = {
    # Die 0 SCP codes (base 0x40)
    0x40: "HSP_SCP_DIE0_OK",
    0x41: "HSP_SCP_DIE0_MESH_INIT_START",
    0x42: "HSP_SCP_DIE0_TOWER_INIT_START",
    0x43: "HSP_SCP_DIE0_ACCEL_INIT_START",
    0x44: "HSP_SCP_DIE0_DDR_INIT_START",
    0x45: "HSP_SCP_DIE0_BOOT_COMPLETE",
    # Die 1 SCP codes (base 0x46)
    0x46: "HSP_SCP_DIE1_OK",
    0x47: "HSP_SCP_DIE1_MESH_INIT_START",
    0x48: "HSP_SCP_DIE1_TOWER_INIT_START",
    0x49: "HSP_SCP_DIE1_ACCEL_INIT_START",
    0x4a: "HSP_SCP_DIE1_DDR_INIT_START",
    0x4b: "HSP_SCP_DIE1_BOOT_COMPLETE",
    # MCP codes
    0x4c: "HSP_MCP_DIE0_OK",
    0x4d: "HSP_MCP_DIE0_BOOT_COMPLETE",
    0x4e: "HSP_MCP_DIE1_OK",
    0x4f: "HSP_MCP_DIE1_BOOT_COMPLETE",
    # SDM codes
    0x50: "HSP_SDM_DIE0_OK",
    0x51: "HSP_SDM_DIE0_BOOT_COMPLETE",
    0x52: "HSP_SDM_DIE1_OK",
    0x53: "HSP_SDM_DIE1_BOOT_COMPLETE",
    # CDED codes
    0x54: "HSP_CDED_DIE0_OK",
    0x55: "HSP_CDED_DIE0_BOOT_COMPLETE",
    0x56: "HSP_CDED_DIE1_OK",
    0x57: "HSP_CDED_DIE1_BOOT_COMPLETE",
    # Error codes (base 0xA0)
    0xA0: "HSP_SCP_DIE0_ACCEL_FAILED",
    0xA1: "HSP_SCP_DIE0_E_DDR0_TRAINING",
    0xA2: "HSP_SCP_DIE0_E_DDR1_TRAINING",
    0xA3: "HSP_SCP_DIE0_E_DDR2_TRAINING",
    0xA4: "HSP_SCP_DIE0_E_DDR3_TRAINING",
    0xA5: "HSP_SCP_DIE0_E_DDR4_TRAINING",
    0xA6: "HSP_SCP_DIE0_E_DDR5_TRAINING",
    0xA7: "HSP_SDM_DIE0_HW_FAULT",
    0xA8: "HSP_CDED_DIE0_HW_FAULT",
}


def decode_boot_status_ex(value):
    """Decode boot_status_ex into group, subgroup, instance, led_status."""
    group = (value >> 0) & 0xFF
    subgroup = (value >> 8) & 0xFF
    instance = (value >> 16) & 0xFF
    led_status = (value >> 24) & 0xFF
    return group, subgroup, instance, led_status


def get_boot_status_name(code):
    """Look up MSCP boot status code name."""
    if code < len(MSCP_BOOT_STATUS_CODES):
        return MSCP_BOOT_STATUS_CODES[code]
    return f"UNKNOWN(0x{code:02x})"


def get_group_name(group):
    return COMPONENT_GROUP.get(group, f"UNKNOWN(0x{group:02x})")


def get_subgroup_name(subgroup):
    return COMPONENT_SUBGROUP.get(subgroup, f"UNKNOWN(0x{subgroup:02x})")


def get_instance_name(instance):
    return COMPONENT_INSTANCE.get(instance, f"UNKNOWN(0x{instance:02x})")


def get_led_name(led_code):
    """Look up LED code name from HSP artifacts."""
    if led_code == 0:
        return "(none)"
    name = HSP_LED_CODES.get(led_code, None)
    if name:
        return name
    name = LED_STATUS_CODES.get(led_code, None)
    if name:
        return f"LED_{name}"
    return f"0x{led_code:02x}"


def extract_hex_bytes(raw_input):
    """
    Extract hex byte values from the raw input string.
    Handles both 'sh system bios code' output and bare hex values.
    """
    # Remove the "sh system bios code ..." header and "Bios Code:" prefix
    text = raw_input
    # Try to find the data after "Bios Code:" if present
    match = re.search(r'Bios Code:\s*(.*)', text, re.DOTALL)
    if match:
        text = match.group(1)

    # Split on whitespace and parse each token as hex
    tokens = text.split()
    hex_bytes = []
    for token in tokens:
        token = token.strip()
        if not token:
            continue
        try:
            val = int(token, 16)
            if 0 <= val <= 0xFF:
                hex_bytes.append(val)
            else:
                # Skip tokens that aren't single bytes
                continue
        except ValueError:
            # Skip non-hex tokens (e.g., "sh", "system", etc.)
            continue

    return hex_bytes


def bytes_to_u32_le(b0, b1, b2, b3):
    """Convert 4 bytes (little-endian) to a 32-bit unsigned integer."""
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24)


def decode_bios_codes(raw_input):
    """
    Parse and decode boot status codes.

    The HSP history buffer stores pairs of (boot_status_ex, boot_status),
    each as a 32-bit little-endian value (4 bytes each, 8 bytes per entry).
    """
    hex_bytes = extract_hex_bytes(raw_input)

    if len(hex_bytes) < 8:
        print(f"Error: Need at least 8 bytes for one entry, got {len(hex_bytes)}")
        return

    # Group into 32-bit words (4 bytes each, little-endian)
    words = []
    for i in range(0, len(hex_bytes) - 3, 4):
        w = bytes_to_u32_le(hex_bytes[i], hex_bytes[i + 1],
                            hex_bytes[i + 2], hex_bytes[i + 3])
        words.append(w)

    if len(words) < 2:
        print("Error: Not enough data for even one entry.")
        return

    # Each entry is a pair: (boot_status_ex, boot_status)
    # boot_status_ex is first, boot_status is second
    entries = []
    i = 0
    while i + 1 < len(words):
        boot_status_ex = words[i]
        boot_status = words[i + 1]
        entries.append((boot_status_ex, boot_status))
        i += 2

    # Print header
    hdr_fmt = "{:>3s}  {:>10s}  {:>10s}  {:>8s}  {:>12s}  {:>14s}  {:>6s}  {:>35s}  {}"
    row_fmt = "{:>3d}  0x{:08x}  0x{:08x}  {:>8s}  {:>12s}  {:>14s}  {:>6s}  {:>35s}  {}"

    print()
    print(hdr_fmt.format(
        "#", "status_ex", "status", "Group", "Subgroup", "Instance", "LED",
        "Boot Status Name", "LED Name"
    ))
    print("-" * 160)

    for idx, (boot_status_ex, boot_status) in enumerate(entries):
        group, subgroup, instance, led = decode_boot_status_ex(boot_status_ex)

        group_name = get_group_name(group)
        subgroup_name = get_subgroup_name(subgroup)
        instance_name = get_instance_name(instance)
        led_hex = f"0x{led:02x}" if led != 0 else "-"
        status_name = get_boot_status_name(boot_status)
        led_name = get_led_name(led)

        entry_type = ""
        if led != 0:
            entry_type = f"[LED] {led_name}"
        else:
            entry_type = ""

        print(row_fmt.format(
            idx + 1,
            boot_status_ex,
            boot_status,
            group_name,
            subgroup_name,
            instance_name,
            led_hex,
            status_name,
            entry_type,
        ))

    # Summary
    print()
    print(f"Total entries: {len(entries)}")

    # Find last meaningful status per instance
    last_per_instance = {}
    for boot_status_ex, boot_status in entries:
        group, subgroup, instance, led = decode_boot_status_ex(boot_status_ex)
        if boot_status != 0:  # Skip UNUSED
            key = (group, instance)
            last_per_instance[key] = (boot_status_ex, boot_status)

    if last_per_instance:
        print("\nLast status per component instance:")
        print(f"  {'Group':<8s}  {'Instance':<16s}  {'Last Boot Status':<40s}  {'LED'}")
        print("  " + "-" * 90)
        for (group, instance), (bsx, bs) in sorted(last_per_instance.items()):
            _, _, _, led = decode_boot_status_ex(bsx)
            print(f"  {get_group_name(group):<8s}  {get_instance_name(instance):<16s}  "
                  f"{get_boot_status_name(bs):<40s}  {get_led_name(led)}")


def main():
    parser = argparse.ArgumentParser(
        description="Decode MSCP boot status codes from Rack Manager BIOS code output."
    )
    parser.add_argument("input", nargs="*",
                        help="Hex byte string (or pass via stdin)")
    parser.add_argument("-f", "--file", type=str,
                        help="Read input from a file")

    args = parser.parse_args()

    if args.file:
        with open(args.file, "r") as f:
            raw_input = f.read()
    elif args.input:
        raw_input = " ".join(args.input)
    elif not sys.stdin.isatty():
        raw_input = sys.stdin.read()
    else:
        print("Paste boot status codes (Ctrl+Z then Enter to finish on Windows):")
        raw_input = sys.stdin.read()

    decode_bios_codes(raw_input)


if __name__ == "__main__":
    main()