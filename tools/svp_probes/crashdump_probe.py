# Copyright (c) Microsoft Corporation. All rights reserved.

"""
crashdump_probe.py
"""
import os
import re
import sim
import struct
from base_probe import BaseProbe

def get_core_info(instance_name):
    pattern = r'/DIE_(\d)/.*?/(MCP|SCP)/'
    match = re.search(pattern, instance_name)
    if match:
        return (f'{match.group(2)}{match.group(1)}', match.group(1), match.group(2))
    return (None, 0, 0)

def fcall_observer_cb(observer, args):
    core = observer.core_probe
    die_index = core.get_core_register_value('Core_R0')
    core_index = core.get_core_register_value('Core_R1')
    is_full = core.get_core_register_value('Core_R2')
    dump_address_ref = core.get_core_register_value('Core_R13') # SP
    dump_size_ref = dump_address_ref + 8
    dump_address = struct.unpack('Q', core.get_memory_value(dump_address_ref, 1, 8))[0]
    dump_size = struct.unpack('Q', core.get_memory_value(dump_size_ref, 1, 8))[0]
    sim.print_message(f'{core.instance_name}: die_index={die_index}, core_index={core_index}, is_full={is_full}, dump_address=0x{dump_address:x}, dump_size=0x{dump_size:x}')

    core_info = get_core_info(core.instance_name)

    if (core_info[0] != None):
        sim_output_path = f"{os.environ['REPO_APP_ROOT']}.svp_simulator/crashdump"
        dump_file_path = f"{sim_output_path}/{core_info[0]}_{'full' if is_full else 'mini'}.dmp"

        if not os.path.exists(sim_output_path):
            os.makedirs(sim_output_path)

        try:
            with open(dump_file_path, 'wb') as dump_file:
                dump_file.write(core.get_memory_value(dump_address, dump_size))
        except Exception as e:
            sim.print_message(f"Failed to write crash dump to {dump_file_path}: {e}")
    else:
        sim.print_message(f"Can't find core information")

doc = """
crashdump_probe.py
"""
class CrashdumpProbes(BaseProbe):
    cores = []
    function_observers = []

    @staticmethod
    def doc():
        return doc
    
    @staticmethod
    def fpath() -> str:
        return __file__
    
    def main(self):
        list_of_cores = sim.get_cores()
        symbol_path = f"{os.environ['REPO_APP_BUILD_DIR']}/{os.environ['REPO_APP_BUILD_CONFIG']}/arm-eabi-aarch/bin/"

        for core_name in list_of_cores:
            if ('.MCP.' in core_name or '.SCP.' in core_name):  # or '.HSP.' in core_name):
                self.cores.append(sim.CoreProbe(core_name))

        for core in self.cores:
            core_info = get_core_info(core.instance_name)
            if core_info[0] != None:
                sim.print_message(f'Loading symbol from {symbol_path}{core_info[2]}/{core_info[2]}_fw.elf for {core.instance_name}')
                core.load_image(f'{symbol_path}{core_info[2]}/{core_info[2]}_fw.elf', 0, True)

            try:
                symbol = core.find_symbol_by_name('crash_dump_svp_probe', 'function')
                fcall_observer = core.create_begin_instruction_observer(fcall_observer_cb, symbol.start_address)
                self.function_observers.append(fcall_observer)
            except:
                sim.print_message(f'crash_dump_svp_probe not found')

if __name__ == "__main__":
    cd_probes = CrashdumpProbes()
    cd_probes.main()
