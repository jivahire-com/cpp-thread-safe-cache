#!/usr/bin/env bash
#
# Copyright (C) Microsoft Corporation. All rights reserved.
#
RESETMON=oob_reset_monitor
VERSION=0.0.1

usage() {
  cat <<EOUSAGE
Usage: $0 [OPTIONS] PATH
This script monitors a directory for and file modification and responds by
executing a set of specific commands to force an out of band (OOB) big FPGA
platform level reset.

The monitor checks every -s seconds (2 by default) and, if triggered, executes
the reset command.  This supports regression testing by issuing on-demand resets
based on file creation events.

NOTE:
The loops run forever! Exiting this script requires Ctrl-C. Typically this
script is expected to be run in the background using utilities like nohup.

OPTIONS

  -h           | --help              display this help and exit
  -v           | --version           output version information and exit
  -n           | --dryrun            execute the monitoring but only echo command
  -s <seconds> | --sleep <seconds>   sleep between checks (default: 2)

POSITIONAL ARGUMENTS

  PATH         path to monitor

EOUSAGE
  exit "${1:-0}"
}

version() {
  cat <<EOVERSION
$RESETMON $VERSION
EOVERSION
  exit "${1:-0}"
}

# defaults
DRYRUN=false
DELAY=2
SYSTEM_NAME=default
MONITOR_PATH=force_reset_dh5

# process command line
while [[ $# -gt 0 ]]; do
  case "$1" in

    -h | --help)
      usage
      ;;

    -v | --version)
      version
      ;;

    -n | --dryrun)
      DRYRUN=true
      shift
      ;;

    -s | --seconds)
      DELAY="$3"
      shift
      ;;

    -m | --system_name)
      SYSTEM_NAME="$2"
      shift
      ;;

    *)
      MONITOR_PATH="$1"
      ;;
  esac
  shift
done

#
# Source FPGA tools
#
cd /home/rdu_lab/haps_runtime/kingsgate_sd/OOB_reset
source /home/continuous_integration/release/enviro/etc/enviro_startup.bash
module load settings/tsd

echo "Monitoring ${MONITOR_PATH} ..."

echo "System Name ${SYSTEM_NAME} ..."

if [ "${SYSTEM_NAME}" == "default" ]; then
    echo "Invalid System Name. Exiting..."
    exit "${1:-0}"
fi

while true; do
  touch -m ${MONITOR_PATH}/.newer
  while true; do
    sleep $DELAY

    # capture output and if something new is found, break to force an oob reset
    list=$(find ${MONITOR_PATH} -type f -newer ${MONITOR_PATH}/.newer)
    if [ ! -z "$list" ]; then break; fi
  done

  if $DRYRUN; then
    echo "reset ${SYSTEM_NAME}"
  else
    echo $(date "+%D-%T: OOB reset requested for ${SYSTEM_NAME}")
    if [ "${SYSTEM_NAME}" == "df2" ]; then
      ./proto_hw reset F2_F3
    elif [ "${SYSTEM_NAME}" == "df5" ]; then
      ./proto_hw reset F5_F6
    elif [ "${SYSTEM_NAME}" == "df8" ]; then
      ./proto_hw reset F8_F9
    elif [ "${SYSTEM_NAME}" == "df11" ]; then
      ./proto_hw reset F11_F12
    elif [ "${SYSTEM_NAME}" == "df14" ]; then
      ./proto_hw reset F14_F15
    else
      ./proto_hw reset
    fi
  fi
done