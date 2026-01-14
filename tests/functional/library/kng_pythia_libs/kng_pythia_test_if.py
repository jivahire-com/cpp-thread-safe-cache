# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- This module contains interface and parsing related utility functions for pythia functional tests.
"""

import re
import time

from pythia.tdk.common.connections.serial_connection import EmptySerialLinesError

class KngPythiaTestIF():

    @staticmethod
    def parse_log(pythia_log, uart_log, uart_pass_logs, uart_fail_logs):
        """
        Check for failure and success strings in the provided lines, removing numbers, 
        and return a summary. The test will fail if any failure strings are found or 
        if any success strings are missing.

        :param uart_log: List of lines read from UART.
        :param uart_pass_logs: List of success strings to look for.
        :param uart_fail_logs: List of failure strings to look for.
        :return: A dictionary with success, failure, and missing success strings.
                Returns False if any failure string is found or any success string is missing.
        """
        print("Raw Log:", uart_log)

        result = {
            "success": [],
            "failure": [],
            "missing_success": []
        }

        # Remove numbers from success and failure strings
        uart_pass_logs = [re.sub(r'\d+', '', pass_str) for pass_str in uart_pass_logs]
        uart_fail_logs = [re.sub(r'\d+', '', fail_str) for fail_str in uart_fail_logs]

        # Compile a pattern for each failure string to match exact words
        fail_patterns = [re.compile(r'\b{}\b'.format(re.escape(fail_string))) for fail_string in uart_fail_logs]

        uart_log_lines_formatted = uart_log.split("\n")
        
        # Check for any failure string in any line
        for line in uart_log_lines_formatted:
            # Remove numbers from the current line
            uart_log_lines_without_numbers = re.sub(r'\d+', '', line).strip()

            # Check for failure patterns
            for fail_pattern in fail_patterns:
                if re.search(fail_pattern, uart_log_lines_without_numbers):
                    pythia_log.error(f"Failure string '{fail_pattern.pattern}' found in line: {uart_log_lines_without_numbers}")
                    result["failure"].append(fail_pattern.pattern)
                    return False  # Immediate test failure if a failure string is found

        # Track success strings found and missing
        pass_found = set()
        for line in uart_log_lines_formatted:
            # Remove numbers from the current line
            uart_log_lines_without_numbers = re.sub(r'\d+', '', line).strip()

            # Check for success strings
            for pass_str in uart_pass_logs:
                if pass_str in uart_log_lines_without_numbers:
                    pythia_log.info(f"Success string '{pass_str}' found in line: {uart_log_lines_without_numbers}")
                    result["success"].append(pass_str)
                    pass_found.add(pass_str)

        # Identify missing success strings
        for pass_str in uart_pass_logs:
            if pass_str not in pass_found:
                pythia_log.error(f"Success string '{pass_str}' not found in the data.")
                result["missing_success"].append(pass_str)

        # Fail the test if any success strings are missing
        if result["missing_success"]:
            return False  # Test fails if any success strings are missing

        return result

    @staticmethod
    def parse_logs_v2(logger, logs, pass_logs, fail_logs):
        match_cnt = 0
        temp_lst = list(pass_logs)
        pass_log_len = len(pass_logs)

        for line in logs.split("\n"):

            for reg_str in fail_logs:
                matches = re.findall(reg_str, line)
                if(len(matches) != 0):
                    logger.error(f"Found unexpected log {line}")
                    return False

            for i in range(len(temp_lst)):
                reg_str = temp_lst[i]
                matches = re.findall(reg_str, line)
                if(len(matches) != 0):
                    match_cnt += len(matches)
                    logger.info(f"Regex Match: {reg_str}")
                    # logger.debug(line)
                    logger.info(f"Parse_log: Found {match_cnt} matches")
                    del temp_lst[i]
                    break

        if(match_cnt != pass_log_len):
            logger.error(f"Total Pass log matches: {match_cnt} less than expected {pass_log_len}")
            logger.error(f"Expected_logs: {pass_logs}")
            logger.error(f"Actual logs: {logs}")
            return False
        return True
    
    # TODO: ADO: 2109191 Regex support should ideally be provided by Pythia 
    @staticmethod
    def read_until_regex(proc_uart, regex_str, time_out = 10):
        st = time.monotonic()
        found = False
        ret_lst = []

        while((time.monotonic() - st) < time_out):
            try:
                ln = proc_uart.read_line()
                ret_lst.append(ln)
            except EmptySerialLinesError:
                continue
            matches = re.findall(regex_str, ln)
            if(len(matches) != 0):
                found = True
                break
        
        return (found, "\n".join(ret_lst))
