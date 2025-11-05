#include <atu_lib.h>
#include <ddrss_runtime_api.h>
#include <cper.h>
#include <idsw_kng.h>
#include <smbios_structs.h>
#include <stdint.h>



extern "C"{
extern bool in_setup_teardown;
extern bool g_should_wrap_idsw_get_platform_sdv;
extern bool g_should_wrap_ddr_create_memory_map;
extern bool g_should_wrap_ddrss_get_ddrss_mask;
extern bool g_check_smb17_params;

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry);
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry);
uint8_t __wrap_mmio_read8(volatile uint8_t* addr);
uint16_t __wrap_mmio_read16(volatile uint16_t* addr);
uint32_t __wrap_mmio_read32(void* addr);
void __wrap_mmio_write8(void* addr, uint8_t data);
void __wrap_mmio_write16(volatile uint16_t* addr, uint16_t data);
void __wrap_mmio_write32(void* addr, uint32_t data);
bool __wrap_config_get_borgens_1gb_ddr_reserve_enable();
bool __wrap_config_get_ddrmanager_bwl_polling_en();
bool __wrap_config_get_ras_init_en();
uint16_t __wrap_ddrss_get_ddrss_mask(void);
uint16_t __real_ddrss_get_ddrss_mask(void);
ddrss_phy_training_dq_margin_t* __wrap_ddrss_get_training_margin_base();
KNG_DIE_ID __wrap_idsw_get_die_id();
KNG_PLAT_ID __wrap_idsw_get_platform_sdv();
int __wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable(uint32_t mc, DDRSS_RAS_NODE_ID erg_id,  bool *enable);
int __wrap_prod_ddrss_set_ras_erg_ce_interrupt_enable(uint32_t mc, DDRSS_RAS_NODE_ID erg_id,  bool enable);

int __wrap_prod_ddrss_get_intr_event_cper(uint32_t mc, uint32_t intr_event, acpi_err_sec_mem_vendor_t* ddr_cper);
void __wrap_hm_submit_cper(uint16_t error_domain_idx, acpi_error_severity_t err_severity, void* err_record_section, uint32_t err_record_section_size);
int __wrap_ddrss_bandwidth_limiter_config(uint32_t mc, bool enable, uint32_t max_acc_cost, uint32_t rd_wr_cost);
void __wrap_ddr_create_memory_map();
void __real_ddr_create_memory_map();

bool __wrap_FPFwCoreInterruptIsEnabled(uint32_t irqnum);
uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t irqnum);
uint32_t __wrap_FPFwCoreInterruptDisableVector(uint32_t irqnum);

void __wrap_copy_single_smbios_type_17(uint8_t* dest_addr, SMBIOS_MEM_DEVICE_17* src_table17);
}