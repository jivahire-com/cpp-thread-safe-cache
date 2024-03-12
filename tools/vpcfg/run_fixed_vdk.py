import argparse
import os
import sys
from datetime import datetime

# Global variables for parameters
args = None

def get_module_path(parameter_full_path):
    return parameter_full_path.split("#")[0][:-1]

def get_set_name(parameter_full_path):
    return parameter_full_path.split("#")[1]

def get_param_path(parameter_full_path):
    return parameter_full_path.split("#")[2]

def set_parameter_value(vp_config, parameter_full_path, new_value):
    module_path = get_module_path(parameter_full_path)
    set_name = get_set_name(parameter_full_path)
    param_path = get_param_path(parameter_full_path)
    vp_config.set_parameter_value_override(module_path, set_name, param_path, new_value)

def get_parameter_value(vp_config, parameter_full_path):
    module_path = get_module_path(parameter_full_path)
    set_name = get_set_name(parameter_full_path)
    param_path = get_param_path(parameter_full_path)
    return str(vp_config.get_parameter_value(module_path, set_name, param_path))

def get_parameter_description(vp_config, parameter_full_path):
    module_path = get_module_path(parameter_full_path)
    set_name = get_set_name(parameter_full_path)
    param_path = get_param_path(parameter_full_path)
    desc = ""
    try:
        desc = str(vp_config.get_parameter_description(module_path, set_name, param_path))
    except:
        return ""
    return desc

def list_parameters(vp_config, template_name):
    vp_config.reset()
    pattern = "*" + template_name + "*"
    vp_config.generate_properties_xml()
    parameters = vp_config.find_parameter_names(pattern=pattern, fullPath=True)
    for param in parameters:
        print(param + "=" + get_parameter_value(vp_config, param) + 
              "\n\tDescription: " + get_parameter_description(vp_config, param))
        
def run_vp_config(vp_config):
    print("\nRunning vpconfig " + vp_config.get_path() + "...\n")

    try:
        if (args.timeout != None):
            vp_config.run(timeout=args.timeout)
        else:
            vp_config.run()
    except RuntimeError as e:
        if hasattr(e, 'message'):
            print(e.message)
        else:
            print(e)

# create the vdk in the current workspace, and then runs it
def create_and_run_vdk():
    if ("SNPS_VS_VDK_SEARCH_PATHS" not in os.environ):
            print("SNPS_VS_VDK_SEARCH_PATHS env variable not set, set it to point to the directory"
                + " containing the vdkzip")
            exit(1)

    if (args.template_name == None):
        for template in vdkcreator.get_templates():
            if ("SVP" in template.get_name()):
                args.template_name = template.get_name()
    
    fixed_vdk_name = args.template_name

    if (fixed_vdk_name in vdkcreator.get_vdk_project_names()):
        fixed_vdk = vdkcreator.load(fixed_vdk_name)
    else:
        fixed_vdk = vdkcreator.create_fixed_vdk(name=fixed_vdk_name, template=args.template_name)

    if (not args.run_last_config):
        ts = datetime.timestamp(datetime.now())
        vp_config = fixed_vdk.create_vp_config("SVPConfig-" + str(args.vpcfg_name))
        vp_config.set_parent(fixed_vdk.get_vp_config("default").get_path())

        if (args.list_params):
            list_parameters(vp_config, args.template_name)
            exit(0)

        if (args.paramsfile != None):
            with open(args.paramsfile[0], 'r') as file:
                paramsFileDump = file.read()
            
            paramsFileExtract = paramsFileDump.split('\n')

            for param in paramsFileExtract:
                split_param = param.split("=", 1)
                set_parameter_value(vp_config, split_param[0], split_param[1])

        if (args.parameter != None):
            for param in args.parameter:
                split_param = param.split("=", 1)
                set_parameter_value(vp_config, split_param[0], split_param[1])

        vp_config.set_auto_continue_initial_crunch(args.debug)
        vp_config.set_data_output_dir(args.output_dir)
        vp_config.set_log_sim_output_to_file(args.log_sim_output_to_file)
        vp_config.set_sim_output_file(args.sim_output_file)
        fixed_vdk.set_active_vp_config(vp_config)
        vp_config.save()
        run_vp_config(vp_config)

def main(argv):

    parser = argparse.ArgumentParser(prog="run_fixed_vdk.py", description="Helper script to run fixed VDKs")
    parser.add_argument("--template_name", type=str, help="The name of the fixed vdk to run")
    parser.add_argument("--list_params", action='store_true', help="Specify if you wish to list available sim params and exit")
    parser.add_argument("--parameter", action='append', help="Simulation parameter, can be specified multiple times. Ex param=value")
    parser.add_argument("--paramsfile", action='append', help="File with simulation parameters, can be specified once.")
    parser.add_argument("--debug", action='store_false', help="Specify if you wish to launch GDB server for processors")
    parser.add_argument("--timeout", type=int, help="Sets the sim timeout in seconds")
    parser.add_argument("--vpcfg_name", type=str, help="Sets the name of the vpcfg")
    parser.add_argument("--output_dir", type=str, help="Sets the simulation output directory, default is working directory")
    parser.add_argument("--run_last_config", action='store_true', help="Will run the sim with the last parameters set")
    parser.add_argument("--log_sim_output_to_file", action='store_true', help="Will log sim output to simout.txt or what is specified by --sim_output_file")
    parser.add_argument("--sim_output_file", type=str, help="Path to the simulation output file")

    if (argv[0] != ''):
        global args
        args = parser.parse_args(argv)

    if (args.output_dir == None):
        args.output_dir = os.getcwd()

    create_and_run_vdk()

main(sys.argv)