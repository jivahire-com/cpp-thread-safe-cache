"""
PLDM Specification Parser

Copyright (C) Microsoft Corporation. All rights reserved.
Confidential and Proprietary.
"""

import json
import os


class pldm_spec_parser:
    def __init__(self):
        self.pldm_types = []
        self.pdr_types = []
        self.error_codes = {}
        self.offsets = {}
        self.output_headers = {}
        self.output_header_comments = {}
        self.raw_commands = {}
        self.sensors_effecters_limit = {}

    def load_spec_data(self, json_filename):
        """
        Load PLDM specification data from a JSON file.
        """

        current_dir = os.path.dirname(os.path.abspath(__file__))
        json_path = os.path.join(current_dir, json_filename)

        with open(json_path, "r", encoding="utf-8") as json_file:
            spec_data = json.load(json_file)

        self.pldm_types = spec_data.get("pldm_types", [])
        self.pdr_types = spec_data.get("pdr_types", [])
        self.error_codes = spec_data.get("error_codes", {})
        self.offsets = spec_data.get("offsets", {})
        self.output_headers = spec_data.get("output_header", {})
        self.raw_commands = spec_data.get("raw_commands", {})
        self.sensors_effecters_limit = spec_data.get("sensors_effecters_limit", {})

    def get_pldm_type_info(self, type_id):
        """
        Retrieve PLDM type information by ID.
        """
        return self.pldm_types.get(type_id)

    def get_all_pldm_types(self):
        """
        Return all PLDM types and their details.
        """
        return self.pldm_types

    def get_pdr_types(self):
        """
        Return the list of supported PDR types.
        """
        return self.pdr_types

    def get_error_code(self, description_key):
        """
        Return the error code for a given error description key.
        """
        return self.error_codes.get(description_key)

    def get_offset(self, offset_name):
        """
        Return the offset value for a given offset name.
        """
        return self.offsets.get(offset_name)

    def get_output_header(self, command_name):
        """
        Return the expected output header for a given command.
        """
        return self.output_headers.get(command_name)

    def get_raw_command(self, command_name):
        """
        Return the raw command for a given command name.
        """
        return self.raw_commands.get(command_name)

    def get_numeric_sensor_limit(self):
        """
        Return the list of numeric sensor IDs.
        """
        return self.sensors_effecters_limit.get("numeric_sensor_limit")

    def get_numeric_effecter_limit(self):
        """
        Return the list of numeric effecter IDs.
        """
        return self.sensors_effecters_limit.get("numeric_effecter_limit")

    def get_state_sensor_limit(self):
        """
        Return the list of state sensor IDs.
        """
        return self.sensors_effecters_limit.get("state_sensor_limit")

    def get_state_effecter_limit(self):
        """
        Return the list of state effecter IDs.
        """
        return self.sensors_effecters_limit.get("state_effecter_limit")
