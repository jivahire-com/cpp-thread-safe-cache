# Copyright (c) Microsoft Corporation. All rights reserved.
#
# thread_utils.py - Provides threading utilities for capturing serial output in test automation.
# Includes ReceiverThread for reading serial data until a key or timeout.
import threading

class ReceiverThread(threading.Thread):
    """
    Thread to capture output from a serial read method until a specific key or timeout.
    Sets self.log to the captured output and self.success to True if the key is found.
    """
    def __init__(self, read_method, key: str, timeout: int = 80):
        """
        Args:
            read_method: Callable to read serial output (must accept read_until_key and timeout_seconds).
            key (str): String to look for in the output to mark success.
            timeout (int): Timeout in seconds for the read operation.
        """
        super().__init__()
        self.read_method = read_method      # e.g., serial_util.read_scp_serial_until
        self.key = key                      # e.g., "MHU Recv Complete CB"
        self.timeout = timeout
        self.log = ""                       # Captured log text
        self.success = False                # Will be True if key found (or output captured)

    def run(self):
        """
        Run the thread: call the read_method and capture output until key or timeout.
        Sets self.log and self.success accordingly.
        """
        try:
            # Use the provided read_method to capture output until key or timeout
            result = self.read_method(read_until_key=self.key, timeout_seconds=self.timeout)
            if result:
                self.log = result
                # Mark success true if the expected key is in the output
                self.success = (self.key in result)
            else:
                self.log = ""  # No data captured (likely timeout)
                self.success = False
        except Exception as e:
            # Capture any exception as an error message
            self.log = f"Error in ReceiverThread: {e}"
            self.success = False
