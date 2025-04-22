# Copyright (c) Microsoft Corporation. All rights reserved.


import ctypes
import json
import os
import argparse
import sys
from pathlib import Path

import sensor_fifo_structs as sf
import tlm_structs as ts

sys.path.append(str(Path(__file__).parent.parent / 'diag_decoder'))

import diag_decoder_structs as ds

def parse_binary_file(input_file):
    """
    Parse the binary file and return a list of dictionaries representing the data.
    """
    if not os.path.exists(input_file):
        raise FileNotFoundError(f"File not found: {input_file}")

    parsed_data = []

    with open(input_file, "rb") as f:
        # Decode a single instance of telemetry_package_hdr
        header_size = ctypes.sizeof(ts.telemetry_package_hdr)
        header_data = f.read(header_size)
        if len(header_data) < header_size:
            raise ValueError("Incomplete telemetry_package_hdr data encountered.")

        # Parse the header
        package_header = ts.telemetry_package_hdr.from_buffer_copy(header_data)
        # Add the parsed header to the result
        parsed_package = {
            "decoder_header": {
                "payload_parser_version": package_header.decoder_header.payload_parser_version,
                "payload_parser_type": package_header.decoder_header.payload_parser_type,
            },
            "payload_header": {
                "manifest_id": {
                    "data1": package_header.payload_header.manifest_id.data1,
                    "data2": package_header.payload_header.manifest_id.data2,
                    "data3": package_header.payload_header.manifest_id.data3,
                    "data4": list(package_header.payload_header.manifest_id.data4),
                },
                "timestamp_us": package_header.payload_header.timestamp_us,
                "timestamp_utc": package_header.payload_header.timestamp_utc,
                "source_die": package_header.payload_header.source_die,
                "package_number": package_header.payload_header.package_number,
                "number_of_records": package_header.payload_header.number_of_records,
                "package_payload_size": package_header.payload_header.package_payload_size,
            },
            "records": []
        }
        # Loop through the number of records
        num_records = package_header.payload_header.number_of_records
        record_header_size = ctypes.sizeof(ts.telemetry_record_hdr)

        for _ in range(num_records):
            # Read and parse the record header
            record_header_data = f.read(record_header_size)
            if len(record_header_data) < record_header_size:
                raise ValueError("Incomplete telemetry_record_hdr data encountered.")

            record_header = ts.telemetry_record_hdr.from_buffer_copy(record_header_data)

            # Add the record header to the parsed package
            parsed_record = {
                "timestamp_us": record_header.timestamp_us,
                "record_number": record_header.record_number,
                "number_of_collections": record_header.number_of_collections,
                "record_payload_size": record_header.record_payload_size,
            }

            # Pass the remaining data to the record_parser function
            record_payload_data = f.read(record_header.record_payload_size)
            parsed_record["collections"] = record_parser(record_payload_data, record_header.number_of_collections)

            # Append the parsed record to the package
            parsed_package["records"].append(parsed_record)

        # Add the parsed package to the result
        parsed_data.append(parsed_package)

    return parsed_data

def record_parser(record_payload_data, number_of_collections):
    """
    Parse the record payload data and return a list of dictionaries representing the collections.
    """
    parsed_collections = []
    offset = 0  # Track the current position in the payload data

    for _ in range(number_of_collections):
        # Decode a single instance of telemetry_collection_hdr
        collection_hdr_size = ctypes.sizeof(ts.telemetry_collection_hdr)
        collection_hdr_data = record_payload_data[offset:offset + collection_hdr_size]
        if len(collection_hdr_data) < collection_hdr_size:
            raise ValueError("Incomplete telemetry_collection_hdr data encountered.")

        # Parse the collection header
        collection_header = ts.telemetry_collection_hdr.from_buffer_copy(collection_hdr_data)

        # Add the collection header to the parsed collections
        parsed_collection = {
            "provider_id": collection_header.provider_id,
            "element_id": collection_header.element_id,
            "collection_id": collection_header.collection_id,
            "number_of_elements": collection_header.number_of_elements,
            "collection_payload_size": collection_header.collection_payload_size,
        }

        if collection_header.provider_id != 0xFFFF or collection_header.element_id != 0xFFFF:
            raise ValueError(f"Not a sensor fifo debug mode collection: provider_id = 0x{collection_header.provider_id:x} element_id = 0x{collection_header.element_id:x}")

        # Advance the offset to the collection payload
        offset += collection_hdr_size

        # Call collection_parser to parse the collection payload
        collection_payload_data = record_payload_data[offset:offset + collection_header.collection_payload_size]
        parsed_collection["elements"] = collection_parser(collection_payload_data, collection_header.collection_id, collection_header.number_of_elements)

        # Advance the offset to the next collection header
        offset += collection_header.collection_payload_size

        # Append the parsed collection to the list
        parsed_collections.append(parsed_collection)

    return parsed_collections

def collection_parser(collection_payload_data, collection_id, number_of_elements):
    """
    Parse the collection payload data based on collection_id and return a list of elements.
    """
    parsed_elements = []
    offset = 0

    # Map sensor_fifo_id to corresponding ctypes structures
    element_struct_map = {
        sf.sensor_fifo_id.sensor_fifo_tile_temperature_telemetry_hw: sf.tile_temp,
        sf.sensor_fifo_id.sensor_fifo_tile_voltage_telemetry_hw: sf.tile_voltage,
        sf.sensor_fifo_id.sensor_fifo_core_current_telemetry_hw: sf.core_current,
        sf.sensor_fifo_id.sensor_fifo_pvt_temp_fw: sf.soc_pvt_temp,
        sf.sensor_fifo_id.sensor_fifo_pvt_voltage_fw: sf.soc_pvt_voltage,
        sf.sensor_fifo_id.sensor_fifo_dimm_temp_fw: sf.sensor_ram_dimm_info,
        sf.sensor_fifo_id.sensor_fifo_vr_temp_fw: sf.vr_temp,
        sf.sensor_fifo_id.sensor_fifo_vr_current_fw: sf.vr_current,
        sf.sensor_fifo_id.sensor_fifo_pstate_telemetry_hw: sf.pstate_telem,
    }

    # Get the structure corresponding to the collection_id
    element_struct = element_struct_map.get(collection_id)
    if element_struct is None:
        raise ValueError(f"Unsupported collection_id: {collection_id}")

    # Calculate the size of the structure
    element_size = ctypes.sizeof(element_struct)

    # Parse the collection payload data as an array of the selected structure
    for _ in range(number_of_elements):
        # Extract the element data
        element_data = collection_payload_data[offset:offset + element_size]
        if len(element_data) < element_size:
            raise ValueError("Incomplete element data encountered.")

        # Parse the element
        element = element_struct.from_buffer_copy(element_data)

        # Convert the element to a dictionary for JSON serialization
        parsed_element = ctypes_to_dict(element)

        # Append the parsed element to the list
        parsed_elements.append(parsed_element)

        # Advance the offset to the next element
        offset += element_size

    return parsed_elements

def ctypes_to_dict(struct):
    """
    Convert a ctypes.Structure or ctypes.Union to a dictionary.
    """
    result = {}
    for field in struct._fields_:
        field_name = field[0]  # The first element is the field name
        value = getattr(struct, field_name)
        if isinstance(value, ctypes.Structure) or isinstance(value, ctypes.Union):
            result[field_name] = ctypes_to_dict(value)  # Recursively convert nested structures
        elif isinstance(value, ctypes.Array):
            result[field_name] = list(value)  # Convert arrays to lists
        else:
            result[field_name] = value
    return result

def write_json_file(output_file, data):
    """
    Write the parsed data to a JSON file, formatting integers as hexadecimal where applicable.
    """
    # Convert integers to hexadecimal strings where needed
    def format_hex(obj):
        if isinstance(obj, int):
            return hex(obj)  # Convert integers to hex strings
        if isinstance(obj, dict):
            return {k: format_hex(v) for k, v in obj.items()}
        if isinstance(obj, list):
            return [format_hex(i) for i in obj]
        return obj

    formatted_data = format_hex(data)

    with open(output_file, "w") as f:
        json.dump(formatted_data, f, indent=4)

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Parse a binary file and output JSON data.")
    parser.add_argument("input_file", help="Path to the input binary file")
    parser.add_argument("output_file", help="Path to the output JSON file")

    # Parse arguments
    args = parser.parse_args()

    # Validate arguments
    if not os.path.isfile(args.input_file):
        print(f"Error: Input file '{args.input_file}' does not exist.")
        return

    try:
        # Parse the binary file
        parsed_data = parse_binary_file(args.input_file)

        # Write the parsed data to a JSON file
        write_json_file(args.output_file, parsed_data)

        print(f"Parsed data written to {args.output_file}")
    except (FileNotFoundError, ValueError) as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()