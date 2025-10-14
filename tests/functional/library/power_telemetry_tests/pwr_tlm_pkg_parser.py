"""
Copyright (c) Microsoft Corporation. All rights reserved.

Power Telemetry Package Parser

A generic parser for power telemetry JSON data files with APIs for data verification and analysis.
Supports various event types including pwr_core_element_pstate, pwr_soc_element_mpam_core_power, etc.
"""

import json
import logging
from typing import List, Dict, Any, Set
from pathlib import Path

logger = logging.getLogger(__name__)


class pwr_tlm_pkg_parser:
    """
    Generic parser for power telemetry JSON data with temporary lifetime.
    Provides APIs to verify and analyze telemetry data across different event types.
    """

    def __init__(self, file_path: str):
        """
        Initialize parser with a JSON file containing power telemetry data.

        Args:
            file_path: Path to the JSON file containing telemetry events
        """
        self.file_path = Path(file_path)
        self.events: List[Dict[str, Any]] = []
        self.event_types: Set[str] = set()
        self.provider_info: Dict[str, Any] = {}
        self._load_data()

    def _load_data(self) -> None:
        """Load and parse JSON data from file."""
        if not self.file_path.exists():
            raise FileNotFoundError(f"Telemetry file not found: {self.file_path}")

        with open(self.file_path, "r") as f:
            for line in f:
                line = line.strip()
                if line:
                    event = json.loads(line)
                    self.events.append(event)

                    # Track event types
                    if "EventName" in event:
                        self.event_types.add(event["EventName"])

                    # Store provider info from first event
                    if not self.provider_info and "ProviderName" in event:
                        self.provider_info = {
                            "name": event.get("ProviderName"),
                            "guid": event.get("ProviderGuid"),
                        }

    def get_source_die(self) -> int:
        """
        Get the SourceDie ID from the telemetry file.

        Since SourceDie is consistent within a file, this method returns the single
        SourceDie value found in the events.

        Returns:
            int: The SourceDie ID from the telemetry events

        Raises:
            ValueError: If no events are loaded or if SourceDie field is missing
        """
        if not self.events:
            raise ValueError("No events loaded - cannot determine SourceDie")

        # Get SourceDie from the first event (should be same for all events in file)
        first_event = self.events[0]

        if "SourceDie" not in first_event:
            raise ValueError("SourceDie field not found in telemetry events")

        return first_event["SourceDie"]

    def __enter__(self):
        """Context manager entry - return self for use in with statement."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit - ensure cleanup always happens."""
        # Log if we're exiting due to an exception
        if exc_type is not None:
            logger.error(
                f"Exiting pwr_tlm_pkg_parser context due to exception: {exc_type.__name__}: {exc_val}"
            )

        # Return False to propagate any exception that occurred in the with block
        return False

    def assert_equal(
        self, event_name: str, collection_id: int, event_field: str, expected_value: Any
    ) -> bool:
        """
        Assert that an event field has the expected value for a specific event and collection.

        Use this method when EventName + CollectionId uniquely identifies a single event
        (e.g., for MPAM events where each collection has one record per event type).

        Args:
            event_name: The EventName to filter by (e.g., 'pwr_soc_element_mpam_core_power')
            collection_id: The CollectionId to filter by
            event_field: The event-specific field name (e.g., 'max_mW', 'average_mW')
            expected_value: The expected value for the field

        Returns:
            True if the field matches the expected value, False otherwise.
            Errors are logged via logger.error() instead of raising exceptions.
        """
        # Find the specific event
        matching_events = [
            event
            for event in self.events
            if event.get("EventName") == event_name
            and event.get("CollectionId") == collection_id
        ]

        if not matching_events:
            error_msg = f"No event found with EventName='{event_name}' and CollectionId={collection_id}"
            logger.error(error_msg)
            return False

        if len(matching_events) > 1:
            error_msg = (
                f"Multiple events found with EventName='{event_name}' and CollectionId={collection_id}. "
                f"Use assert_equal_with_event_field() for events that need additional filtering."
            )
            logger.error(error_msg)
            return False

        event = matching_events[0]

        if event_field not in event:
            error_msg = f"Field '{event_field}' not found in event with EventName='{event_name}'"
            logger.error(error_msg)
            return False

        actual_value = event[event_field]
        return actual_value == expected_value

    def assert_equal_with_event_field(
        self,
        event_name: str,
        collection_id: int,
        event_field1: str,
        event_value1: Any,
        test_field: str,
        expected_value: Any,
    ) -> bool:
        """
        Assert that a test field has the expected value for an event identified by
        EventName, CollectionId, and one additional event field.

        Use this method when you need to specify one additional event field to uniquely
        identify the event (e.g., pstate_id for pstate events).

        Args:
            event_name: The EventName to match (e.g., 'pwr_core_element_pstate')
            collection_id: The CollectionId to match
            event_field1: First event field name to match (e.g., 'pstate_id')
            event_value1: Value for the first event field (e.g., 0)
            test_field: The field to test (e.g., 'frequency_Mhz')
            expected_value: The expected value for the test field

        Returns:
            True if the test field matches the expected value, False otherwise.
            Errors are logged via logger.error() instead of raising exceptions.
        """
        # Find events matching all criteria
        matching_events = [
            event
            for event in self.events
            if event.get("EventName") == event_name
            and event.get("CollectionId") == collection_id
            and event.get(event_field1) == event_value1
        ]

        if not matching_events:
            error_msg = (
                f"No event found with EventName='{event_name}', CollectionId={collection_id}, "
                f"{event_field1}={event_value1}"
            )
            logger.error(error_msg)
            return False

        if len(matching_events) > 1:
            error_msg = (
                f"Multiple events found with EventName='{event_name}', CollectionId={collection_id}, "
                f"{event_field1}={event_value1}"
            )
            logger.error(error_msg)
            return False

        event = matching_events[0]

        if test_field not in event:
            error_msg = (
                f"Field '{test_field}' not found in event with EventName='{event_name}'"
            )
            logger.error(error_msg)
            return False

        actual_value = event[test_field]
        return actual_value == expected_value

    def assert_range(
        self,
        event_name: str,
        collection_id: int,
        event_field: str,
        min_value: Any,
        max_value: Any,
    ) -> bool:
        """
        Assert that an event field value is within the expected range for a specific event and collection.

        Use this method when EventName + CollectionId uniquely identifies a single event
        (e.g., for MPAM events where each collection has one record per event type).

        Args:
            event_name: The EventName to match (e.g., 'pwr_soc_element_mpam_core_power')
            collection_id: The CollectionId to match
            event_field: The event-specific field name (e.g., 'max_mW', 'average_mW')
            min_value: The minimum acceptable value (inclusive)
            max_value: The maximum acceptable value (inclusive)

        Returns:
            True if the field value is within the range [min_value, max_value], False otherwise.
            Errors are logged via logger.error() instead of raising exceptions.
        """
        # Find the specific event
        matching_events = [
            event
            for event in self.events
            if event.get("EventName") == event_name
            and event.get("CollectionId") == collection_id
        ]

        if not matching_events:
            error_msg = f"No event found with EventName='{event_name}' and CollectionId={collection_id}"
            logger.error(error_msg)
            return False

        if len(matching_events) > 1:
            error_msg = (
                f"Multiple events found with EventName='{event_name}' and CollectionId={collection_id}. "
                f"Use assert_range_with_event_field() for events that need additional filtering."
            )
            logger.error(error_msg)
            return False

        event = matching_events[0]

        if event_field not in event:
            error_msg = f"Field '{event_field}' not found in event with EventName='{event_name}'"
            logger.error(error_msg)
            return False

        actual_value = event[event_field]

        if not (min_value <= actual_value <= max_value):
            error_msg = (
                f"Field '{event_field}' value {actual_value} is outside range [{min_value}, {max_value}] "
                f"for EventName='{event_name}', CollectionId={collection_id}"
            )
            logger.error(error_msg)
            return False

        return True

    def assert_range_with_event_field(
        self,
        event_name: str,
        collection_id: int,
        event_field1: str,
        event_value1: Any,
        test_field: str,
        min_value: Any,
        max_value: Any,
    ) -> bool:
        """
        Assert that a test field value is within the expected range for an event identified by
        EventName, CollectionId, and one additional event field.

        Use this method when you need to specify one additional event field to uniquely
        identify the event (e.g., pstate_id for pstate events).

        Args:
            event_name: The EventName to match (e.g., 'pwr_core_element_pstate')
            collection_id: The CollectionId to match
            event_field1: First event field name to match (e.g., 'pstate_id')
            event_value1: Value for the first event field (e.g., 0)
            test_field: The field to test (e.g., 'frequency_Mhz')
            min_value: The minimum acceptable value (inclusive)
            max_value: The maximum acceptable value (inclusive)

        Returns:
            True if the test field value is within the range [min_value, max_value], False otherwise.
            Errors are logged via logger.error() instead of raising exceptions.
        """
        # Find events matching all criteria
        matching_events = [
            event
            for event in self.events
            if event.get("EventName") == event_name
            and event.get("CollectionId") == collection_id
            and event.get(event_field1) == event_value1
        ]

        if not matching_events:
            error_msg = (
                f"No event found with EventName='{event_name}', CollectionId={collection_id}, "
                f"{event_field1}={event_value1}"
            )
            logger.error(error_msg)
            return False

        if len(matching_events) > 1:
            error_msg = (
                f"Multiple events found with EventName='{event_name}', CollectionId={collection_id}, "
                f"{event_field1}={event_value1}"
            )
            logger.error(error_msg)
            return False

        event = matching_events[0]

        if test_field not in event:
            error_msg = (
                f"Field '{test_field}' not found in event with EventName='{event_name}'"
            )
            logger.error(error_msg)
            return False

        actual_value = event[test_field]

        if not (min_value <= actual_value <= max_value):
            error_msg = (
                f"Field '{test_field}' value {actual_value} is outside range [{min_value}, {max_value}] "
                f"for EventName='{event_name}', CollectionId={collection_id}, {event_field1}={event_value1}"
            )
            logger.error(error_msg)
            return False

        return True


# to run the main functionality below from a repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/library/power_telemetry_tests/pwr_tlm_pkg_parser.py  # noqa: E501


def main():
    """Test the pwr_tlm_pkg_parser with sample data files."""

    # Setup logging similar to pwr_tlm_lib.py
    import sys

    logging.basicConfig(
        level=logging.DEBUG,
        handlers=[logging.StreamHandler(sys.stdout)],
    )

    # Set our logger to INFO level for cleaner output during testing
    logging.getLogger(__name__).setLevel(logging.DEBUG)

    # Test file paths
    mine_json = r"c:\decode\throttle\mine.json"
    die0_json = r"c:\\decode\\die0_1.json"

    print("=== Testing pwr_tlm_pkg_parser ===\n")

    # Test logger
    logger.info("Logger test: INFO level message")
    logger.error("Logger test: ERROR level message")
    logger.debug("Logger test: DEBUG level message")

    # Test with mine.json using context manager pattern
    print("Testing with mine.json...")
    with pwr_tlm_pkg_parser(mine_json) as parser1:
        print(f"Total events loaded: {len(parser1.events)}")
        print(f"Event types found: {list(parser1.event_types)}")
        print(f"Provider info: {parser1.provider_info}")
        print(f"Source die: {parser1.get_source_die()}")

        # Test assert_equal with known values from mine.json
        print("\n--- Testing assert_equal API ---")

        # Test collection 0: max_mW should be 264
        result = parser1.assert_equal(
            "pwr_soc_element_mpam_core_power", 0, "max_mW", 264
        )
        print(f"Collection 0, max_mW == 264: {result}")

        # Test collection 1: max_mW should be 286
        result = parser1.assert_equal(
            "pwr_soc_element_mpam_core_power", 1, "max_mW", 286
        )
        print(f"Collection 1, max_mW == 286: {result}")

        # Test collection 31: max_mW should be 154
        result = parser1.assert_equal(
            "pwr_soc_element_mpam_core_power", 31, "max_mW", 154
        )
        print(f"Collection 31, max_mW == 154: {result}")

        # Test with wrong value (should return False)
        result = parser1.assert_equal(
            "pwr_soc_element_mpam_core_power", 0, "max_mW", 999
        )
        print(f"Collection 0, max_mW == 999 (wrong): {result}")

        # Test error cases with parser1 in scope
        print("\n--- Testing Error Cases ---")

        # Test with non-existent event (should return False and log error)
        result = parser1.assert_equal("non_existent_event", 0, "some_field", 123)
        print(f"Non-existent event test result: {result}")

        # Test with non-existent field (should return False and log error)
        result = parser1.assert_equal(
            "pwr_soc_element_mpam_core_power", 0, "non_existent_field", 123
        )
        print(f"Non-existent field test result: {result}")

        # Test range APIs with parser1 in scope
        print("\n--- Testing Range APIs ---")

        # Test collection 0: max_mW should be within range 260-270 (should pass)
        result = parser1.assert_range(
            "pwr_soc_element_mpam_core_power", 0, "max_mW", 260, 270
        )
        print(f"Collection 0, max_mW in range [260, 270]: {result}")

        # Test collection 0: max_mW should be within range 100-200 (should fail)
        result = parser1.assert_range(
            "pwr_soc_element_mpam_core_power", 0, "max_mW", 100, 200
        )
        print(f"Collection 0, max_mW in range [100, 200]: {result}")

    # Test with die0_1.json using context manager pattern
    print("\nTesting with die0_1.json...")
    with pwr_tlm_pkg_parser(die0_json) as parser2:
        print(f"Total events loaded: {len(parser2.events)}")
        print(f"Event types found: {list(parser2.event_types)}")
        print(f"Source die: {parser2.get_source_die()}")

        # Test assert_equal with known values from die0_1.json
        print("\n--- Testing assert_equal API ---")

        # Test collection 0, pstate_id 0: frequency should be 3700
        result = parser2.assert_equal_with_event_field(
            "pwr_core_element_pstate", 0, "pstate_id", 0, "frequency_Mhz", 3700
        )
        print(f"Collection 0, pstate_id 0, frequency_Mhz == 3700: {result}")

        # Test with a known active pstate that should have reasonable residency
        result = parser2.assert_range_with_event_field(
            "pwr_core_element_pstate", 3, "pstate_id", 5, "residency_mS", 1, 10000
        )
        print(
            f"Collection 3, pstate_id 5, has reasonable residency [1-10000ms]: {result}"
        )

        # Test range APIs with parser2 in scope
        print("\n--- Testing Range APIs with Event Field ---")

        # Test collection 0, pstate_id 0: frequency should be within range 3600-3800 (should pass)
        result = parser2.assert_range_with_event_field(
            "pwr_core_element_pstate", 0, "pstate_id", 0, "frequency_Mhz", 3600, 3800
        )
        print(
            f"Collection 0, pstate_id 0, frequency_Mhz in range [3600, 3800]: {result}"
        )

        # Test collection 0, pstate_id 0: frequency should be within range 4000-5000 (should fail)
        result = parser2.assert_range_with_event_field(
            "pwr_core_element_pstate", 0, "pstate_id", 0, "frequency_Mhz", 4000, 5000
        )
        print(
            f"Collection 0, pstate_id 0, frequency_Mhz in range [4000, 5000]: {result}"
        )


if __name__ == "__main__":
    main()
