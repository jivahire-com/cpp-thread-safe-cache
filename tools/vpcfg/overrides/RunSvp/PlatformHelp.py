"""This script will define the API for Platform specific help messages.
The Platform specific file should copy/overwrite this one"""

# python standard library
import platform

def platform_help_message():
    """Returns the platform specific help message"""
    
    line_break = "`"
    if platform.system() == 'Linux':
        line_break = "\\"

    help_string = "Example Command Structure: \n"
    help_string += f'vssh -d "path/to/workspace" {line_break}\n \
    -s "path/to/run_fixed_vdk.py" {line_break}\n \
    --pyargs {line_break}\n \
    --help {line_break}\n \
    --pyargs_end'

    return help_string