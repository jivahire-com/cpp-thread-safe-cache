#include <stdint.h>



extern "C"{
extern bool in_setup_teardown;
uint32_t __wrap_mmio_read32(void* addr);
void __wrap_mmio_write32(void* addr, uint32_t data);
bool __wrap_ddr_manager_platform_is_polling_supported();
bool __wrap_config_get_borgens_1gb_ddr_reserve_enable();
}