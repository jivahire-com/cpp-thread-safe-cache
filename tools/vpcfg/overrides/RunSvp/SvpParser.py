"""Module provides the parser for Svp Vdk arguments. SvpParser is
a wrapper around argparse."""
import argparse
import os
import sys

try:
  __file__
except:
  import inspect
  __file__ = inspect.getfile(inspect.currentframe())

relative_path = "./PlatformHelp.py"
absolute_path = os.path.dirname(os.path.abspath(
                                os.path.join(os.path.dirname(__file__), relative_path)))

if os.path.exists(absolute_path):
    sys.path.append(absolute_path)
    from PlatformHelp import platform_help_message

class SvpParserNoRun(Exception):
    """Standard SvpVdk Exception"""
    pass

class SvpParser(argparse.ArgumentParser):
    """SvpParser class, provides the default Svp argument structure."""
    def __init__(self, file_name, print_errors=False):
        super().__init__(prog=file_name,
                         description="Silicon Virtual Platform",
                         exit_on_error=False)
        
        self.print_errors=print_errors
        self.add_argument("--template_name",
                                 type=str, help="The name of the fixed vdk to run")
        self.add_argument("--list_params",
                                 action='store_true',
                                 help="Specify if you wish to list available sim params and exit")
        self.add_argument("--parameter",
                                 action='append',
                                 help="Simulation parameter, can be specified"
                                 + " multiple times. Ex param=value")
        self.add_argument("--debug",
                                 action='store_true',
                                 help="Specify if you wish suspend simulation to connect to GDB")
        self.add_argument("--timeout",
                                 type=int,
                                 help="Sets the sim timeout in seconds")
        self.add_argument("--output_dir",
                                 type=str,
                                 help="Sets the simulation output"
                                 + " directory, default is working directory")
        self.add_argument("--central_vdk_dir",
                                 type=str,
                                 help="If running from a central vdk install,"
                                 + " set this path to where .vdkfxd is located")
        self.add_argument("--run_last_config",
                                 action='store_true',
                                 help="Will run the sim with the last parameters set")
        self.add_argument("--log_sim_output_to_file",
                                 action='store_true',
                                 help="Will log sim output to"
                                 + " simout.txt or what is specified by --sim_output_file")
        self.add_argument("--sim_output_file",
                                 type=str, help="Path to the simulation output file")
        self.add_argument("--max_instruction_count",
                                 type=int,
                                 help="Sets the max instruction count for any core in the sim")
        self.add_argument("--primary_core",
                                 type=str,
                                 help="Sets the primary core for tracking"
                                 + " max instruction count, use the cores full hierarchy")
        self.add_argument("--sim_probe",
                                 type=str,
                                 help="Specify a sim probe python script")
        self.add_argument("--env",
                                 action='append',
                                 help=str("Virtualizer environment variable to set during"
                                 + " simulation. Can be specified multiple times, ex TEST_VAL=0"))
        self.add_argument("--vpconfig",
                                 type=str,
                                 help="Specifies the name of the vpconfig"
                                 + " you want to run, creates it if it doesn't exist")
        self.add_argument("--nosave",
                                 action='store_true',
                                 help="Specify if you do not wish to save"
                                 + " the created vpconfig to file system for later use")
        self.add_argument("--norun",
                                 action='store_true',
                                 help="Specify if you only wish to create the vpconfig"
                                 + " and not run the simulation after")
        self.add_argument("--vpscript",
                                 type=str,
                                 help="Specifies the name of the vpscript."
                                 + " Optional and include to use a vpscript.")
        self.add_argument("--gui",
                                 action='store_true',
                                 help="Specify if you wish to launch the GUI."
                                 + " Note all other arguments will be ignored")
        
    def exit(self, status=0, message=None):
        if message and self.print_errors:
            self._print_message(message, sys.stderr)
        raise SvpParserNoRun(status)

    def error(self, message=None):
        """error(message: string)

        If you override this in a subclass, it should not return -- it
        should either exit or raise an exception.
        """
        self.exit(2, f"{self.prog}: {message}\n")

    def print_help(self, file=None):
        if file is None:
            file = sys.stdout
        self._print_message(self.format_help(), file)
        if callable(platform_help_message):
            self._print_message(f"\n{platform_help_message()}\n", file=file)

    def get_args(self, arg_source=None, namespace=None):
        """Gets the Svp arguments. Use sys.argv to pass arguments from the command
        line. Using argparse.Namespace and the namespace argument allows you 
        to pass arguments that did not orginate from the command line"""

        # try to parse the given args, if they aren't valid
        # we will print the help menu and set the norun
        # argument
        try:
            args = self.parse_args(arg_source, namespace)
        except SvpParserNoRun:
            self.print_help()
            args = self.parse_args(['--norun'], namespace)

        if args.output_dir is None:
            args.output_dir = os.getcwd()

        return args
