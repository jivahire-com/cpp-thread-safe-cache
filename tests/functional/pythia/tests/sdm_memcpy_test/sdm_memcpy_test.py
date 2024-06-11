"""
- Python based Pythia 2.0 Test.
- Tests that checks for sdm memcpy completes from AP core0.
"""

from pathlib import Path
from ..test_lib.base_mscp_test import BaseMSCPTest

# Class name must match file name for Robot Framework Library usage
class sdm_memcpy_test(BaseMSCPTest):

    """
    :param name: 
        Name of the test case
    :param workspace_config:
        Path to the workspace config file
    :param default_log_home:
        Path for log storage
    :param fw_payload_path:
        Path to the recipe payload
    :param host_config:
        Path to the host config file/directory
    """
    def __init__(
        self,
        name: str = "sdm_memcpy_test",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
    ):

        # Call parent class init
        super().__init__(
            name=name,
            workspace_config=workspace_config,
            default_log_home=default_log_home,
            fw_payload_path=fw_payload_path,
            host_config=host_config,
        )

    def test_sdm_memcpy(self):
        """
        Test function:
            1. Setup the Test.
            2. Assert that a connection to AP core on DIE 0 can be
                established and open it.
            3. Look for the SDM memcpy completion string.
            4. Teardown Test.
        """
        self.dut.setup()

        # Ensure the host config file used alongside this test has these connections defined.

        assert self.dut.mb.soc_0.apns is not None

        self.dut.mb.soc_0.apns.open()
        assert self.dut.mb.soc_0.apns.is_open()

        apns_lines = ' '.join(self.read_and_log_lines(connection=self.dut.mb.soc_0.apns, num_lines=15))

        test_pass = True if ('compare_data_buffer: INFO: SUCCESS' in apns_lines) else False

        self.dut.mb.soc_0.apns.close()

        # Notify of the test results. If test_pass is False it is an error (which also signals to the base test class that the test failed).
        self.test_notify(step="sdm_memcpy", msg="Success", _is_error=not(test_pass))
        self.dut.teardown()

        return test_pass