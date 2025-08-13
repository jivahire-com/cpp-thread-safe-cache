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

READELF_FORMAT = "\.{}[\s]+[a-zA-Z]+[\s]+[0-9a-f]+[\s]+[0-9a-f]+[\s]+([0-9a-f]+)"
READELF_PATH = f"{os.environ['REPO_APP_PATH_gcc.arm.eabi.aarch-win64']}/bin/arm-none-eabi-readelf.exe"
FW_PATH_FORMAT = os.environ['REPO_APP_TARGET_BUILD_DIR'] + "/bin/{core}/"
FW_CORES = ["scp", "mcp"]
ELF_SECTIONS = ["text", "rodata.itcm", "rodata.rmss", "placed.code", "data", "bss", "stack", "heap"]
LINKER_VARS = ["RMSS_DATA_size", "RMSS_DATA_used", "RMSS_DATA_code"]

comment_string = ""
for core in FW_CORES:
    # get paths to fw images
    elf_path = (
        FW_PATH_FORMAT.format(core=core)
        + f"{core}_fw.elf"
    )
    embed_path = (
        FW_PATH_FORMAT.format(core=core)
        + f"{core}_fw.elf.embed"
    )
    dtcm_path = (
        FW_PATH_FORMAT.format(core=core)
        + f"{core}_fw.elf.dtcm.bin"
    )
    itcm_path = (
        FW_PATH_FORMAT.format(core=core)
        + f"{core}_fw.elf.itcm.bin"
    )
    dtcm_gz_path = (
        FW_PATH_FORMAT.format(core=core)
        + f"{core}_fw.elf.dtcm.bin.gz"
    )
    itcm_gz_path = (
        FW_PATH_FORMAT.format(core=core)
        + f"{core}_fw.elf.itcm.bin.gz"
    )

    # get size information
    compressed_size = os.path.getsize(embed_path)
    dtcm_size = os.path.getsize(dtcm_path)
    itcm_size = os.path.getsize(itcm_path)
    dtcm_compressed_size = os.path.getsize(dtcm_gz_path)
    itcm_compressed_size = os.path.getsize(itcm_gz_path)

    readelf_output = subprocess.run(
        [f"{READELF_PATH}", "-S", elf_path], stdout=subprocess.PIPE
    ).stdout.decode("utf-8")

    section_sizes = {}
    for section in ELF_SECTIONS:
        section_desc = re.search(
            READELF_FORMAT.format(section), readelf_output
        )
        if not section_desc:
            continue

        section_size = section_desc.groups()[0]
        section_size = int(section_size, 16)
        section_sizes[section] = section_size

    variable_values = {}
    for vairable in LINKER_VARS:
        print ({READELF_PATH}, "-s", {elf_path}, "| Select-String", {vairable})
        command = f'powershell "{READELF_PATH} -s {elf_path} | Select-String {vairable}"'
        readelf_var_string = subprocess.run(command, capture_output=True, text=True, shell=True)

        if not readelf_var_string.stdout.strip():
            continue

        hex_value = readelf_var_string.stdout.strip().split()[1]
        decimal_value = int(hex_value, 16)
        variable_values[vairable] = decimal_value

    # Define the widths for the columns
    image_column_width = 25
    size_column_width = 10

    comment_string += f"""
## {core.upper()}
| {'Image'.ljust(image_column_width)} | {'Size'.rjust(size_column_width)} |
|{'-' * (image_column_width+2)}|{'-' * (size_column_width+2)}|
| {'Main Image(compressed)'.ljust(image_column_width)} | {str(compressed_size).rjust(size_column_width)} |
| {'DTCM (**Must be < 512k**)'.ljust(image_column_width)} | {str(dtcm_size).rjust(size_column_width)} |
| {'ITCM (**Must be < 512k**)'.ljust(image_column_width)} | {str(itcm_size).rjust(size_column_width)} |
| {'DTCM (compressed)'.ljust(image_column_width)} | {str(dtcm_compressed_size).rjust(size_column_width)} |
| {'ITCM (compressed)'.ljust(image_column_width)} | {str(itcm_compressed_size).rjust(size_column_width)} |

| {'Section'.ljust(image_column_width)} | {'Size'.rjust(size_column_width)} |
|{'-' * (image_column_width+2)}|{'-' * (size_column_width+2)}|
"""
    for section in section_sizes.keys():
        comment_string += f"| {section.ljust(image_column_width)} | {str(section_sizes[section]).rjust(size_column_width)} |\n" 

#display linker variables
    comment_string += f"""
| {'Variables'.ljust(image_column_width)} | {'Value'.rjust(size_column_width)} |
|{'-' * (image_column_width+2)}|{'-' * (size_column_width+2)}|
"""
    for var in variable_values.keys():
        comment_string += f"| {var.ljust(image_column_width)} | {str(variable_values[var]).rjust(size_column_width)} |\n"

print(comment_string)

# If running in a pipeline, post comment
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
                "closed"
            ]
        )
    except Exception as e:
        print(f"adoutil failed to publish comment [{str(e)}]")
