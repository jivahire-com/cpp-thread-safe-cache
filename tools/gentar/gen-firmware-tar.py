# Copyright (c) Microsoft Corporation. All rights reserved.

#!/usr/bin/env python3
import argparse
import os
import tempfile
import tarfile
import shutil
import json
import hashlib
import subprocess
import sys
import platform
from pathlib import Path

HELP_TEXT = """
Generate the OBMC compatible Tarball for component FW images

Usage: python gen-firmware-tar.py [OPTIONS]

Options:
   -c, --component <name>         Specify the component name for the package
   -m, --machine <name>           Specify the target machine name of this image
   -v, --version <name>           Specify the version of image file
   -p, --package <name>           Specify the name of the component package feed to download (optional if using local source)
   -i, --input  <name>            Specify an input folder path to use instead of artifact feed (optional if using artifact feeds)
   -o, --output <name>            Specify an output folder path for the generated package (optional if needed to override the default output folder)
"""

def parse_arguments():
    parser = argparse.ArgumentParser(description=HELP_TEXT, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-c', '--component', required=True, help='Specify the component name')
    parser.add_argument('-m', '--machine', required=True, help='Specify the target machine name')
    parser.add_argument('-v', '--version', required=True, help='Specify the version of component package to download')
    parser.add_argument('-p', '--package', default="None", help='Specify the name of the component package feed to download (optional if using local folder)')
    parser.add_argument('-i', '--input', default="None", help='Specify an input folder path to use instead of artifact feed (optional if using artifact feeds)')
    parser.add_argument('-o', '--output', help='Specify an output folder path for the generated package (optional if needed to override the default output folder)')
    parser.add_argument('--token', action="store", default=os.environ.get('USERPAT', None))
    return parser.parse_args()

def find_azure_cli():
    """
    Find the Azure CLI executable on the system.
    Returns the path to the executable or raises an exception if not found.
    """
    # Try common Azure CLI executable names
    az_commands = ['az', 'az.cmd', 'az.exe']

    for az_cmd in az_commands:
        try:
            # Try to run az --version to check if it's available
            result = subprocess.run([az_cmd, '--version'],
                                  capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                return az_cmd
        except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
            continue

    # If not found in PATH, try common installation locations on Windows
    if os.name == 'nt':  # Windows
        common_paths = [
            r"C:\Program Files (x86)\Microsoft SDKs\Azure\CLI2\wbin\az.cmd",
            r"C:\Program Files\Microsoft SDKs\Azure\CLI2\wbin\az.cmd",
            os.path.expanduser(r"~\AppData\Local\Programs\Microsoft VS Code\bin\az.cmd"),
            os.path.expanduser(r"~\scoop\shims\az.cmd"),
            os.path.expanduser(r"~\scoop\shims\az.exe"),
        ]

        for path in common_paths:
            if os.path.isfile(path):
                try:
                    result = subprocess.run([path, '--version'],
                                          capture_output=True, text=True, timeout=10)
                    if result.returncode == 0:
                        return path
                except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
                    continue

    # If still not found, raise an error
    raise FileNotFoundError(
        "Azure CLI not found. Please install Azure CLI and ensure it's in your PATH. "
        "You can download it from: https://docs.microsoft.com/en-us/cli/azure/install-azure-cli"
    )

def find_openssl_executable():
    """
    Find the OpenSSL executable on the system.
    Returns the path to the executable or raises an exception if not found.
    """
    # Try common OpenSSL executable names
    openssl_commands = ['openssl', 'openssl.exe']

    for openssl_cmd in openssl_commands:
        try:
            # Try to run openssl version to check if it's available
            result = subprocess.run([openssl_cmd, 'version'],
                                  capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                return openssl_cmd
        except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
            continue

    # If not found in PATH, try common installation locations on Windows
    if platform.system() == 'Windows':
        common_paths = [
            r"C:\Program Files\OpenSSL-Win64\bin\openssl.exe",
            r"C:\Program Files (x86)\OpenSSL-Win32\bin\openssl.exe",
            r"C:\OpenSSL-Win64\bin\openssl.exe",
            r"C:\OpenSSL-Win32\bin\openssl.exe",
            r"C:\tools\openssl\openssl.exe",
            # Chocolatey installation paths
            r"C:\ProgramData\chocolatey\lib\openssl\tools\openssl.exe",
            r"C:\ProgramData\chocolatey\bin\openssl.exe",
            # Scoop installation paths
            os.path.expanduser(r"~\scoop\apps\openssl\current\bin\openssl.exe"),
            os.path.expanduser(r"~\scoop\shims\openssl.exe"),
            # Git for Windows includes OpenSSL
            r"C:\Program Files\Git\usr\bin\openssl.exe",
            r"C:\Program Files (x86)\Git\usr\bin\openssl.exe",
            # Windows Subsystem for Linux paths
            r"C:\Windows\System32\bash.exe",  # We'll use this to run openssl in WSL if available
        ]

        for path in common_paths:
            if os.path.isfile(path):
                if path.endswith('bash.exe'):
                    # Try WSL openssl
                    try:
                        result = subprocess.run([path, '-c', 'openssl version'],
                                              capture_output=True, text=True, timeout=10)
                        if result.returncode == 0:
                            return 'wsl openssl'  # Special marker for WSL
                    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
                        continue
                else:
                    try:
                        result = subprocess.run([path, 'version'],
                                              capture_output=True, text=True, timeout=10)
                        if result.returncode == 0:
                            return path
                    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
                        continue

    # If still not found, raise an error
    raise FileNotFoundError(
        "OpenSSL not found. Please install OpenSSL and ensure it's in your PATH. "
        "On Windows, you can download it from: https://slproweb.com/products/Win32OpenSSL.html "
        "or install it via Chocolatey: choco install openssl"
    )

def download_universal_package(organization, project, feed, package_name, package_version, output_directory):
    """
    Downloads a universal package from Azure Artifacts.

    :param organization: Azure DevOps organization URL (e.g., https://dev.azure.com/{organization})
    :param project: Name of the project associated with this feed (project scoped).
    :param feed: Name of the feed containing the package.
    :param package_name: Name of the package to download.
    :param package_version: Version of the package to download.
    :param output_directory: Directory to save the downloaded package.
    """
    az_cmd = find_azure_cli()

    if len(project) != 0:
        print(f"Downloading (project scoped) {package_name}-{package_version} from {organization}/{project}/{feed}")
        # Construct the Azure CLI command
        command = [
            az_cmd, "artifacts", "universal", "download",
            "--project", project,
            "--scope", "project",
            "--organization", organization,
            "--feed", feed,
            "--name", package_name,
            "--version", package_version,
            "--path", output_directory
        ]
    else:
        print(f"Downloading (organizational scoped) {package_name}-{package_version} from {organization}{feed}")
        # Construct the Azure CLI command
        command = [
            az_cmd, "artifacts", "universal", "download",
            "--organization", organization,
            "--feed", feed,
            "--name", package_name,
            "--version", package_version,
            "--path", output_directory
        ]
    try:
        # Execute the command
        print(f"Running az cli command: {command}")
        result = subprocess.run(command, check=True, capture_output=True, text=True, env=os.environ)
        print("Download successful!")
        print("Output:", result.stdout)

    except subprocess.CalledProcessError as e:
        print("Error occurred while downloading the package.")
        print("Error Output:", e.stderr)

def publish_universal_package(organization, project, feed, package_name, package_version, output_directory):
    """
    Publishes a universal package to Azure Artifacts.

    :param organization: Azure DevOps organization URL (e.g., https://dev.azure.com/{organization})
    :param project: Name of the project associated with this feed (project scoped).
    :param feed: Name of the feed to publish the package to.
    :param package_name: Name of the package to publish.
    :param package_version: Version of the package to publish.
    :param output_directory: Directory containing the package files to publish.
    """
    # Find Azure CLI executable
    try:
        az_executable = find_azure_cli()
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return

    if len(project) != 0:
        print(f"Publishing (project scoped) {package_name}-{package_version} to {organization}/{project}/{feed}")
        # Construct the Azure CLI command
        command = [
            az_executable, "artifacts", "universal", "publish",
            "--project", project,
            "--scope", "project",
            "--organization", organization,
            "--feed", feed,
            "--name", package_name,
            "--version", package_version,
            "--path", output_directory
        ]
    else:
        print(f"Publishing (organizational scoped) {package_name}-{package_version} to {organization}{feed}")
        # Construct the Azure CLI command
        command = [
            az_executable, "artifacts", "universal", "publish",
            "--organization", organization,
            "--feed", feed,
            "--name", package_name,
            "--version", package_version,
            "--path", output_directory
        ]
    try:
        # Execute the command
        print(f"Running az cli command: {command}")
        result = subprocess.run(command, check=True, capture_output=True, text=True, env=os.environ)
        print("Publish successful!")
        print("Output:", result.stdout)

    except subprocess.CalledProcessError as e:
        print("Error occurred while publishing the package.")
        print("Error Output:", e.stderr)

def get_component_config(component):
    """
    Get the component config which includes feed details and component details
    """
    config_file = "package_feed_config.json"
    with open(config_file, "r") as f:
        feed_config = json.load(f)

    if component not in feed_config:
        print(f"Component {component} not found in the package feed config")
        exit(1)

    return feed_config[component]

class PackageGenerator:
    def __init__(self, input_directory, output_directory, dev_key_path):
        self.input_directory = input_directory
        self.output_directory = output_directory
        self.dev_key_path = dev_key_path
        self.manifest_file_path = os.path.join(self.output_directory, "MANIFEST.json")
        self.manifest_file_sig_path = os.path.join(self.output_directory, "MANIFEST.json.sig")

    def __del__(self):
        if os.path.exists(self.manifest_file_sig_path):
            os.remove(self.manifest_file_sig_path)

    def list_files(self):
        try:
            source_files = []
            input_files = os.path.join( self.input_directory, "input_files.json")
            if os.path.exists(input_files):
                with open(input_files, "r") as f:
                    data = json.load(f)
                files = data["Files"]
                for entry in files:
                    file_path = os.path.join(self.input_directory, entry["File"])
                    if os.path.exists(file_path):
                        print(f"Found file: {file_path}")
                        source_files.append(file_path)
                    else:
                        print(f"File not found: {file_path}")
                return source_files
            else:
                print(f"Could not find input_files.json at path: {input_files}. Defaulting to all files in {self.input_directory}")
                return [os.path.join(self.input_directory, f) for f in os.listdir(self.input_directory) if os.path.isfile(os.path.join(self.input_directory, f))]
        except json.JSONDecodeError as e:
            print(f"JSONDecodeError: Failed to decode JSON from {input_files}. Error: {e}")
            return []
        except KeyError as e:
            print(f"KeyError: The Key: {e} was not found in {input_files}.")
            return []

    def get_file_metadata(self, file):
        try:
            input_files = os.path.join( self.input_directory, "input_files.json")
            if os.path.exists(input_files):
                with open(input_files, "r") as f:
                    data = json.load(f)
                files = data["Files"]
                for entry in files:
                    #If the File in input_files.json contains relative path, get the basename for comparison
                    if os.path.basename(entry["File"]) == file:
                        if "Metadata" in entry:
                            return entry["Metadata"]
        except json.JSONDecodeError:
            print(f"ERROR: Failed to decode JSON from {input_files}.")
            return None
        return None

    def get_file_sha512_hash(self, file):
        sha512 = hashlib.sha512()
        with open(file, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                sha512.update(chunk)
        return sha512.hexdigest()

    def generate_manifest(self, file_paths, compatiblename, component, machine, version):
        print(f"Total files in package: {len(file_paths)}")
        manifest = {}
        manifest["version"] = version
        manifest["CompatibleName"] = compatiblename
        manifest["MachineName"] = machine
        manifest["Files"] = []
        for file in file_paths:
            print(f"File: {file}")
            file_entry = {}
            file_entry["FileName"] = os.path.basename(file)
            file_entry["Hash"] = self.get_file_sha512_hash(file)
            metadata = self.get_file_metadata(file_entry["FileName"])
            if metadata:
                file_entry.update(metadata)
            else:
                print(f"No metadata found for file: {file}")
            manifest["Files"].append(file_entry)
        # Convert the manifest to a JSON string with specific formatting
        json_string = json.dumps(manifest, indent=4)
        # Ensure all line endings are '\n' (LF) instead of '\r\n' (CRLF)
        json_string = json_string.replace('\r\n', '\n')
        # Add a final newline character to ensure the file ends with 0x0A
        json_string += '\n'

        # Write the formatted JSON string to the file
        with open(self.manifest_file_path, 'w', newline='\n') as f:
            f.write(json_string)
        print("=== Manifest [START]: ===")
        print(json_string)
        print("==== Manifest [END]: ====")

    def sign_manifest(self):
        if not os.path.exists(self.manifest_file_path):
            print(f"ERROR: Manifest file not found at: {self.manifest_file_path}. Unable to sign")
            exit(1)
        else:
            print(f"Signing Manifest file: {self.manifest_file_path}")
            print(f"Dev Key: {self.dev_key_path}")

            try:
                # Find OpenSSL executable
                openssl_exe = find_openssl_executable()
                print(f"Found OpenSSL: {openssl_exe}")

                # Construct command based on the type of OpenSSL found
                if openssl_exe == 'wsl openssl':
                    # Use WSL to run openssl
                    command = [
                        'wsl', 'openssl', 'dgst', '-sha512',
                        '-sign', self.dev_key_path.replace('\\', '/').replace('C:', '/mnt/c'),
                        '-out', self.manifest_file_sig_path.replace('\\', '/').replace('C:', '/mnt/c'),
                        self.manifest_file_path.replace('\\', '/').replace('C:', '/mnt/c')
                    ]
                else:
                    # Use regular openssl command
                    command = [
                        openssl_exe, "dgst", "-sha512",
                        "-sign", self.dev_key_path,
                        "-out", self.manifest_file_sig_path,
                        self.manifest_file_path,
                    ]

                # Execute the command
                print(f"Running openssl command: {command}")
                result = subprocess.run(command, check=True, capture_output=True, text=True, env=os.environ)
                print("Signing successful!")

            except FileNotFoundError as e:
                print(f"ERROR: {e}")
                print("Please install OpenSSL and ensure it's accessible.")
                exit(1)
            except subprocess.CalledProcessError as e:
                print("Error signing manifest.")
                print("Error Output:", e.stderr)
                exit(1)

            print(f"MANIFEST signature written to {self.manifest_file_sig_path}")

    def generate_tarball(self, file_paths, output_file_name):
        output_file = os.path.join(self.output_directory, output_file_name)
        #Remove existing tarball
        if os.path.isfile(output_file):
            os.remove(output_file)
        with tarfile.open(output_file, "w:gz", dereference=True) as tar:
            for file_path in file_paths:
                tar.add(file_path, arcname=os.path.basename(file_path), recursive=False)
            tar.add(self.manifest_file_path, arcname=os.path.basename(self.manifest_file_path))
            tar.add(self.manifest_file_sig_path, arcname=os.path.basename(self.manifest_file_sig_path))
            contents = tar.getnames()
        print("Contents of the tarball:")
        for item in contents:
            print(item)
        print(f"Tarball created at {output_file}")

    def generate_package(self, compatiblename, component, machine, version, output_package_name):
        file_paths = self.list_files()
        if len(file_paths) == 0:
            print(f"Could not find any files to package at the path: {self.input_directory}")
        else:
            self.generate_manifest(file_paths, compatiblename, component, machine, version)
            self.sign_manifest()
            self.generate_tarball(file_paths, output_package_name)

class BMCPackageGenerator(PackageGenerator):
    # Override package generation generation method from baseclass for BMC to generate cs0 and cs0+cs1 package
    def generate_package(self, compatiblename, component, machine, version, output_package_name):
        file_paths = self.list_files()
        if len(file_paths) == 0:
            print(f"ERROR: Invalid files for BMC component at the path: {self.input_directory}")
            exit(1)

        package_extension = ".tar.gz"
        for file in file_paths:
            if "full" in file:
                print(f"Detected full image: {file}")
                package_extension = ".full.tar.gz"

        # Get output file name from -output parameter if provided, otherwise use default naming
        if hasattr(self, 'output_file_name_from_param') and self.output_file_name_from_param:
            output_file_name = self.output_file_name_from_param
        else:
            output_file_name = f"{machine.upper()}.OBMC.{version}"
        output_package_name = output_file_name + package_extension
        cs0_files = []
        for file in file_paths:
            cs0_files.append(file)

        #CS0 only
        print(f"\n ============ Generating Single Partition Package ============ \n")
        self.generate_manifest(cs0_files, compatiblename, component, machine, version)
        self.sign_manifest()
        self.generate_tarball(cs0_files, output_package_name)

        #CS0 and #CS1
        print(f"\n ============ Generating Dual Partition Package ============ \n")
        output_package_name = output_file_name + ".cs0.cs1" + package_extension
        cs1_files = []
        for file in file_paths:
            cs1_file_name = file + "-1"
            shutil.copy(file, cs1_file_name)
            cs1_files.append(cs1_file_name)
        files = cs0_files + cs1_files
        self.generate_manifest(files, compatiblename, component, machine, version)
        self.sign_manifest()
        self.generate_tarball(files, output_package_name)

        #Clean up files
        for file in cs1_files:
            os.remove(file)

def get_feed_details(config, section, defaults):
    feed_section = config.get(section, {})
    return {
        'organization': feed_section.get('organization', defaults.get('organization', "")),
        'project': feed_section.get('project', defaults.get('project', "")),
        'feed': feed_section.get('feed', defaults.get('feed', ""))
    }

def main():
    args = parse_arguments()
    print(f"Input Arguments:")
    print(f"Component: {args.component}")
    print(f"Machine: {args.machine}")
    print(f"Version: {args.version}")
    print(f"Package: {args.package}")
    print(f"Input source path (local signing): {args.input}")
    print(f"Output source path (local signing): {args.output}")

    # Download the OpenBMC dev key
    devkey_download_path = os.path.join(os.getcwd(), "devkey_feed_download")
    download_universal_package("https://azurecsi.visualstudio.com/", "Woodinville","EchoFalls.Dependency", "openbmc-dev-key", "*", devkey_download_path)
    if not os.path.exists(devkey_download_path):
        print(f"Failed to download openbmc-dev-key package")
        exit(1)
    else:
        print(f"Downloaded openbmc-dev-key package to {devkey_download_path}")

    dev_key_path = os.path.join(devkey_download_path, os.listdir(devkey_download_path)[0])
    config = get_component_config(args.component)
    compatible_name = config['CompatibleName']
    machine = args.machine
    local_signing = False
    if args.input != "None":
        local_signing = True
        # Use local input path as source files for packaging
        if not os.path.exists(args.input):
            print(f"Invalid local source path: {args.input}")
            exit(1)
        else:
            source_files_path = args.input
    elif "FeedConfig" in config:
        # Download the component universal package as source files for packaging
        feed_config = config['FeedConfig']
        if machine.upper() not in feed_config:
            print(f"Platform specific {machine.upper()} config not found in feed config for component {args.component}. Set to default machine m1120-wcs")
            machine = "m1120-wcs"
        else:
            print(f"Platform specific config: {machine.upper()} selected for component: {args.component}")

        default_feed = feed_config['Default']
        config_feed = feed_config.get(machine.upper(), {})
        if not config_feed:
            # If no specific feed config is found for the machine, use the default feed
            print(f"Platform specific feed config not found for {machine.upper()}. Using default feed config")
            input_feed_config = default_feed['Input']
            output_feed_config = default_feed['Output']
        else:
            input_feed_config = get_feed_details(config_feed, 'Input', default_feed['Input'])
            output_feed_config = get_feed_details(config_feed, 'Output', default_feed['Output'])

        #Set input and output feed details
        input_feed_org = input_feed_config['organization']
        input_feed_prj = input_feed_config['project']
        input_feed = input_feed_config['feed']
        output_feed_org = output_feed_config['organization']
        output_feed_prj = output_feed_config['project']
        output_feed = output_feed_config['feed']

        component_download_path = os.path.join(os.getcwd(), "component_feed_download")
        if os.path.exists(component_download_path):
            shutil.rmtree(component_download_path)

        #Download the artifact feed
        download_universal_package(input_feed_org, input_feed_prj, input_feed, args.package, args.version, component_download_path)
        if not os.path.exists(component_download_path):
            print(f"Failed to download the package from {input_feed_org}-{input_feed}-{args.package}-{args.version}")
            exit(1)
        else:
            print(f"Downloaded {args.component} package to {component_download_path}")
        source_files_path = component_download_path
    else:
        print(f"Ensure the feed config is correct for the component: {args.component} or specify a local folder path for signing")
        exit(1)

    # Prep output directory and extract output file name if provided
    output_file_name_from_param = None
    if args.output is None:
        output_directory = os.path.join(os.getcwd(), "fw_package_gen_output")
    else:
        # Check if args.output is a file path or just a directory
        if os.path.splitext(args.output)[1]:  # Has file extension, treat as file path
            output_directory = os.path.dirname(args.output)
            basename = os.path.basename(args.output)
            # Check for .tar.gz extension
            if basename.endswith('.tar.gz'):
                output_file_name_from_param = basename[:-7]  # Remove .tar.gz
            else:
                # For other extensions, use standard splitext
                output_file_name_from_param = os.path.splitext(basename)[0]

        else:
            output_directory = args.output

    # Create output directory only if it does not exist
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    # Create the tarball name, using output file name if provided
    if output_file_name_from_param:
        tarball_name = f"{output_file_name_from_param}.tar.gz"
    else:
        tarball_name = f"{machine.upper()}-{args.component}-{args.version}.tar.gz"

    if args.component == "BMC":
        package_gen = BMCPackageGenerator(source_files_path, output_directory, dev_key_path)
        if output_file_name_from_param:
            package_gen.output_file_name_from_param = output_file_name_from_param
        package_gen.generate_package(compatible_name, args.component, machine, args.version, tarball_name)
    else:
        package_gen = PackageGenerator(source_files_path, output_directory, dev_key_path)
        if output_file_name_from_param:
            package_gen.output_file_name_from_param = output_file_name_from_param
        package_gen.generate_package(compatible_name, args.component, machine, args.version, tarball_name)

    # TODO : Only enable this once OpenSSL Based signing is enabled as part of the python script itself.
    # #Upload the tarball to the artifact feed
    # if not local_signing:
    #     package_name = f"{machine.lower()}-{args.component.lower()}"
    #     publish_universal_package(output_feed_org, output_feed_prj, output_feed, package_name, args.version, output_directory)

if __name__ == "__main__":
    print(f"================== OBMC FW Package Generator v2.0 [Python: {sys.version}] ==================")
    main()
