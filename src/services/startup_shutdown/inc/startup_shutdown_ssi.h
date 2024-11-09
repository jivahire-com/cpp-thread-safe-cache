// Copyright Microsoft. All rights reserved.

/**
 * @file startup_shutdown_ssi.h
 * This file contains the interface to register to the startup/shutdown service for participation in sequencing.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkCommon.h>
#include <stdint.h>


/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/**
 *  @brief Synchonous and Asynchronous Interface Request IDs
 */
enum SSI_REQUEST_IDS
{
    SSI_START_ID = 0xFF00, /* allow for possibility of only one interface registration for owning driver/service */
    SSI_STARTUP_STAGE_START_ASYNC,
    SSI_STARTUP_STAGE_COMPLETE_ASYNC, /* all start actions are complete */
    SSI_SHUTDOWN_QUIESCE_ASYNC,
};

/**
 *  @brief Startup boot stages
 *
 *    Note the inclusion of phases; while the values only need to be unique and order here is not important,
 * ideally these should be kept somewhat in order to avoid any unnecessary confusion.  Stages will be a
 * part of some phase, and drivers dependent on startup notification can expect start/complete of the phase
 * enveloping all of the start/complete of individual boot stages.
 * 
 */
typedef enum _ssi_startup_stage
{
    STARTUP_PHASE_MSCP_ASYNC = 0x0, /* Phase included in cold boot and MSCP subsystem reset */
    STARTUP_MCP_LOAD,
    STARTUP_KMP_LOAD,
    STARTUP_CLUSTER_CORE_INIT,
    STARTUP_AP_SOC_POWER_INIT,
    STARTUP_AP_SOC_POWER_INIT_POST_SYNC,
    STARTUP_PHASE_AP_ASYNC = 0x100, /* Phase included in cold boot and AP warm reset */
    STARTUP_BL31_LOAD,
    STARTUP_STMM_LOAD,
    STARTUP_BL33_LOAD,
    STARTUP_HAFNIUM_LOAD,
    STARTUP_RMM_LOAD,
    STARTUP_SPMC_LOAD,
    STARTUP_PRIMARY_AP_CORE_BOOT
} ssi_startup_stage_t;

/**
 *  @brief Startup type
 *
 *    Startup/shutdown module will determine the type of startup being performed and provide this through the
 * interface so that registered drivers do not have to also query in order to make behavior changes.
 *
 */
typedef enum _startup_type
{
    COLD_BOOT,
    WARM_BOOT_PRE_AP,
    WARM_BOOT_POST_AP,
} ssi_startup_type_t;

/**
 *  @brief Shutdown type
 *
 *    Startup/shutdown module will provide a shutdown type through the interface so that registered drivers do
 * not have can determine based on the type of shutdown if they have requirements to quiesce.
 *
 */
typedef enum _shutdown_type
{
    SHUTDOWN,
    COLD_RESET,
    MSCP_SUBSYS_RESET,
    AP_WARM_RESET, /* Drivers should ignore unless they have specific requirements around AP warm reset */
} ssi_shutdown_type_t;

/**
 * @brief Request structures
 */
typedef struct _ssi_startup_notification_request_t
{
    DFWK_ASYNC_REQUEST_HEADER header;
    ssi_startup_stage_t stage;
    ssi_startup_type_t boot_type;
} ssi_startup_notification_request_t, *pssi_startup_notification_request_t;

typedef struct _ssi_shutdown_notification_request_t
{
    DFWK_ASYNC_REQUEST_HEADER header;
    ssi_shutdown_type_t shutdown_type;
} ssi_shutdown_notification_request_t, *pssi_shutdown_notification_request_t;

// union of all async request structures
typedef union _ssi_request_t
{
    ssi_startup_notification_request_t startup_notification;
    ssi_shutdown_notification_request_t shutdown_notification;
} ssi_request_t, *pssi_request_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 *  Inline function to send an asynchronous startup stage begin request/notification
 *
 *  @param p_interface An interface to a driver that implements the startup/shutdown interface.
 *
 *  @param p_request A DFWK request with enough space for a ssi_request_t
 *
 *  @param stage The startup stage that is starting
 *
 *  @param boot_type The current boot type
 *
 *  @param completion_routine A completion routine to call when the operation has completed.
 *
 *  @param p_completion_context A context that is supplied when the completion routine is called.
 */
 void ssi_startup_stage_start(PDFWK_INTERFACE_HEADER p_interface,
                                           pssi_request_t p_request,
                                           ssi_startup_stage_t stage,
                                           ssi_startup_type_t boot_type,
                                           DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                           void* p_completion_context);

/**
 *  Inline function to send an asynchronous startup stage complete request/notification
 *
 *  @param p_interface An interface to a driver that implements the startup/shutdown interface.
 *
 *  @param p_request A DFWK request with enough space for a ssi_request_t
 *
 *  @param stage The startup stage that has completed
 *
 *  @param boot_type The current boot type
 *
 *  @param completion_routine A completion routine to call when the operation has completed.
 *
 *  @param p_completion_context A context that is supplied when the completion routine is called.
 */
 void ssi_startup_stage_complete(PDFWK_INTERFACE_HEADER p_interface,
                                              pssi_request_t p_request,
                                              ssi_startup_stage_t stage,
                                              ssi_startup_type_t boot_type,
                                              DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                              void* p_completion_context);

/**
 *  Inline function to send an asynchronous shutdown/quiesce request/notification
 *
 *  @param p_interface An interface to a driver that implements the startup/shutdown interface.
 *
 *  @param p_request A DFWK request with enough space for a ssi_request_t
 *
 *  @param shutdown_type The type of shutdown being performed
 *
 *  @param completion_routine A completion routine to call when the operation has completed.
 *
 *  @param p_completion_context A context that is supplied when the completion routine is called.
 */
 void ssi_shutdown_quiesce(PDFWK_INTERFACE_HEADER p_interface,
                                        pssi_request_t p_request,
                                        ssi_shutdown_type_t shutdown_type,
                                        DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                        void* p_completion_context);
