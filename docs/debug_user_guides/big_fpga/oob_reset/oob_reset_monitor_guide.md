# Running oob_reset_monitor on bigFPGA unix machines

This is a guide that describes the steps needed to run oob_reset_monitor on an FPGA lab setup.
The oob_reset_monitor monitors a directory for and file modification and responds by
executing a set of specific commands to force an out of band (OOB) big FPGA
platform level reset.

1. `ssh` into the Unix machine attached to the desired big FPGA setup.

1. Place the below oob_reset_monitor.sh script in a local directory (Example: /home/vamshi/oob_reset/).

1. Navigate to the directory containing `oob_reset_monitor.sh` script you have copied into the unix machine. (Example: /home/vamshi/oob_reset/)

1. Run the following commands. In this case the example is for setting it up on the DH3 bigFPGA system. update the commands if any of your parameters change.

```bash
# Kill any previously running instances of oob_reset_monitor.sh on the machine
ps -ef | grep "oob_reset_monitor"

# Sample output:
v-vamshis@rdu-120015-h3-1:~> ps -ef | grep "oob_reset_monitor"
v-vamshis+ 3131393       1  0 15:06 ?        00:00:00 bash ./oob_reset_monitor.sh /home/rdu_lab/haps_runtime/kingsgate_big_fpga/reset_tools/force_reset_dh3

# Grab the pid from the previous output, in this example it is 3131393
# kill the old process by running:
kill 9 3131393

#
# Now, start a fresh run
#
# nohup           - keeps the process running even if you kill your ssh session
# bash            - as the default shell is zsh and we want to use bash for this
#                    script, invoke it explicitly
# oob_reset_monitor.sh - Invoked with the proper <machine_name>, in this example dh3
# >               - Redirect logs from oob_reset_monitor.sh to any directory of your
#                   choice
#
nohup bash ./oob_reset_monitor.sh /home/rdu_lab/haps_runtime/kingsgate_big_fpga/reset_tools/force_reset_dh3 -m dh3 > ~/work/nohup_dh3.out &

# Make sure oob_reset_monitor is up and going
ps -ef | grep "oob_reset_monitor"

# Exit the ssh session
exit

```

1. `oob_reset_monitor`in the example above watches `/home/rdu_lab/haps_runtime/kingsgate_big_fpga/reset_tools/force_reset_<machine_name>` for any file changes. This`/home/` maps to `lakshmi.svceng.com` on the FPGA PC.