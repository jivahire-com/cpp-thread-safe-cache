# Copyright (c) Microsoft Corporation. All rights reserved.

from typing import Dict, Optional, Tuple
import logging
from pathlib import Path
import os, sys
from enum import Enum

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([pylibs_dir, current_dir])
sys.path.append(str(Path(__file__).parent.parent / 'kng_pythia_libs'))

try:
    from .dcp_commands import DcpResponse, DataCollectionProtocol
except ImportError:
    from dcp_commands import DcpResponse, DataCollectionProtocol

logger = logging.getLogger(__name__)

# Constants for die and CPU identifiers
DIE_0 = 'die0'
CPU_AP = 'ap'
CPU_MCP = 'mcp'
CPU_SCP = 'scp'

class TransportType(Enum):
    """Transport types supported by the interface"""
    ICC_MHU = 'icc_mhu'
    MAILBOX = 'mailbox'
    HW_REGS = 'hw_regs'

# Keeping this for reference, we are not using it currently.
# class TransportConfig:
#     """Transport configuration container"""
#     CONFIGS = {
#         TransportType.ICC_MHU: {
#             'die0_to_mcp': {
#                 'uart': 'mcp',
#                 'channel': 4,
#                 'command_code': 0x20001
#             }
#         },
#         TransportType.MAILBOX: {
#             'die0_to_mcp': {
#                 'uart': 'scp',
#                 'channel': 2,
#                 'command_code': 0x10001
#             }
#         }
#     }

# Transport configurations
TRANSPORT_CONFIG = {
    'dcp': {
        'uart': 'mcp',
        'channel': 4,
        'command_code': 0x20001
    },
    'icc_mhu': {
        'uart': 'mcp',
        'channel': 4,
        'command_code': 0x20001
    },
    #dummy transport for mailbox
    'mailbox': {
        'uart': 'scp',
        'channel': 2,
        'command_code': 0x10001
    }
}

class BaseTransport:
    """Base class for all transport implementations"""
    def __init__(self, source_die: str, source_cpu: str, config: dict):
        self.source_die = source_die
        self.source_cpu = source_cpu
        self.config = config

    def format_command(self, msg: str) -> str:
        """Format the command for this transport type"""
        raise NotImplementedError("Subclasses must implement format_command")

    def send_dcp_msg(self, target_die: str, target_cpu: str, msg: str) -> str:
        """Send DCP message through configured transport
        
        Args:
            target_die: Target die identifier (e.g., DIE_0)
            target_cpu: Target CPU identifier (e.g., CPU_MCP)
            msg: Formatted message to send
            
        Returns:
            str: Formatted command ready to be sent
        """
        try:
            # Validate DCP path (must be die0/AP -> die0/MCP)
            if not (self.source_die == DIE_0 and self.source_cpu == CPU_AP and 
                   target_die == DIE_0 and target_cpu == CPU_MCP):
                raise ValueError("DCP messages must be from die0/AP to die0/MCP")

            return self.format_command(msg)

        except Exception as e:
            logger.error(f"Error sending DCP message: {e}")
            raise

class IccMhuTransport(BaseTransport):
    """ICC MHU transport implementation"""
    def format_command(self, msg: str) -> str:
        """Format command for ICC MHU transport"""
        # clean up the message
        msg = msg.strip("'\"")
        
        command = (f"icc_mhu Send {self.config['channel']} "
                  f"0x{self.config['command_code']:x} "
                  f"0x{len(msg.split()):02x} {msg}")
        logger.debug(f"Command via {self.config['uart']}: {command}")
        return command

#dummy for mailbox
class MailboxTransport(BaseTransport):
    """Mailbox transport implementation"""
    def format_command(self, msg: str) -> str:
        """Format command for Mailbox transport"""
        command = (f"mailbox Send {self.config['channel']} "
                  f"0x{self.config['command_code']:x} {msg}")
        logger.debug(f"Command via {self.config['uart']}: {command}")
        return command

# Implementation specific to HW registers
class HwRegsTransport(BaseTransport):
    """Hardware registers transport implementation"""
    def format_command(self, msg: str) -> str:
        """Format command for HW registers transport"""
        raise NotImplementedError("HW registers transport not yet implemented")

def get_transport(source_die: str, source_cpu: str, config_type: str = 'dcp') -> BaseTransport:
    """Factory function to create appropriate transport instance"""
    if config_type not in TRANSPORT_CONFIG:
        raise ValueError(f"Unknown transport config type: {config_type}")
    
    config = TRANSPORT_CONFIG[config_type]
    
    # Select transport class based on config type
    if config_type == 'dcp' or config_type == 'icc_mhu':
        return IccMhuTransport(source_die, source_cpu, config)
    elif config_type == 'mailbox':
        return MailboxTransport(source_die, source_cpu, config)
    elif config_type == 'hw_regs':
        return HwRegsTransport(source_die, source_cpu, config)
    else:
        raise ValueError(f"Unsupported transport type: {config_type}")