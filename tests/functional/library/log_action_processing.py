# Copyright (c) Microsoft Corporation. All rights reserved.

"""
Python-based Pythia 2.0 Test Log Utilities

This module provides interface and parsing utility functions for Pythia functional tests.

Features:
- Log parsing for success and failure detection
- Regex-based log matching and validation
- UART log reading with timeout handling
"""

import re
import time
from pythia.tdk.common.connections.serial_connection import EmptySerialLinesError

class LogActionProcessing:
    """
    Class providing methods to parse logs and extract relevant information,
    including success and failure detection, regex matching, and UART reading.
    """

    @staticmethod
    def parse_log(pythia_log, uart_log, uart_pass_logs, uart_fail_logs, ignore_numbers=False):
        """
        Parses UART logs to identify success and failure patterns.

        Args:
            pythia_log (Logger): Logger instance for logging results.
            uart_log (str): UART log contents.
            uart_pass_logs (list): List of success strings to check for.
            uart_fail_logs (list): List of failure strings to check for.
            ignore_numbers (bool): Whether to ignore numbers in logs.

        Returns:
            dict or bool: Summary dictionary with detected logs, or False on failure detection.
        """
        print("Raw Log:", uart_log)

        result = {"success": [], "failure": [], "missing_success": []}

        if ignore_numbers:
            uart_pass_logs = [re.sub(r'\d+', '', pass_str) for pass_str in uart_pass_logs]
            uart_fail_logs = [re.sub(r'\d+', '', fail_str) for fail_str in uart_fail_logs]

        fail_patterns = [re.compile(r'\b{}\b'.format(re.escape(fail_string))) for fail_string in uart_fail_logs]
        uart_log_lines = uart_log.split("\n")

        for line in uart_log_lines:
            processed_line = re.sub(r'\d+', '', line).strip() if ignore_numbers else line.strip()
            for fail_pattern in fail_patterns:
                if re.search(fail_pattern, processed_line):
                    pythia_log.error(f"Failure string '{fail_pattern.pattern}' found in line: {processed_line}")
                    result["failure"].append(fail_pattern.pattern)
                    return False

        pass_found = set()
        for line in uart_log_lines:
            processed_line = re.sub(r'\d+', '', line).strip() if ignore_numbers else line.strip()
            for pass_str in uart_pass_logs:
                if pass_str in processed_line:
                    pythia_log.info(f"Success string '{pass_str}' found in line: {processed_line}")
                    result["success"].append(pass_str)
                    pass_found.add(pass_str)

        for pass_str in uart_pass_logs:
            if pass_str not in pass_found:
                pythia_log.error(f"Success string '{pass_str}' not found in the data.")
                result["missing_success"].append(pass_str)

        return False if result["missing_success"] else result

    @staticmethod
    def parse_logs_v2(logger, logs, pass_logs, fail_logs, sub_dict={}):
        """
        Parses logs using regex-based matching for pass and fail conditions.

        Args:
            logger (Logger): Logger instance.
            logs (str): Log data to be processed.
            pass_logs (list): Regex patterns for expected success conditions.
            fail_logs (list): Regex patterns for unexpected failure conditions.
            sub_dict (dict): Optional dictionary for additional processing.

        Returns:
            bool: True if all success conditions are met, False if failures occur.
        """
        match_cnt = 0
        temp_lst = list(pass_logs)
        pass_log_len = len(pass_logs)

        for line in logs.split("\n"):
            for reg_str in fail_logs:
                if re.findall(reg_str, line):
                    logger.error(f"Found unexpected log {line}")
                    return False

            for i, reg_str in enumerate(temp_lst):
                if re.findall(reg_str, line):
                    match_cnt += 1
                    logger.info(f"Regex Match: {reg_str}")
                    logger.info(f"Parse_log: Found {match_cnt} matches")
                    del temp_lst[i]
                    break

        if match_cnt != pass_log_len:
            logger.error(f"Total Pass log matches: {match_cnt} less than expected {pass_log_len}")
            logger.error(f"Expected_logs: {pass_logs}")
            logger.error(f"Actual logs: {logs}")
            return False
        return True

    @staticmethod
    def read_until_regex(proc_uart, regex_str, time_out=10):
        """
        Reads UART lines until a regex pattern is matched or a timeout occurs.

        Args:
            proc_uart (object): UART interface instance.
            regex_str (str): Regular expression to match in the logs.
            time_out (int): Timeout in seconds.

        Returns:
            tuple: (bool, str) - Boolean indicating match success, and collected log lines.
        """
        st = time.monotonic()
        found = False
        ret_lst = []

        while (time.monotonic() - st) < time_out:
            try:
                ln = proc_uart.read_line()
                ret_lst.append(ln)
            except EmptySerialLinesError:
                continue
            if re.findall(regex_str, ln):
                found = True
                break

        return found, "\n".join(ret_lst)
