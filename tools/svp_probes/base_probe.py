"""
base_probe.py
"""

import sim

base_doc = """
base_probe

Used as a common probe interface to be implemented and consumed by the mother probe
"""

class BaseProbe:
    def __init__(self, args = []):
        self._args = args

    @staticmethod
    def fpath() -> str:
        raise NotImplementedError("must implement")

    @staticmethod 
    def doc() -> str:
        raise NotImplementedError("must implement")
    
    def args(self):
        return self._args

if __name__ == "__main__":
    sim.print_message("base_probe")