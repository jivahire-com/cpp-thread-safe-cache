"""SvpVdk class for running creating and running VDKs"""
import os
from SvpUtil import SvpUtil

class SvpVdkException(Exception):
    """Standard SvpVdk Exception"""
    pass

class SvpVdk:
    """SvpVdk is a wrapper around the virtualizer vdk class that controls
    SVP specific creation of the vpconfigs from the provided SvpParser arguments"""
    def __init__(self, svp_args, vdk, vp_config_generator=None):
        self.svp_args = svp_args

        if self.svp_args.gui:
            exit(SvpUtil.launch_gui())

        self.vdk = vdk
        self.vp_config_generator = vdk

        if vp_config_generator is not None:
            self.vp_config_generator = vp_config_generator

    def debug(sim):
        """vpx callback used for holding the simulation
        until user input is received."""
        print("GDB Debug requested, waiting for user")
        # Python code to wait for user input
        while True:
            user_input = input("Type 'continue' or 'c' to proceed: ")
            if user_input.lower() in ["continue", "c"]:
                break
            else:
                print("Invalid input. Please type 'continue' or 'c'.")

    def set_vp_config_attributes(self, vp_config):
        """Sets the common SVP vpconfig attributes"""
        vp_config.set_parent(self.vdk.get_vp_config("default").get_path())

        SvpUtil.set_params(vp_config, self.svp_args.parameter)
        SvpUtil.set_env_vars(vp_config, self.svp_args.env)
        SvpUtil.set_max_instruction_count(vp_config, self.svp_args.max_instruction_count)
        SvpUtil.set_sim_probe(vp_config, self.svp_args.sim_probe)
        SvpUtil.set_primary_core(vp_config, self.svp_args.primary_core)

        default_sim_probe_script = os.path.abspath(os.path.join(os.path.dirname(__file__), '..',
                                                                'sim_probes', 'sim_probes.py'))
        SvpUtil.set_startup_script(vp_config, default_sim_probe_script)

        SvpUtil.set_vp_script(vp_config, self.svp_args.vpscript)

        vp_config.set_auto_continue_initial_crunch(True)
        vp_config.set_data_output_dir(self.svp_args.output_dir)
        vp_config.set_log_sim_output_to_file(self.svp_args.log_sim_output_to_file)
        vp_config.set_sim_output_file(self.svp_args.sim_output_file)
        self.vdk.set_active_vp_config(vp_config)

        return vp_config

    def create_vp_config(self):
        """Creates an SVP vpconfig"""
        if self.svp_args.run_last_config:
            return self.vp_config_generator.get_active_vp_config()

        vp_config = None
        vpconfig_name = self.svp_args.vpconfig
        if vpconfig_name is None:
            vp_config = self.vp_config_generator.create_vp_config("SVPConfig"
                                                                   + str(SvpUtil.get_timestamp()))
        else:
            try:
                vp_config = self.vp_config_generator.get_vp_config(vpconfig_name)
            except Exception:
                vp_config = self.vp_config_generator.create_vp_config(vpconfig_name)

        return self.set_vp_config_attributes(vp_config)

    def run_vp_config(self, vp_config):
        """Runs an SVP vpconfig"""
        print("\nRunning vpconfig " + vp_config.get_path() + "...\n")

        try:
            if self.svp_args.timeout is not None:
                vp_config.run(timeout=self.svp_args.timeout)
            else:
                vp_config.run()
        except RuntimeError as e:
            if hasattr(e, 'message'):
                print(e.message)
            else:
                print(e)

    def run(self, vp_config):
        """Performs some checks and vpx setup before running the SVP vpconfig"""
        if self.svp_args.list_params:
            SvpUtil.list_parameters(vp_config, self.vdk.get_name())
            return
      
        # if self.svp_args.debug:
        #     vpx.add_callback("connected", 'debug(__sim__)')

        self.run_vp_config(vp_config)

        # if self.svp_args.debug:
        #     vpx.remove_callback("connected", 'debug(__sim__)')

class SvpCentralVdk(SvpVdk):
    """Extension of SvpVdk for a centrally installed fixed vdk"""
    def __init__(self, svp_args):
        try:
            self.vp_config_project = vpconfigs.load("SVP")
        except Exception:
            self.vp_config_project = vpconfigs.create_project("SVP")

        super().__init__(svp_args,
                         vdkcreator.import_project(path=svp_args.central_vdk_dir, copy=False),
                         self.vp_config_project)

class SvpFixedVdk(SvpVdk):
    """Extension of SvpVdk for a non-centrally installed VDk"""
    def __init__(self, svp_args):  
        _vdk = None
        try:
            _vdk = vdkcreator.load(SvpUtil.get_template_name(svp_args))
        except Exception as e:
            if "SNPS_VS_VDK_SEARCH_PATHS" not in os.environ:
                raise SvpVdkException("SNPS_VS_VDK_SEARCH_PATHS env variable not set,"
                                    + " set it to point to the directory"
                                    + " containing the vdkzip") from e
            if not os.path.isdir(os.environ['SNPS_VS_VDK_SEARCH_PATHS']):
                raise SvpVdkException("SNPS_VS_VDK_SEARCH_PATHS is not a directory,"
                                    + " set it to point to the directory"
                                    + " containing the vdkzip") from e
            _vdk = SvpUtil.get_fixed_vdk(SvpUtil.get_template_name(svp_args))

        super().__init__(svp_args, _vdk)
