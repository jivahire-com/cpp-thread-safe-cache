# Copyright (c) Microsoft Corporation. All rights reserved.

#!/bin/env python3
"""
This script generates a summary of firmware image sizes and section sizes for different cores
by parsing ELF files using the readelf tool

Dependencies:
- The script requires the 'adoutil' tool to post comments in Azure DevOps.

Usage:
- Run the script to generate and print the summary.
- If running in an Azure DevOps pipeline, the summary is posted as a comment on the pull request.
"""
import os
import re
import subprocess

def to_kb(value: int) -> str:
    return f"{value / 1024:.1f}"

READELF_FORMAT = r"\.{}[\s]+[a-zA-Z]+[\s]+[0-9a-f]+[\s]+[0-9a-f]+[\s]+([0-9a-f]+)"
READELF_PATH = f"{os.environ['REPO_APP_PATH_gcc.arm.eabi.aarch-win64']}/bin/arm-none-eabi-readelf.exe"
FW_PATH_FORMAT = os.environ["REPO_APP_TARGET_BUILD_DIR"] + "/bin/{core}/"
FW_CORES = ["scp", "mcp"]

# Memory limits (bytes)
DTCM_LIMIT = 512 * 1024
ITCM_LIMIT = 512 * 1024

ELF_SECTIONS = [
    "text",
    "rodata.itcm",
    "rodata.rmss",
    "placed.code",
    "data",
    "bss",
    "stack",
    "heap",
]
LINKER_VARS = [
    "ro_data_size",
    "ro_code_size",
    "ro_metadata_size",
    "ro_total_used_size",
    "ro_total_size",
]

comment_string = ""
for core in FW_CORES:
    # get paths to fw images
    elf_path = FW_PATH_FORMAT.format(core=core) + f"{core}_fw.elf"
    embed_path = FW_PATH_FORMAT.format(core=core) + f"{core}_fw.elf.embed"
    dtcm_path = FW_PATH_FORMAT.format(core=core) + f"{core}_fw.elf.dtcm.bin"
    itcm_path = FW_PATH_FORMAT.format(core=core) + f"{core}_fw.elf.itcm.bin"
    dtcm_gz_path = FW_PATH_FORMAT.format(core=core) + f"{core}_fw.elf.dtcm.bin.gz"
    itcm_gz_path = FW_PATH_FORMAT.format(core=core) + f"{core}_fw.elf.itcm.bin.gz"

    # get size information
    compressed_size = os.path.getsize(embed_path)
    dtcm_size = os.path.getsize(dtcm_path)
    itcm_size = os.path.getsize(itcm_path)
    dtcm_compressed_size = os.path.getsize(dtcm_gz_path)
    itcm_compressed_size = os.path.getsize(itcm_gz_path)

    available_dtcm = DTCM_LIMIT - dtcm_size
    available_itcm = ITCM_LIMIT - itcm_size

    # Section sizes
    readelf_output = subprocess.run(
        [READELF_PATH, "-S", elf_path],
        stdout=subprocess.PIPE,
        text=True
    ).stdout

    section_sizes = {}
    for section in ELF_SECTIONS:
        match = re.search(READELF_FORMAT.format(section), readelf_output)
        if match:
            section_sizes[section] = int(match.group(1), 16)

    # Linker variables
    variable_values = {}
    for variable in LINKER_VARS:
        command = f'powershell "{READELF_PATH} -s {elf_path} | Select-String {variable}"'
        result = subprocess.run(command, capture_output=True, text=True, shell=True)
        if result.stdout.strip():
            hex_value = result.stdout.strip().split()[1]
            variable_values[variable] = int(hex_value, 16)

    available_rmss = (
        variable_values.get("ro_total_size", 0)
        - variable_values.get("ro_total_used_size", 0)
    )

    # Table formatting
    name_w = 30
    bytes_w = 14
    kb_w = 10

    comment_string += f"""
## {core.upper()}
| {'Image'.ljust(name_w)} | {'Size (bytes)'.rjust(bytes_w)} | {'Size (KB)'.rjust(kb_w)} |
|{'-' * (name_w+2)}|{'-' * (bytes_w+2)}|{'-' * (kb_w+2)}|
| {'Main Image(compressed)'.ljust(name_w)} | {str(compressed_size).rjust(bytes_w)} | {to_kb(compressed_size).rjust(kb_w)} |
| {'DTCM (**Must be < 512k**)'.ljust(name_w)} | {str(dtcm_size).rjust(bytes_w)} | {to_kb(dtcm_size).rjust(kb_w)} |
| {'Available DTCM'.ljust(name_w)} | {str(available_dtcm).rjust(bytes_w)} | {to_kb(available_dtcm).rjust(kb_w)} |
| {'ITCM (**Must be < 512k**)'.ljust(name_w)} | {str(itcm_size).rjust(bytes_w)} | {to_kb(itcm_size).rjust(kb_w)} |
| {'Available ITCM'.ljust(name_w)} | {str(available_itcm).rjust(bytes_w)} | {to_kb(available_itcm).rjust(kb_w)} |
| {'DTCM (compressed)'.ljust(name_w)} | {str(dtcm_compressed_size).rjust(bytes_w)} | {to_kb(dtcm_compressed_size).rjust(kb_w)} |
| {'ITCM (compressed)'.ljust(name_w)} | {str(itcm_compressed_size).rjust(bytes_w)} | {to_kb(itcm_compressed_size).rjust(kb_w)} |

| {'Section'.ljust(name_w)} | {'Size (bytes)'.rjust(bytes_w)} | {'Size (KB)'.rjust(kb_w)} |
|{'-' * (name_w+2)}|{'-' * (bytes_w+2)}|{'-' * (kb_w+2)}|
"""

    for section, size in section_sizes.items():
        comment_string += (
            f"| {section.ljust(name_w)} | "
            f"{str(size).rjust(bytes_w)} | "
            f"{to_kb(size).rjust(kb_w)} |\n"
        )

    comment_string += f"""
| {'Variables'.ljust(name_w)} | {'Value (bytes)'.rjust(bytes_w)} | {'Value (KB)'.rjust(kb_w)} |
|{'-' * (name_w+2)}|{'-' * (bytes_w+2)}|{'-' * (kb_w+2)}|
"""

    for var, value in variable_values.items():
        comment_string += (
            f"| {var.ljust(name_w)} | "
            f"{str(value).rjust(bytes_w)} | "
            f"{to_kb(value).rjust(kb_w)} |\n"
        )

    comment_string += (
        f"| {'Available RMSS Data'.ljust(name_w)} | "
        f"{str(available_rmss).rjust(bytes_w)} | "
        f"{to_kb(available_rmss).rjust(kb_w)} |\n"
    )

print(comment_string)

# Post to PR if running in pipeline
if os.getenv("TF_BUILD") and os.getenv("SYSTEM_PULLREQUEST_PULLREQUESTID"):
    try:
        subprocess.run(
            [
                "adoutil",
                "prs",
                "add-comment",
                os.environ["SYSTEM_TEAMPROJECTID"],
                os.environ["BUILD_REPOSITORY_NAME"],
                os.environ["SYSTEM_PULLREQUEST_PULLREQUESTID"],
                comment_string,
                "--status",
                "closed",
            ],
            check=True
        )
    except Exception as e:
        print(f"adoutil failed to publish comment [{e}]")