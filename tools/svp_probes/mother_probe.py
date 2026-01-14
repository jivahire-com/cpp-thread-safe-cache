# Copyright (c) Microsoft Corporation. All rights reserved.

"""
mother_probe.py
"""

import sim_setup
import sim

from crashdump_probe import CrashdumpProbes

doc = """
mother_probe.py

Registers a set of probes objects listed in the mother_probe.py file
Comment out any probes or change their parameters for your specific run.
NOTE: should be the only probe with access to sim_setup
"""

class MotherProbe():
    def __init__(self, brood = []):
        self._brood = brood

    def _doc(self):
        header = "==================PROBE DOCS==================\n"
        header += doc + "\n"
        header += "".join(["===\n" + child.doc() + "\n" for child in self._brood])
        header += "==============================================\n"
        return header
    
    def _start(self):
        sim.print_message(self._doc())
        sim.print_message("SIMULATION BEGIN\n")

    def _end(self):
        sim.print_message("==============================================\n")
        pass
    
    def _spawn(self):
        for child in self._brood:
            sim_setup.create_sim_python_thread(child.fpath(), child.args())

    def main(self):
        sim_setup.add_begin_of_simulation_callback(self._start)
        sim_setup.add_end_of_simulation_callback(self._end)
        self._spawn()

if __name__ == "__main__":
    #no args
    probe = MotherProbe(brood=[
        CrashdumpProbes(),
    ])
    probe.main()
