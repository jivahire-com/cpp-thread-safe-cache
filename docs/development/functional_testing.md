# Firmware Functional Tests

Functional testing is a type of software testing that ensures the application's overall functionality meets the business requirements. It involves testing the software's input and output, data manipulation, user interactions, and the system's response to various scenarios and conditions. Our functional tests are built on top of available Software Development Vehicles (SDV) and should be designed to be SDV agnostic where applicable.

## Table of Contents

[[_TOC_]]

## Terms

| Term | Description |
| - | - |
| Pythia 2.0 | [Testing framework developed by FSE Team](https://azurecsi.visualstudio.com/DuvallFw/_git/fse.pythia2.0) |
| Robot Framework | [Test Framework built on top of python](https://robotframework.org/) |
| SDV | Software Development Vehicle |

## High level goals

- Azure pipeline integration so that tests can be used to verify checkins and nightly builds
- Tests are easily runnable to reproduce errors found during pipeline execution
- Tests are scalable to run across different HW implementations (Emulator, FPGA, HW)

## Pythia Functional Tests

To stay aligned with as many teams within the organization as possible, use of Pythia 2.0 libraries is preferred. Pythia 2.0 comes as installable python modules.

### Workspace Configurations

These are JSON files that described the expected configuration of a Pythia workspace. You can find available configuration files [here](../../tests/functional/configs/).

### Host Configurations

These are JSON files that describe the Host Machine the test will run on, what type of connections it will have that can be used by a test, and any other configuration details needed that may impact running a test (ex: the path to the SVP executable needed to for Pythia to launch SVP). You can find available configuration files [here](../../tests/functional/hosts/).

### Recipe Configurations

Recipe JSON files describe all of the components that need to be downloaded, copied, etc.. for a test to run. It's used directly with Pythia's [Recipe Payload Downloader script](https://azurecsi.visualstudio.com/DuvallFw/_git/fse.pythia2.0?path=/tdk_rrm/pythia/tdk/rrm/payload_download/payload_download.py).

Fortunately for SVP based tests all of our dependencies get downloaded and setup when we configure this repo, which results in our SVP recipe relying on local pathing to dependencies (through environment variables).

You can find available configuration files [here](../../tests/functional/recipes/).

### Python Tests

Pythia provides an EchoFalls Test Development Kit (TDK) python module that we can leverage. Including the `EchoFallsBaseTest` class. By using this class we can leverage all of the work done there.

Example:

```python
class MyTestClass(EchoFallsBaseTest):

    """
    :param name: Name of the test case
    :param number: ADO Number of the test case
    :param workspace_config: Path to the workspace config file
    :param default_log_home: Path for log storage
    :param fw_payload_path: Path to the recipe payload
    :param dut_platform: Platform used during the test case execution
    :param host_config: Path to the host config file/directory
    :param host_name: Name of the host to find the host config file, if host_config is a directory. Defaults to None
    """
    def __init__(
        self,
        name: str = "TEST NAME HERE",
        number: str = "NaN or ADO Number",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = "KingsgateSVP OR Others",
        host_config: Path | str = None,
        host_name: str | None = None,
    ):
        # Call parent class init
        super().__init__(
            name,
            number,
            workspace_config,
            default_log_home,
            fw_payload_path,
            dut_platform,
            host_config,
            host_name,
        )

    def my_functional_test_function(self):

        # Call the DUT setup
        self.dut.setup()

        # 
        # TEST CONTENT HERE
        # 

        # Call the DUT teardown
        self.dut.teardown()
```

These python tests can be called directly by python as python script as well.

Example:

```python
if __name__ == "__main__":

    # Create an ArgumentParser object
    parser = argparse.ArgumentParser(description="Process some named parameters.")

    # Add arguments to the ArgumentParser object
    parser.add_argument("--workspace_config", type=str, help="Workspace config JSON.")
    parser.add_argument("--log_dir", type=str, help="Logs directory.")
    parser.add_argument("--payload_dir", type=str, help="Payload Directory.")
    parser.add_argument("--host_config", type=str, help="Host config JSON.")

    # Parse the command-line arguments
    args = parser.parse_args()

    test = MyTestClass(
        workspace_config=args.workspace_config,
        default_log_home=args.log_dir,
        fw_payload_path=args.payload_dir,
        host_config=args.host_config
    )
    assert test.my_functional_test_function()

```

#### Test(s) Location

All python based tests can be found [here](../../tests/functional/tests/).

### Robot Framework Tests

Robot Framework tests are built on top of python and operate as Test Suites with multiple Test Cases. Since it is built on top of Python, we can use any installed python module within a Test Suite. We can even directly call python based tests.

Example:

```robot
*** Settings ***
Documentation    Use the existing python test as a library and call it's test method

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ../../tests/example/ExampleTest.py
...             workspace_config=${WORKSPACE_CONFIG}
...             default_log_home=${LOG_DIR}
...             fw_payload_path=${PAYLOAD_DIR}
...             host_config=${HOST_CONFIG}
...             WITH NAME    ExampleTest

*** Test Cases ***
Test Case - Example Test
    # Get an instance of the test libray
    ${test_object}=    Get Library Instance    ExampleTest
    
    # Call the test method from the test library and store the result
    ${test_pass}=    Call Method    ${test_object}    test

    # Log the status
    Should be True    ${test_pass}
```

#### Test Suite(s) Location

All test suites can be found [here](../../tests/functional/test_suites/).

### Running Pythia Tests

We've wrapped the setup needed to run Pythia based tests in a PowerShell function, available in the module found [here](../../tools/pwsh/modules/Pythia-Utils.psm1).

The provided `Invoke-Pythia -test <path to .py or path to dir>` function can be used to execute a test. All test results will be stored in a newly created test directory under `<repo_enlistment>/.testlogs/pythia`. Check the test results with the appropriate timestamp (also logged when running the test).

You can run a single python file or an entire directory of Robot Framework Test Suites. For examples, refer to the commands in the gif below.

Example:

![img](../.img/invoke_pythia.gif)
