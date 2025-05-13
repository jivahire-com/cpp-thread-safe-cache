# @ BootloaderEmbed.py
##
# Copyright (c) 2022, Microsoft Corporation. All rights reserved.
#
import sys
import os
import uuid
import zlib
import struct
import subprocess
import re
from collections import OrderedDict

#
# Generates the payload portion of the bootloader
#
# Creates build rules to:
#   * Split the main image .elf in to separate .elf.dtcm.bin, .elf.itcm.bin & .elf.rmss.bin files for the ITCM, DTCM & RMSS regions
#   * Compress the .bin files in to .bin.gz files
#   * Run a script to combine the .gz files into .elf.embed
#     The .embed file will begin with a header followed by the content of the .gz files
#
#  struct embed_image_section {
#      uint32_t compressed_offset;  // Offset from beginning of the embed_image_header to the first byte of the compressed image
#      uint32_t compressed_size;    // Compressed size of the image
#      uint32_t uncompressed_size;  // Uncompressed size of the image
#      uint32_t uncompressed_crc32; // CRC32 checksum of the uncompressed image    
#  }
#  struct embed_image_header {
#      uint32_t embed_image_header_tag;  // 0xEBDD4EAD
#      struct embed_image_section iram;  // Location and size of the iram image
#      struct embed_image_section dram;  // Location and size of the dram image
#      struct embed_image_section dram;  // Location and size of the rmss image
#  }
#

class Section:
    def __init__(self, idx, name, size, vma, lma, fileoff, align):
        self.idx = int(idx)
        self.name = name
        self.size = int(size, 16)
        self.vma = int(vma, 16)
        self.lma = int(lma, 16)
        self.fileoff = int(fileoff, 16)
        self.align = align


def get_sections(objdump, elf):    
    proc = subprocess.Popen(
        "{} -h {}".format(objdump, elf).split(' '),
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        text = True)
    stdout, stderr = proc.communicate()

    if stderr != "":
        sys.stderr.write(stderr)

    if proc.returncode != 0:
        raise Exception("objdump returned: {}".format(proc.returncode))
    
    expr = r"^\s*(?P<idx>\d+)\s+(?P<name>[a-zA-Z0-9_\.]+)\s+(?P<size>[a-fA-F0-9]+)\s+(?P<vma>[a-fA-F0-9]+)\s+(?P<lma>[a-fA-F0-9]+)\s+(?P<fileoff>[a-fA-F0-9]+)\s+(?P<align>[0-9a-fA-F\*]+)\s*$"
    sections = OrderedDict()
    for section_regex in re.findall(expr, stdout, re.MULTILINE):
        section = Section(
            section_regex[0],
            section_regex[1],
            section_regex[2],
            section_regex[3],
            section_regex[4],
            section_regex[5],
            section_regex[6])
        sections[section.name] = section

    return sections
        


# This script is directly called from CreateBootloaderEmbed.cmake
# Input arguments should match inputs from that cmake script
def main():

    # Input files
    ITCM_BIN_PATH = sys.argv[1]
    DTCM_BIN_PATH = sys.argv[2]
    RMSS_BIN_PATH = sys.argv[3]
    ITCM_BIN_GZ_PATH = sys.argv[4]
    DTCM_BIN_GZ_PATH = sys.argv[5]
    RMSS_BIN_GZ_PATH = sys.argv[6]
    BOOTLOADER_ELF_PATH = sys.argv[7]
    OBJDUMP_PATH = sys.argv[8]

    # Output file
    EMBED_PATH = sys.argv[9]

    # Size checks
    ITCM_BASE = int(sys.argv[10],0)
    ITCM_SIZE = int(sys.argv[11],0)
    DTCM_BASE = int(sys.argv[12],0)
    DTCM_SIZE = int(sys.argv[13],0)
    RMSS_BASE = int(sys.argv[14],0)
    RMSS_SIZE = int(sys.argv[15],0)

    itcm_crc = 0
    dtcm_crc = 0
    rmss_crc = 0

    with open(ITCM_BIN_PATH, 'rb') as itcm_bin_file:
        itcm_uncompressed = itcm_bin_file.read()
    with open(DTCM_BIN_PATH, 'rb') as dtcm_bin_file:
        dtcm_uncompressed = dtcm_bin_file.read()
    with open(RMSS_BIN_PATH, 'rb') as rmss_bin_file:
        rmss_uncompressed = rmss_bin_file.read()

    with open(ITCM_BIN_GZ_PATH, 'rb') as itcm_bin_gz_file:
        itcm_compressed = itcm_bin_gz_file.read()
    with open(DTCM_BIN_GZ_PATH, 'rb') as dtcm_bin_gz_file:
        dtcm_compressed = dtcm_bin_gz_file.read()
    with open(RMSS_BIN_GZ_PATH, 'rb') as rmss_bin_gz_file:
        rmss_compressed = rmss_bin_gz_file.read()

    embed_file_size = 0

    if(not os.path.exists(BOOTLOADER_ELF_PATH)):
        raise Exception("Bootloader elf not found")

    with open(EMBED_PATH, 'wb') as embed_file:

        header_format = "<IIIIIIIIIIIII"

        embed_image_header_tag = 0xEBDD4EAD
        iram_compressed_offset = struct.calcsize(header_format)
        iram_compressed_size = len(itcm_compressed)
        iram_uncompressed_size = len(itcm_uncompressed)
        iram_uncompressed_crc32 = zlib.crc32(itcm_uncompressed)

        dram_compressed_offset = iram_compressed_offset + iram_compressed_size
        dram_compressed_size = len(dtcm_compressed)
        dram_uncompressed_size = len(dtcm_uncompressed)
        dram_uncompressed_crc32 = zlib.crc32(dtcm_uncompressed)

        rmss_compressed_offset = dram_compressed_offset + dram_compressed_size
        rmss_compressed_size = len(rmss_compressed)
        rmss_uncompressed_size = len(rmss_uncompressed)
        rmss_uncompressed_crc32 = zlib.crc32(rmss_uncompressed)
        
        if iram_uncompressed_size > ITCM_SIZE:
            sys.stderr.write("ITCM section uncompressed exceeds size limit of {} by {}.\n".format(
                ITCM_SIZE,
                iram_uncompressed_size - ITCM_SIZE
            ))
            sys.exit(1)

        if dram_uncompressed_size > DTCM_SIZE:
            sys.stderr.write("DTCM section uncompressed exceeds size limit of {} by {}.\n".format(
                DTCM_SIZE,
                dram_uncompressed_size - DTCM_SIZE
            ))
            sys.exit(1)

        if rmss_uncompressed_size > RMSS_SIZE:
            sys.stderr.write("RMSS section uncompressed exceeds size limit of {} by {}.\n".format(
                RMSS_SIZE,
                rmss_uncompressed_size - RMSS_SIZE
            ))
            sys.exit(1)

        header = struct.pack(header_format,
            embed_image_header_tag,
            iram_compressed_offset,
            iram_compressed_size,
            iram_uncompressed_size,
            iram_uncompressed_crc32,
            dram_compressed_offset,
            dram_compressed_size,
            dram_uncompressed_size,
            dram_uncompressed_crc32,
            rmss_compressed_offset,
            rmss_compressed_size,
            rmss_uncompressed_size,
            rmss_uncompressed_crc32)
        
        embed_file.write(header)
        embed_file.write(itcm_compressed)
        embed_file.write(dtcm_compressed)
        embed_file.write(rmss_compressed)

        # Get the size of the file from the current file position
        embed_file_size = embed_file.tell()

    
    # Get the sections from the bootloader
    bootloader_sections = get_sections(OBJDUMP_PATH, BOOTLOADER_ELF_PATH)

    # Get the maximum size of the embedded file from the .mainimage section of the bootloader
    max_embed_size = bootloader_sections[".mainimage"].size

    if max_embed_size < embed_file_size:
        sys.stderr.write("Embed file '{}' exceeds size limit of {} by {}.\n".format(
            EMBED_PATH,
            max_embed_size,
            embed_file_size - max_embed_size
        ))
        sys.exit(1)


    # This is the first address in the DTCM that is needed by the bootloader at runtime
    bootloader_data_start = bootloader_sections[".data"].vma

    # Find the address of the largest initialized data in the main image
    mainimage_data_start = DTCM_BASE
    mainimage_data_end = DTCM_BASE + len(dtcm_uncompressed)

    # Verify that the end of the initialized data from the main image does not extend in to the
    # area of DTCM used at runtime by the bootloader
    if mainimage_data_end > bootloader_data_start:
        sys.stderr.write("Main image initialized data of {} bytes overlaps bootloader runtime data at 0x{:x}.\n".format(
            len(dtcm_uncompressed),
            bootloader_data_start
        ))
        sys.exit(1)


    return


if __name__ == '__main__':
    sys.exit(main())
