#!/usr/bin/env python3
"""
Boot Polling Script - D2D Stress Test

================================================================================
DESCRIPTION
================================================================================
This script automates power cycle stress testing for D2D (Die-to-Die) validation.
It monitors the SCP (System Control Processor) serial output for power error
messages while waiting for the AP_NS (Application Processor Non-Secure) to boot
to the SAC> prompt.

================================================================================
USAGE
================================================================================
    python boot_poll_d2d_stress.py

The script will prompt for the following inputs (no command-line arguments):
    1. SSH Host (RSCM IP)     - IP address of the RSCM server to connect to
    2. SSH Password           - Password for root user (hidden input)
    3. Node ID                - Node number for serial sessions (e.g., 5)
    4. BMC sudo password      - Password for 'su' command on BMC (hidden input)
    5. SCP Error Threshold    - Error count threshold to reach (optional, default: 300)

================================================================================
PREREQUISITES
================================================================================
    - Python 3.6 or higher
    - paramiko library: pip install paramiko
    - Network access to the RSCM server
    - Valid credentials for SSH and BMC

================================================================================
WORKFLOW
================================================================================
    1. Connect to RSCM via SSH and establish 3 serial sessions:
       - BMC (port 2200): For power cycle commands
       - SCP (port 2202): For monitoring power error messages  
       - AP_NS (port 22): For detecting SAC> boot prompt

    2. Execute power cycle on BMC using 'ipmitool power cycle'

    3. Monitor in parallel:
       - SCP: Count occurrences of "SocPowerModule::PowerErrorParam:"
       - AP_NS: Wait for "SAC>" prompt (up to 10 minutes)

    4. Exit conditions:
       - SUCCESS: If SCP error count exceeds threshold (configurable, default 300) (before or after SAC>)
       - CONTINUE: If SAC> appears and 3-minute post-boot monitoring passes
                   without exceeding threshold, start another power cycle

================================================================================
OUTPUT FILES
================================================================================
Logs are saved to: <script_directory>/logs/
    - bmc.log    : BMC serial output and power cycle commands
    - scp.log    : SCP serial output (appended across all cycles)
    - ap_ns.log  : AP_NS serial output (cleared each cycle)

Note: AP_NS log is cleared before each power cycle to prevent large file sizes.
SCP log is preserved/appended across all cycles to track error patterns throughout the test run.

================================================================================
EXIT CODES
================================================================================
    0 - Success (threshold exceeded or normal completion)
    1 - Error (connection failure, exception, etc.)

================================================================================
INTERRUPTING THE SCRIPT
================================================================================
Press Ctrl+C to stop the script gracefully. Final statistics will be displayed.

================================================================================
EXAMPLE SESSION
================================================================================
    $ python boot_poll_d2d_stress.py

    ============================================================
    Boot Polling Script - Configuration
    ============================================================

    Enter SSH Host (RSCM IP): 172.29.131.23
    Enter SSH Password: 
    Enter Node ID (for serial session -i <node_id>): 5
    Enter BMC sudo password (for 'su' on BMC):
    Enter SCP error threshold [default: 300]: 500 

    ============================================================
    Configuration:
      SSH Host: 172.29.131.23
      SSH User: root
      SSH Password: ****************************
      Node ID: 5
      BMC Sudo Password: *******
      BMC Serial: Start serial session -i 5 -p 2200
      SCP Serial: Start serial session -i 5 -p 2202
      AP_NS Serial: Start serial session -i 5 -p 22
      SCP Error Threshold: 500
    ============================================================

    Continue with this configuration? [Y/n]: y

================================================================================
"""

import os
import signal
import sys
import time
import getpass
from pathlib import Path
import paramiko
import threading
from datetime import datetime, timedelta

# These will be set by user input
SSH_HOST = None
SSH_USERNAME = "root"
SSH_PASSWORD = None
NODE_ID = None
BMC_SUDO_PASSWORD = None

# Serial session command templates (node_id will be filled in)
BMC_SERIAL_CMD_TEMPLATE = "Start serial session -i {node_id} -p 2200"
SCP_SERIAL_CMD_TEMPLATE = "Start serial session -i {node_id} -p 2202"
AP_NS_SERIAL_CMD_TEMPLATE = "Start serial session -i {node_id} -p 22"

# Get script directory for logs - use absolute path
SCRIPT_DIR = Path(__file__).parent.resolve()
LOG_DIR = SCRIPT_DIR / "logs"
LOG_DIR.mkdir(exist_ok=True)

# Log file paths (absolute)
BMC_LOG_PATH = LOG_DIR / "bmc.log"
SCP_LOG_PATH = LOG_DIR / "scp.log"
AP_NS_LOG_PATH = LOG_DIR / "ap_ns.log"

# Global counters and tracking
SCRIPT_START_TIME = datetime.now()
POWER_CYCLE_COUNT = 0
SCP_ERROR_COUNT = 0
SCP_ERROR_THRESHOLD = 300  # Default threshold, configurable via user input
SAC_FOUND = False
SHOULD_EXIT = False

# Thread-safe locks
EXIT_LOCK = threading.Lock()
LOG_LOCK = threading.Lock()
SCP_COUNT_LOCK = threading.Lock()


def get_user_input():
    """Prompt user for SSH host, password, node ID, and BMC sudo password"""
    global SSH_HOST, SSH_PASSWORD, NODE_ID, BMC_SUDO_PASSWORD, SCP_ERROR_THRESHOLD
    
    print("=" * 60)
    print("Boot Polling Script - Configuration")
    print("=" * 60)
    print()
    
    # Get SSH Host
    SSH_HOST = input("Enter SSH Host (RSCM IP): ").strip()
    if not SSH_HOST:
        print("Error: SSH Host cannot be empty")
        sys.exit(1)
    
    # Get SSH Password (hidden)
    SSH_PASSWORD = getpass.getpass("Enter SSH Password: ")
    if not SSH_PASSWORD:
        print("Error: SSH Password cannot be empty")
        sys.exit(1)
    
    # Get Node ID
    NODE_ID = input("Enter Node ID (for serial session -i <node_id>): ").strip()
    if not NODE_ID:
        print("Error: Node ID cannot be empty")
        sys.exit(1)
    
    # Validate node ID is a number
    try:
        int(NODE_ID)
    except ValueError:
        print(f"Error: Node ID must be a number, got: {NODE_ID}")
        sys.exit(1)
    
    # Get BMC sudo password (hidden)
    BMC_SUDO_PASSWORD = getpass.getpass("Enter BMC sudo password (for 'su' on BMC): ")
    if not BMC_SUDO_PASSWORD:
        print("Error: BMC sudo password cannot be empty")
        sys.exit(1)
    

    # Get SCP error threshold (optional, default 300)
    threshold_input = input("Enter SCP error threshold [default: 300]: ").strip()
    if threshold_input:
        try:
            SCP_ERROR_THRESHOLD = int(threshold_input)
            if SCP_ERROR_THRESHOLD <= 0:
                print("Error: Threshold must be a positive number")
                sys.exit(1)
        except ValueError:
            print(f"Error: Invalid threshold value: {threshold_input}")
            sys.exit(1)
    # else: keep default value of 300
    print()
    print("=" * 60)
    print(f"Configuration:")
    print(f"  SSH Host: {SSH_HOST}")
    print(f"  SSH User: {SSH_USERNAME}")
    print(f"  SSH Password: {'*' * len(SSH_PASSWORD)}")
    print(f"  Node ID: {NODE_ID}")
    print(f"  BMC Sudo Password: {'*' * len(BMC_SUDO_PASSWORD)}")
    print(f"  BMC Serial: Start serial session -i {NODE_ID} -p 2200")
    print(f"  SCP Serial: Start serial session -i {NODE_ID} -p 2202")
    print(f"  AP_NS Serial: Start serial session -i {NODE_ID} -p 22")
    print(f"  SCP Error Threshold: {SCP_ERROR_THRESHOLD}")
    print("=" * 60)
    print()
    
    confirm = input("Continue with this configuration? [Y/n]: ").strip().lower()
    if confirm and confirm != 'y' and confirm != 'yes':
        print("Aborted by user")
        sys.exit(0)
    
    print()


def get_serial_commands():
    """Get serial session commands with node ID filled in"""
    return {
        'BMC': BMC_SERIAL_CMD_TEMPLATE.format(node_id=NODE_ID),
        'SCP': SCP_SERIAL_CMD_TEMPLATE.format(node_id=NODE_ID),
        'AP_NS': AP_NS_SERIAL_CMD_TEMPLATE.format(node_id=NODE_ID),
    }


def write_to_log(log_path, text):
    """Thread-safe log writing"""
    with LOG_LOCK:
        try:
            with open(log_path, "a", encoding="utf-8") as f:
                f.write(text)
                f.flush()
        except Exception as e:
            print(f"[ERROR] Failed to write to {log_path}: {e}")


class SerialSession:
    def __init__(self, session_name, serial_cmd, sudo_password=None):
        self.session_name = session_name
        self.serial_cmd = serial_cmd
        self.sudo_password = sudo_password
        self.client = None
        self.channel = None
        self.is_root = False

    def open(self):
        log_step(f"[{self.session_name}] Connecting to {SSH_HOST} for serial session: {self.serial_cmd}")
        self.client = paramiko.SSHClient()
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        
        try:
            self.client.connect(
                SSH_HOST,
                username=SSH_USERNAME,
                password=SSH_PASSWORD,
                look_for_keys=False,
                allow_agent=False,
            )
            self.channel = self.client.invoke_shell()
            self.channel.settimeout(0.0)
            
            # Wait for initial prompt and clear output
            time.sleep(3)
            self.read()  # Clear initial output
            
            # Start serial session
            log_step(f"[{self.session_name}] Starting serial session: {self.serial_cmd}")
            self.write(f"{self.serial_cmd}\n")
            time.sleep(5)
            
            # Read initial serial session output
            initial_output = self.read()
            log_step(f"[{self.session_name}] Serial session established, output length: {len(initial_output)}")
            
        except Exception as exc:
            log_step(f"[{self.session_name}] Failed to establish serial session: {exc}")
            self.close()
            raise RuntimeError(f"Unable to establish {self.session_name} serial session") from exc
        
        log_step(f"[{self.session_name}] Serial session ready")

    def become_root(self):
        """Ensure we are root user for BMC operations"""
        if self.is_root or self.session_name != "BMC":
            return True
            
        if not self.sudo_password:
            return True
            
        log_step(f"[{self.session_name}] Attempting to become root with su command")
        self.read()  # Clear buffer
        
        self.write("su\n")
        time.sleep(3)
        
        buffer = self.read()
        if b"password:" in buffer.lower() or b"Password:" in buffer:
            log_step(f"[{self.session_name}] Providing sudo password")
            self.write(f"{self.sudo_password}\n")
            time.sleep(3)
            
            verify_buffer = self.read()
            if b"root@" in verify_buffer or b"#" in verify_buffer:
                log_step(f"[{self.session_name}] ✓ Successfully became root")
                self.is_root = True
            else:
                log_step(f"[{self.session_name}] ⚠ Root verification unclear, proceeding anyway")
                self.is_root = True
        else:
            log_step(f"[{self.session_name}] No password prompt found, may already be root")
            self.is_root = True
            
        return True

    def write(self, data):
        if not self.channel:
            raise RuntimeError(f"{self.session_name} serial session not open")
        if isinstance(data, str):
            data = data.encode("utf-8")
        self.channel.send(data)

    def read(self):
        if not self.channel or self.channel.closed:
            return b""
        if self.channel.recv_ready():
            try:
                return self.channel.recv(4096)
            except Exception:
                return b""
        return b""

    def close(self):
        log_step(f"[{self.session_name}] Closing serial session")
        if self.channel:
            self.channel.close()
            self.channel = None
        if self.client:
            self.client.close()
            self.client = None


def log_step(message):
    timestamp = time.strftime("%F %T")
    print(f"[STEP {timestamp}] {message}")
    sys.stdout.flush()


def log_chunk_to_file(log_path, chunk, prefix=None):
    """Thread-safe chunk logging to file"""
    if not chunk:
        return
    text = chunk.decode("utf-8", errors="replace")
    
    # Write to file
    write_to_log(log_path, text)
    
    # Also print to console with prefix
    if prefix:
        stdout_encoding = sys.stdout.encoding or "utf-8"
        safe_text = text.encode(stdout_encoding, errors="backslashreplace").decode(
            stdout_encoding, errors="ignore"
        )
        print(f"[{prefix}] {safe_text}", end="")
        sys.stdout.flush()


def calculate_runtime():
    """Calculate total runtime since script started"""
    runtime = datetime.now() - SCRIPT_START_TIME
    total_seconds = int(runtime.total_seconds())
    hours = total_seconds // 3600
    minutes = (total_seconds % 3600) // 60
    seconds = total_seconds % 60
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


def clear_cycle_logs():
    """Clear AP_NS log before each power cycle (SCP log is appended across cycles)"""
    log_step(f"[LOGS] Clearing AP_NS log for new cycle (SCP log is preserved/appended)...")

    # SCP log is NOT cleared - it accumulates across power cycles
    # This allows tracking error patterns across the entire test run

    # Clear AP_NS log only (SAC> detection log)
    try:
        with open(AP_NS_LOG_PATH, "w", encoding="utf-8") as f:
            f.write(f"=== Log cleared at {time.strftime('%F %T')} for new power cycle ===\n")
        log_step(f"[LOGS] AP_NS log cleared: {AP_NS_LOG_PATH}")
    except Exception as e:
        log_step(f"[LOGS] Warning: Could not clear AP_NS log: {e}")


def execute_power_cycle(bmc_session):
    """Execute power cycle command and increment counter"""
    global POWER_CYCLE_COUNT
    POWER_CYCLE_COUNT += 1
    
    # Clear AP_NS log before power cycle (SCP log is preserved across cycles)
    clear_cycle_logs()
    
    log_step(f"[BMC] Executing power cycle #{POWER_CYCLE_COUNT}")
    power_command = "ipmitool power cycle\n"
    bmc_session.write(power_command)
    
    # Log to BMC log file
    log_msg = f"\n[{time.strftime('%F %T')}] >>> Power Cycle #{POWER_CYCLE_COUNT}: {power_command}"
    write_to_log(BMC_LOG_PATH, log_msg)
    
    return POWER_CYCLE_COUNT


def drain_connection(session, log_path, duration=3, prefix=None):
    end = time.time() + duration
    while time.time() < end:
        chunk = session.read()
        if chunk:
            log_chunk_to_file(log_path, chunk, prefix=prefix)
        else:
            time.sleep(0.1)


def monitor_scp_thread(scp_session, stop_event):
    """Thread to continuously monitor SCP for power error messages"""
    global SCP_ERROR_COUNT, SHOULD_EXIT
    
    pattern = b"SocPowerModule::PowerErrorParam:"
    pattern_str = pattern.decode("utf-8")
    buffer = bytearray()
    
    log_step(f"[SCP-MONITOR] Started monitoring for '{pattern_str}'")
    log_step(f"[SCP-MONITOR] Logging to: {SCP_LOG_PATH}")
    
    while not stop_event.is_set():
        chunk = scp_session.read()
        if chunk:
            # Log chunk to file (thread-safe)
            log_chunk_to_file(SCP_LOG_PATH, chunk, prefix="SCP")
            buffer.extend(chunk)
            
            # Count occurrences
            buffer_str = buffer.decode("utf-8", errors="replace")
            new_count = buffer_str.lower().count(pattern_str.lower())
            
            with SCP_COUNT_LOCK:
                if new_count > SCP_ERROR_COUNT:
                    SCP_ERROR_COUNT = new_count
                    log_step(f"[SCP-MONITOR] Power error count: {SCP_ERROR_COUNT}")
                    
                    # Check threshold
                    if SCP_ERROR_COUNT > SCP_ERROR_THRESHOLD:
                        with EXIT_LOCK:
                            SHOULD_EXIT = True
                        log_step(f"[SCP-MONITOR] 🎯 THRESHOLD EXCEEDED! Count: {SCP_ERROR_COUNT}")
                        return
            
            # Keep buffer manageable
            if len(buffer) > 10000:
                buffer = buffer[-5000:]
        else:
            time.sleep(0.1)
    
    with SCP_COUNT_LOCK:
        log_step(f"[SCP-MONITOR] Stopped. Final count: {SCP_ERROR_COUNT}")


def wait_for_sac_prompt(ap_ns_session, timeout=600):
    """Wait for SAC> prompt on AP_NS, returns True if found
    
    Timeout is 600s (10 minutes) to allow for longer boot times.
    Uses case-insensitive matching and progress logging.
    """
    global SAC_FOUND, SHOULD_EXIT
    
    buffer = bytearray()
    deadline = time.time() + timeout
    last_data_time = time.time()
    bytes_received = 0
    last_progress_log = 0
    
    log_step(f"[AP_NS] Waiting for 'SAC>' prompt (timeout: {timeout}s / 10 minutes)")
    log_step(f"[AP_NS] Logging to: {AP_NS_LOG_PATH}")
    
    while time.time() < deadline:
        # Check if SCP already found enough errors
        with EXIT_LOCK:
            if SHOULD_EXIT:
                log_step(f"[AP_NS] SCP threshold already reached, stopping SAC> wait")
                return False
        
        chunk = ap_ns_session.read()
        if chunk:
            last_data_time = time.time()
            bytes_received += len(chunk)
            log_chunk_to_file(AP_NS_LOG_PATH, chunk, prefix="AP_NS")
            buffer.extend(chunk)
            
            # Check for SAC> with flexible matching (case insensitive, allow variations)
            buffer_str = buffer.decode("utf-8", errors="replace").upper()
            if "SAC>" in buffer_str:
                SAC_FOUND = True
                log_step(f"[AP_NS] ✓ SAC> prompt detected! (received {bytes_received} bytes total)")
                return True
            
            # Keep buffer manageable but large enough for detection
            if len(buffer) > 50000:
                buffer = buffer[-25000:]
        else:
            time.sleep(0.1)
        
        # Progress logging every 30 seconds
        elapsed = int(time.time() - (deadline - timeout))
        if elapsed > 0 and elapsed % 30 == 0 and elapsed != last_progress_log:
            last_progress_log = elapsed
            since_data = int(time.time() - last_data_time)
            remaining = int(deadline - time.time())
            log_step(f"[AP_NS] Still waiting for SAC>... ({elapsed}s elapsed, {remaining}s remaining, {bytes_received} bytes received, {since_data}s since last data)")
    
    log_step(f"[AP_NS] Timeout waiting for SAC> prompt after {timeout}s. Total bytes received: {bytes_received}")
    return False


def monitor_scp_for_duration(scp_session, duration_seconds):
    """Monitor SCP for a specific duration after SAC> is found"""
    global SCP_ERROR_COUNT, SHOULD_EXIT
    
    pattern = b"SocPowerModule::PowerErrorParam:"
    pattern_str = pattern.decode("utf-8")
    buffer = bytearray()
    
    deadline = time.time() + duration_seconds
    log_step(f"[SCP] Post-SAC> monitoring for {duration_seconds}s (3 minutes)")
    
    while time.time() < deadline:
        chunk = scp_session.read()
        if chunk:
            log_chunk_to_file(SCP_LOG_PATH, chunk, prefix="SCP")
            buffer.extend(chunk)
            
            # Count occurrences
            buffer_str = buffer.decode("utf-8", errors="replace")
            new_count = buffer_str.lower().count(pattern_str.lower())
            
            with SCP_COUNT_LOCK:
                if new_count > SCP_ERROR_COUNT:
                    SCP_ERROR_COUNT = new_count
                    log_step(f"[SCP] Power error count: {SCP_ERROR_COUNT}")
                    
                    # Check threshold
                    if SCP_ERROR_COUNT > SCP_ERROR_THRESHOLD:
                        SHOULD_EXIT = True
                        log_step(f"[SCP] 🎯 THRESHOLD EXCEEDED! Count: {SCP_ERROR_COUNT}")
                        return True
            
            # Keep buffer manageable
            if len(buffer) > 10000:
                buffer = buffer[-5000:]
        else:
            time.sleep(0.1)
    
    with SCP_COUNT_LOCK:
        log_step(f"[SCP] 3-minute monitoring complete. Error count: {SCP_ERROR_COUNT}")
        return SCP_ERROR_COUNT > SCP_ERROR_THRESHOLD


def print_final_stats(success=False):
    """Print final statistics"""
    final_runtime = calculate_runtime()
    
    stats_msg = f"\n{'='*60}\n"
    if success:
        stats_msg += f"✅ MISSION ACCOMPLISHED!\n"
    else:
        stats_msg += f"⚠ Script ended without reaching threshold\n"
    stats_msg += f"📊 Final Statistics:\n"
    stats_msg += f"   • Total runtime: {final_runtime}\n"
    stats_msg += f"   • Power cycles executed: {POWER_CYCLE_COUNT}\n"
    stats_msg += f"   • SCP power errors detected: {SCP_ERROR_COUNT}\n"
    stats_msg += f"   • Started at: {SCRIPT_START_TIME.strftime('%Y-%m-%d %H:%M:%S')}\n"
    stats_msg += f"   • Ended at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
    stats_msg += f"{'='*60}\n"
    
    log_step(stats_msg)
    
    # Write to all log files
    write_to_log(BMC_LOG_PATH, stats_msg)
    write_to_log(SCP_LOG_PATH, stats_msg)
    write_to_log(AP_NS_LOG_PATH, stats_msg)


def main():
    global SCP_ERROR_COUNT, SAC_FOUND, SHOULD_EXIT, SCRIPT_START_TIME
    
    # Get user input for SSH host, password, node ID, and BMC sudo password
    get_user_input()
    
    # Reset start time after user input
    SCRIPT_START_TIME = datetime.now()
    
    # Get serial commands with node ID
    serial_cmds = get_serial_commands()
    
    log_step(f"🚀 Boot polling script started at {SCRIPT_START_TIME.strftime('%Y-%m-%d %H:%M:%S')}")
    log_step(f"📁 Script directory: {SCRIPT_DIR}")
    log_step(f"📁 Logs directory: {LOG_DIR}")
    log_step(f"📁 BMC log: {BMC_LOG_PATH}")
    log_step(f"📁 SCP log: {SCP_LOG_PATH}")
    log_step(f"📁 AP_NS log: {AP_NS_LOG_PATH}")
    log_step("Starting serial sessions for BMC, SCP, and AP_NS")
    
    # Write runtime start to all log files (without any credentials)
    start_msg = f"\n{'='*60}\n"
    start_msg += f"=== SCRIPT START: {SCRIPT_START_TIME.strftime('%Y-%m-%d %H:%M:%S')} ===\n"
    start_msg += f"SSH Host: {SSH_HOST}\n"
    start_msg += f"Node ID: {NODE_ID}\n"
    start_msg += f"{'='*60}\n"
    write_to_log(BMC_LOG_PATH, start_msg)
    write_to_log(SCP_LOG_PATH, start_msg)
    write_to_log(AP_NS_LOG_PATH, start_msg)
    
    # Create three separate serial sessions (only BMC needs sudo password)
    bmc = SerialSession("BMC", serial_cmds['BMC'], sudo_password=BMC_SUDO_PASSWORD)
    scp = SerialSession("SCP", serial_cmds['SCP'])
    ap_ns = SerialSession("AP_NS", serial_cmds['AP_NS'])
    
    # Open sessions
    bmc.open()
    scp.open()
    ap_ns.open()
    
    # Ensure BMC is root for power commands
    bmc.become_root()
    
    cycle = 1
    stop_scp_monitor = threading.Event()
    scp_monitor_thread = None
    
    try:
        while True:
            # Reset cycle state
            SAC_FOUND = False
            with EXIT_LOCK:
                SHOULD_EXIT = False
            stop_scp_monitor.clear()
            
            current_runtime = calculate_runtime()
            cycle_msg = f"\n{'='*60}\n=== CYCLE {cycle} START === (Runtime: {current_runtime}, Power cycles: {POWER_CYCLE_COUNT}, Errors: {SCP_ERROR_COUNT})\n{'='*60}\n"
            log_step(cycle_msg)
            write_to_log(BMC_LOG_PATH, cycle_msg)
            write_to_log(SCP_LOG_PATH, cycle_msg)
            write_to_log(AP_NS_LOG_PATH, cycle_msg)
            
            # STEP 1: Send power cycle on BMC
            execute_power_cycle(bmc)
            drain_connection(bmc, BMC_LOG_PATH, duration=3, prefix="BMC")
            
            # STEP 2: Start SCP monitoring thread (parallel monitoring)
            log_step(f"[PARALLEL] Starting SCP monitoring while waiting for AP_NS SAC>")
            stop_scp_monitor.clear()
            scp_monitor_thread = threading.Thread(
                target=monitor_scp_thread,
                args=(scp, stop_scp_monitor),
                daemon=True
            )
            scp_monitor_thread.start()
            
            # STEP 3: Wait for SAC> on AP_NS (while SCP monitoring runs in parallel)
            sac_found = wait_for_sac_prompt(ap_ns, timeout=600)  # 10 minute timeout
            
            # Check if SCP threshold was reached BEFORE SAC> appeared
            with EXIT_LOCK:
                if SHOULD_EXIT:
                    stop_scp_monitor.set()
                    log_step(f"🎯 SCP threshold exceeded BEFORE SAC> appeared!")
                    
                    # Write success to logs
                    success_msg = f"\n{'='*60}\n"
                    success_msg += f"SUCCESS: SCP threshold exceeded BEFORE SAC>\n"
                    success_msg += f"Power cycles: {POWER_CYCLE_COUNT}\n"
                    success_msg += f"SCP errors: {SCP_ERROR_COUNT}\n"
                    success_msg += f"Runtime: {calculate_runtime()}\n"
                    success_msg += f"{'='*60}\n"
                    write_to_log(BMC_LOG_PATH, success_msg)
                    write_to_log(SCP_LOG_PATH, success_msg)
                    write_to_log(AP_NS_LOG_PATH, success_msg)
                    
                    print_final_stats(success=True)
                    break
            
            if not sac_found:
                # SAC> didn't appear, continue to next cycle
                stop_scp_monitor.set()
                log_step(f"[CYCLE {cycle}] SAC> not found, continuing to next power cycle")
                cycle += 1
                continue
            
            # STEP 4: SAC> appeared - stop parallel monitor, start 3-minute timed monitor
            stop_scp_monitor.set()
            if scp_monitor_thread:
                scp_monitor_thread.join(timeout=2)
            
            log_step(f"[CYCLE {cycle}] SAC> found! Starting 3-minute post-SAC> monitoring...")
            drain_connection(ap_ns, AP_NS_LOG_PATH, duration=1, prefix="AP_NS")
            
            # STEP 5: Monitor SCP for exactly 3 minutes after SAC> appears
            threshold_reached = monitor_scp_for_duration(scp, duration_seconds=180)
            
            with SCP_COUNT_LOCK:
                if threshold_reached or SCP_ERROR_COUNT > SCP_ERROR_THRESHOLD:
                    log_step(f"🎯 SCP threshold exceeded during 3-minute post-SAC> monitoring!")
                    
                    # Write success to logs
                    success_msg = f"\n{'='*60}\n"
                    success_msg += f"SUCCESS: SCP threshold exceeded during post-SAC> monitoring\n"
                    success_msg += f"Power cycles: {POWER_CYCLE_COUNT}\n"
                    success_msg += f"SCP errors: {SCP_ERROR_COUNT}\n"
                    success_msg += f"Runtime: {calculate_runtime()}\n"
                    success_msg += f"{'='*60}\n"
                    write_to_log(BMC_LOG_PATH, success_msg)
                    write_to_log(SCP_LOG_PATH, success_msg)
                    write_to_log(AP_NS_LOG_PATH, success_msg)
                    
                    print_final_stats(success=True)
                    break
                else:
                    log_step(f"[CYCLE {cycle}] 3-minute monitoring complete. Errors: {SCP_ERROR_COUNT}. Starting new power cycle...")
            
            cycle_end_msg = f"\n=== CYCLE {cycle} END === (SCP Power Errors: {SCP_ERROR_COUNT})\n"
            log_step(cycle_end_msg)
            write_to_log(BMC_LOG_PATH, cycle_end_msg)
            write_to_log(SCP_LOG_PATH, cycle_end_msg)
            write_to_log(AP_NS_LOG_PATH, cycle_end_msg)
            
            cycle += 1
            
    except KeyboardInterrupt:
        final_runtime = calculate_runtime()
        log_step(f"⚠ Interrupted by user after {final_runtime}")
        print_final_stats(success=False)
    finally:
        stop_scp_monitor.set()
        bmc.close()
        scp.close()
        ap_ns.close()
    
    log_step(f"🏁 All serial sessions closed.")


if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    try:
        main()
    except Exception as exc:
        final_runtime = calculate_runtime()
        print(f"❌ Failure after {final_runtime} runtime: {exc}", file=sys.stderr)
        print(f"📊 Power cycles: {POWER_CYCLE_COUNT}, SCP errors: {SCP_ERROR_COUNT}", file=sys.stderr)
        sys.exit(1)
