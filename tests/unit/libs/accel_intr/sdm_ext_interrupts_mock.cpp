//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sdm_ext_intrrupts_mock.c
 * sdm_ext_interrupts MOCKS for unit testing. Only built for win32
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <mock.h>
#include <sdm_ext_interrupts.h> // for sdm_ext_int_t, sdm_ext_int_disable
#include <silibs_platform.h>    // for SILIBS_SUCCESS
#include <stdbool.h>            // for bool, true
#include <utils.h>              // for UNUSED

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

bool cur_int_status = false;

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
/**
 * @brief set_int_status - Function used by test to set expected interrupt status
 *
 *  @param[in]
 *      bool
 *
 *  @return
 *      void
 */
void set_int_status(bool new_int_status)
{
    cur_int_status = new_int_status;
}

/**
 * @brief sdm_ext_int_mask_disable - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 * @param[in] interrupt_mask : Interrupt Mask register  indicate bits to disable
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_mask_disable(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(category_id);
    FPFW_UNUSED(interrupt_mask);

    return mock_type(int);
}

/**
 * @brief sdm_ext_int_mask_status_clear - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 * @param[in] interrupt_mask : Interrupt Mask register indicate bits to clear
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_mask_status_clear(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(category_id);
    FPFW_UNUSED(interrupt_mask);

    return mock_type(int);
}

/**
 * @brief sdm_ext_int_mask_enable - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 * @param[in] interrupt_mask : Interrupt Mask register indicate bits to enable
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_mask_enable(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(category_id);
    FPFW_UNUSED(interrupt_mask);

    return mock_type(int);
}

/**
 * @brief is_sdm_ext_int_status_set - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] vector : Vector of interrupt to indicate interrupt to get status of
 * @param[in] int_status : Interrupt status value
 *
 *  @return
 *      int
 */
int __wrap_is_sdm_ext_int_status_set(uintptr_t ext_cfg_addr, sdm_ext_int_t vector, bool* int_status)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(vector);
    *int_status = cur_int_status;

    return mock_type(int);
}

/**
 * @brief is_sdm_ext_int_mask_status_clear - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] vector : Vector of interrupt to indicate interrupt to get status of
 * @param[in] int_status : Interrupt status value
 *
 *  @return
 *      int
 */
int __wrap_is_sdm_ext_int_mask_status_clear(uintptr_t ext_cfg_addr,
                                            SDM_EXT_INT_CATEGORY category_id,
                                            uint32_t interrupt_mask,
                                            bool* int_status)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(category_id);
    FPFW_UNUSED(interrupt_mask);

    *int_status = !cur_int_status;

    return mock_type(int);
}

/**
 * @brief is_sdm_ext_int_mask_status_clear - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 *
 *  @return
 *      int
 */
uint32_t __wrap_sdm_ext_get_category_status_reg_addr(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(category_id);

    return mock_type(uint32_t);
}

/**
 * @brief sdm_ext_get_category_mask_reg_addr - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 *
 *  @return
 *      int
 */
uint32_t __wrap_sdm_ext_get_category_mask_reg_addr(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(category_id);

    return mock_type(uint32_t);
}

/**
 * @brief sdm_ext_get_category_mask_reg_addr - Mock function
 *
 * @param[in] sub_system_inp : Subsystem : 0x0 HSP, 0x1 MCP, 0x2 SCP
 *
 *  @return
 *      int
 */
int __wrap_set_ext_int_sub_system(sdm_ext_subsystem_type_t sub_system_inp)
{
    if (sub_system_inp > SDM_EXT_MAX_SUBSYSTEM)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(uint32_t);
}

/**
 * @brief sdm_ext_int_disable - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] vector : Vector of interrupt to indicate interrupt to disable
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_disable(uintptr_t ext_cfg_addr, sdm_ext_int_t vector)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(vector);

    return mock_type(int);
}

/**
 * @brief sdm_ext_int_status_clear - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] vector : Vector of interrupt to indicate interrupt to clear
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_status_clear(uintptr_t ext_cfg_addr, sdm_ext_int_t vector)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(vector);

    return mock_type(int);
}

/**
 * @brief sdm_ext_int_enable - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] vector : Vector of interrupt to indicate interrupt to enable
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_enable(uintptr_t ext_cfg_addr, sdm_ext_int_t vector)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(vector);

    return mock_type(int);
}

} // extern "c"