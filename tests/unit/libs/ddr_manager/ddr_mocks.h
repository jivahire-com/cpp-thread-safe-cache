#include <atu_lib.h>
#include <idsw_kng.h>
#include <stdint.h>



extern "C"{
extern bool in_setup_teardown;
int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry);
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry);
uint8_t __wrap_mmio_read8(volatile uint8_t* addr);
uint16_t __wrap_mmio_read16(volatile uint16_t* addr);
uint32_t __wrap_mmio_read32(void* addr);
void __wrap_mmio_write8(void* addr, uint8_t data);
void __wrap_mmio_write16(volatile uint16_t* addr, uint16_t data);
void __wrap_mmio_write32(void* addr, uint32_t data);
bool __wrap_ddr_manager_platform_is_polling_supported();
bool __wrap_config_get_borgens_1gb_ddr_reserve_enable();
KNG_DIE_ID __wrap_idsw_get_die_id();
}