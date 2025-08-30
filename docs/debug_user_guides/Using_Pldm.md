# How to use Pldm

This guide will show how to define PLDM components, and how to interact with the PLDM service from the
attached BMC.

[[_TOC_]]

## Defining Pldm components (sensor/effecters)

### Creating a Platform Description Record entry

In order to define a Pldm component one has to define what the component is as PDR (Platform Description Record). PDR
definition can be found in the [public specification](https://www.dmtf.org/sites/default/files/standards/documents/DSP0248_1.3.0.pdf)
in the following sections accordingly:

* Numeric Sensor PDR: Section 28.4
* State Sensor PDR: Section 28.6
* Numeric Effecter PDR: Section 28.11
* State Effecter PDR: Section 28.13
* Sensor Auxiliary Names PDR: Section 28.8
* Effecter Auxiliary Names PDR: Section 28.15

The PDR for this project is static, and defined at build time. Its definition can be found in: `<repo_root>\src\libs\runtime_init\pdr_repo_init.c`.

In order to create a new PDR entry, the following is needed:

1. Allocate a sensor/effecter id from the `PLDM_PDR_SENSOR_ID` or `PLDM_PDR_EFFECTER_ID` enum in `<repo_root>\src\libs\pldm_pdr\inc\pldm_pdr.h`.

    >**NOTE:** Each core has an allocated range.

2. Choose a PDR `struct` based on the required data types of the PDR definition. The PDR structure for the four different components (numeric/state sensor and effecter) may have different sizes based on the configuration of the component. In order to initialize statically the different size combinations are represented as different `structs`. Here is how to pick for each component:

    * Numeric Sensor PDR:
        `fpfw_pldm_pdr_numeric_sensor_<pdr.sensor_data_size>_<pdr.range_field_format>`
        For example `fpfw_pldm_pdr_numeric_sensor_UINT32_REAL32`. (See description of the PDR for selecting these fields)
    * State Sensor PDR:
        `fpfw_pldm_pdr_state_sensor_COMPOSITE_<pdr.composite_sensor_count>_STATES_<pdr.possible_states_size>`
        For example `fpfw_pldm_pdr_state_sensor_COMPOSITE_3_STATES_4`. (See description of the PDR for selecting these fields)
    * Numeric Effecter PDR:
        `fpfw_pldm_pdr_numeric_effecter_<pdr.effecter_data_size>_<pdr.range_field_format>`
        For example `fpfw_pldm_pdr_numeric_effecter_UINT32_REAL32`. (See description of the PDR for selecting these fields),
    * State Effecter PDR:
        `fpfw_pldm_pdr_state_effecter_COMPOSITE_<pdr.composite_effecter_count>_STATES_<pdr.possible_states_size>`
        For example `fpfw_pldm_pdr_state_effecter_COMPOSITE_3_STATES_4`. (See description of the PDR for selecting these fields)
    * Sensor Auxiliary Names PDR:
        `fpfw_pldm_pdr_sensor_auxiliary_name_t`
    * Effecter Auxiliary Names PDR:
        `fpfw_pldm_pdr_effecter_auxiliary_name_t`

3. Create your entry in `<repo_root>\src\libs\runtime_init\pdr_repo_init.c` as a static variable.

4. Fill the different members. Please refer to the PDR definition in the [public specification](https://www.dmtf.org/sites/default/files/standards/documents/DSP0248_1.3.0.pdf) as mentioned before. Some of the fields will need predefined values, these are:

    ```c
        .hdr.record_handle = 0, // Populated by PLDM lib
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_STATE_SENSOR_PDR, // Should match the PDR you are creating
        .hdr.length = 
            sizeof(fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t) - sizeof(fpfw_pmc_pdr_header_t), // Should match the struct you
                                                                                                // are using
        .hdr.record_change_num = 0,
    ```

5. Add your PDR entry into the `s_pdr_list` variable at the end of the `<repo_root>\src\libs\runtime_init\pdr_repo_init.c` file. See the example below.
NOTE: We not support names associated with the PDR entry to not have a static entry on the BMC, hence you must set the field `auxiliar_init_pdr` to `true`.

    ```c
    static fpfw_pldm_pdr_sensor_auxiliary_name_t s_dummy0_name = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_SENSOR_AUXILIARY_NAMES_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_sensor_auxiliary_name_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_0,
        .sensor_count = 1,
        .name_string_count = 0x01,
        .name_language_tag = "en", // Language tag for the name. 3 bytes, null terminated. "en" = English
    .sensor_name = L"Dummy_State_Sensor_0" // Sensor name in UTF-16 format
    };

    static fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t s_dummy0 = {
        .hdr.version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION,
        .hdr.type = PLDM_STATE_SENSOR_PDR,
        .hdr.length = sizeof(fpfw_pldm_pdr_state_sensor_COMPOSITE_1_STATES_1_t) - sizeof(fpfw_pmc_pdr_header_t),
        .hdr.record_change_num = 0,

        .sensor_id = PLDM_SENSOR_ID_MCP_DUMMY_0, // Unique id across sensors. Used for registration in the PldmService API

        .entity_type = PLDM_ENTITY_PROC_MODULE, // Entity for which this sensor is associated. Section 7.1 https://www.dmtf.org/sites/default/files/standards/documents/DSP0249_1.0.0.pdf
        .entity_instance = 0, // Instance id in case there are multiple entities of the same time in the same container
        .container_id = 0,           // 0 = SYSTEM, related to entity associations in DSP0248_1.2.1 (Section 28)
        .sensor_init = PLDM_NO_INIT, // Only no-init/enable supported
        .auxiliar_init_pdr = true,  // Required if sensor has an auxiliary name
        .composite_sensor_count = 1,
        .possible_states = {
            {.state_set_id = PLDM_STATE_SET_AVAILABILITY,
                .possible_states_size = 1,
                .possible_states = {
                   PLDM_STATE_SET_AVAILABILITY_REBOOTING,
                }
            }
        }
    };

    ...

    static void* s_pdr_list[] = {
        &s_dummy0,
        &s_dummy0_name,
        NULL};
    ```

6. For the Auxiliary Names PDR, note that the sensor name is in UTF-16 format and the data type is wchar_t. So it must be stored as shown below

    ```c
    .sensor_name = L"Dummy_State_Sensor_0"
    ```

For all other steps, refer to the document here- [PldmService.md](https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.fwlibs?path=/doc/Modules/services/PldmService.md&_a=preview&anchor=api)