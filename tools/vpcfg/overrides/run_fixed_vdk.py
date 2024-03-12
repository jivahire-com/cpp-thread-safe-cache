"""Helper script for running SVP VDKs. Depends on the
RunSvp Python Library"""
import os
import sys

try:
  __file__
except:
  import inspect
  __file__ = inspect.getfile(inspect.currentframe())

sys.path.append(os.path.dirname(os.path.realpath(__file__)))
from RunSvp import *

def main():
    """Entry to run_fixed_vdk.py"""
    args = SvpParser("run_fixed_vdk.py").get_args(sys.argv)

    SvpUtil.check_for_virtualizer_studio_shell()

    if args.central_vdk_dir:
        svp_vdk = SvpCentralVdk(args)
    else:
        svp_vdk = SvpFixedVdk(args)

    vp_config = svp_vdk.create_vp_config()

    if not args.nosave:
        SvpUtil.save_vpconfig(vp_config)

    if not args.norun:
        svp_vdk.run(vp_config)

main()
