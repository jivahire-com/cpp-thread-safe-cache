"""Contains SvpUtil static helper class"""
from datetime import datetime
import subprocess
import os
import sys

class SvpUtil:
    """Contains static helper methods for creating and running SvpVdks"""
    @staticmethod
    def get_module_path(parameter_full_path):
        """Gets the module path"""
        return parameter_full_path.split("#")[0][:-1]

    @staticmethod
    def get_set_name(parameter_full_path):
        """Gets the set name"""
        return parameter_full_path.split("#")[1]

    @staticmethod
    def get_param_path(parameter_full_path):
        """Gets the parameter path"""
        return parameter_full_path.split("#")[2]

    @staticmethod
    def set_parameter_value(vp_config, parameter_full_path, new_value):
        """Sets the parameter value"""
        module_path = SvpUtil.get_module_path(parameter_full_path)
        set_name = SvpUtil.get_set_name(parameter_full_path)
        param_path = SvpUtil.get_param_path(parameter_full_path)
        vp_config.set_parameter_value_override(module_path, set_name, param_path, new_value)

    @staticmethod
    def get_parameter_value(vp_config, parameter_full_path):
        """Gets the parameter value"""
        module_path = SvpUtil.get_module_path(parameter_full_path)
        set_name = SvpUtil.get_set_name(parameter_full_path)
        param_path = SvpUtil.get_param_path(parameter_full_path)
        return str(vp_config.get_parameter_value(module_path, set_name, param_path))

    @staticmethod
    def get_parameter_description(vp_config, parameter_full_path):
        """Gets the parameter description"""
        module_path = SvpUtil.get_module_path(parameter_full_path)
        set_name = SvpUtil.get_set_name(parameter_full_path)
        param_path = SvpUtil.get_param_path(parameter_full_path)
        desc = ""
        try:
            desc = str(vp_config.get_parameter_description(module_path, set_name, param_path))
        except Exception:
            return ""
        return desc

    @staticmethod
    def list_parameters(vp_config, template_name):
        """List all the parameters of a vdk in context of the vpconfig"""
        vp_config.reset()
        pattern = "*" + template_name + "*"
        vp_config.generate_properties_xml()
        parameters = vp_config.find_parameter_names(pattern=pattern, fullPath=True)
        for param in parameters:
            print(param + "=" + SvpUtil.get_parameter_value(vp_config, param) + 
                "\n\tDescription: " + SvpUtil.get_parameter_description(vp_config, param))

    @staticmethod
    def set_params(vp_config, params):
        """Uses params to set the vpconfig parameters, expected format is param=val for each
        value in params"""
        if params is not None:
            for param in params:
                split_param = param.split("=", 1)
                SvpUtil.set_parameter_value(vp_config, split_param[0], split_param[1])

    @staticmethod
    def set_env_vars(vp_config, env_vars):
        """Uses env_vars to set the vpconfig environment variables, expected format
        is env_var=val"""
        if env_vars is not None:
            for var in env_vars:
                split_env = var.split("=", 1)
                vp_config.set_environment_variable(split_env[0], split_env[1])

    @staticmethod
    def set_max_instruction_count(vp_config, inst_count):
        """Sets the vp_config max instruction count"""
        if inst_count is not None:
            vp_config.set_environment_variable('MAX_CORE_INSTRUCTIONS', str(inst_count))

    @staticmethod
    def set_sim_probe(vp_config, sim_probe_script):
        """Sets vp_config sim probe"""
        if sim_probe_script is not None:
            vp_config.set_sim_args(f"--snps_startup_script {sim_probe_script}")

    @staticmethod
    def set_startup_script(vp_config, sim_startup):
        """Sets vp_config startup script"""
        if sim_startup is not None:
            vp_config.set_simulation_startup_script(sim_startup)

    @staticmethod
    def set_vp_script(vp_config, vp_script):
        """Sets vp_config vp script"""
        if vp_script is not None:
            vp_config.set_vpscript(vp_script)

    @staticmethod
    def set_primary_core(vp_config, primary_core):
        """Sets vp_config primary core"""
        if primary_core is not None:
            vp_config.set_environment_variable('PRIMARY_CORE', primary_core)

    @staticmethod
    def get_template_name(svp_args):
        """Searches workspace for template name if none was provided
        as argument and returns either the argument or the first template
        name in the workspace with SVP in it"""
        if svp_args.template_name is None:
            for template in vdkcreator.get_templates():
                if "SVP" in template.get_name():
                    return template.get_name()
        return svp_args.template_name

    @staticmethod
    def get_fixed_vdk(template_name):
        """Gets the fixed vdk that is in the workspace and returns it or
        creates one and returns it"""
        if template_name in vdkcreator.get_vdk_project_names():
            return vdkcreator.load(template_name)
        else:
            return vdkcreator.create_fixed_vdk(name=template_name, template=template_name)

    @staticmethod
    def get_timestamp():
        """Creates a timestamp"""
        return datetime.timestamp(datetime.now())

    @staticmethod
    def save_vpconfig(vp_config):
        """Saves the vp_config to disk"""
        vp_config.save()
    
    @staticmethod
    def launch_gui():
        """Launches the virtualizer GUI, function copies the current
        python environment. Runs in a detached process"""

        command = ['vs', '-d', eclipse.get_workspace_path()]
        try:
            env = os.environ.copy()
            result = subprocess.Popen(command,
                                    stdout=subprocess.DEVNULL,
                                    env=env,
                                    shell=True,
                                    start_new_session=True)
            if result.returncode is not None:
                print(result)
                return 1
        except subprocess.CalledProcessError as e:
                print(f'vs (GUI) failed with exit code {e.returncode}: {e.stderr}')
                return 1
        return 0
    
    @staticmethod
    def check_for_virtualizer_studio_shell():
        """Method provides a mechanism to check if a script is
        executing inside of Virtualizer Studio"""
        try:
            _ = vdkcreator
        except:
            print("\nThis script needs to be run in Virtualizer Studio (vssh or vs)!",
                  file=sys.stderr)
            exit(1)
