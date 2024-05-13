# @ CompressFiles.py
##
# Copyright (c) 2024, Microsoft Corporation. All rights reserved.
#
import gzip
import sys

# This script is directly called from CreateBootloader.cmake
# and compresses ITCM and DTCM section files into .gz files
# Input arguments should match inputs from that cmake script
def main():

    # Input files
    ITCM_INPUT_PATH = sys.argv[1]
    ITCM_OUTPUT_PATH = sys.argv[2]
    
    with open(ITCM_INPUT_PATH, 'rb') as f_in, gzip.open(ITCM_OUTPUT_PATH, 'wb') as f_out:
        f_out.write(f_in.read())
    
    return

if __name__ == '__main__':
    sys.exit(main())