# DVFS ITD Voltage Band Normalization

## Document Information
- **Author**: Shreya Chakraborty
- **Date**: December 22, 2025
- **Purpose**: Explain the DVFS crawl hang prevention implementation for ITD temperature column changes

---

## Table of Contents
1. [Problem Statement](#problem-statement)
2. [Background: DVFS and VMAT](#background-dvfs-and-vmat)
3. [The Root Cause](#the-root-cause)
4. [Solution Overview](#solution-overview)
5. [Implementation Details](#implementation-details)
6. [Files Modified](#files-modified)
7. [Testing and Verification](#testing-and-verification)

---

## Problem Statement

### The Issue
When **ITD (In-Die Temperature)** monitoring is enabled, DVFS (Dynamic Voltage and Frequency Scaling) can experience a **crawl hang** during P-state transitions. This occurs when:

1. A DVFS transition is in progress (changing from one P-state to another)
2. The temperature changes mid-transition, causing ITD to switch temperature columns
3. The voltages in different temperature columns span different **mem-assist boundaries**
4. The hardware gets confused because the starting and ending mem-assist configurations don't match

### Impact
- CPU hangs during voltage/frequency transitions
- System becomes unresponsive
- Requires hardware reset to recover

---

## Background: DVFS and VMAT

### What is DVFS?
**Dynamic Voltage and Frequency Scaling (DVFS)** is a power management technique that adjusts:
- **Voltage**: Lower voltage = less power consumption, but slower operation
- **Frequency**: CPU clock speed

The CPU operates at different **P-states** (Performance states):
- **P0**: Highest performance (highest voltage, highest frequency)
- **P31**: Lowest performance (lowest voltage, lowest frequency)

### What is VMAT?
**VMAT (Voltage and Memory Assist Table)** is a hardware table that stores:
- Voltage settings for each P-state
- Memory assist (memasst) settings for each P-state

The VMAT is programmed during DVFS initialization and tells the hardware what voltage and memory timing parameters to use for each P-state.

### VMAT Structure

```c
// The VFT (Voltage/Frequency Table) contains VMAT info for each temperature column
typedef struct _dvfs_vft_t {
    dvfs_vmat_info_t vmat_info[NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS];  // 4 columns
    uint16_t itd_temp_boundaries[NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1];  // 3 boundaries
} dvfs_vft_t;

// Each VMAT info structure contains per-P-state data
typedef struct _dvfs_vmat_info_t {
    uint16_t ldo_dac_in[NUM_PSTATES];      // Voltage (LDO DAC input) for each P-state
    uint8_t memasst_hd_ema[NUM_PSTATES];   // Memory assist: HD EMA
    uint8_t memasst_hd_rawlm[NUM_PSTATES]; // Memory assist: HD RAWLM
    uint8_t memasst_hshc_ema[NUM_PSTATES]; // Memory assist: HSHC EMA
    uint8_t memasst_hshc_rawlm[NUM_PSTATES]; // Memory assist: HSHC RAWLM
    uint8_t memasst_tp_emaa[NUM_PSTATES];  // Memory assist: TP EMAA
    uint8_t memasst_tp_emab[NUM_PSTATES];  // Memory assist: TP EMAB
    uint8_t memasst_hd_emaw[NUM_PSTATES];  // Memory assist: HD EMAW
} dvfs_vmat_info_t;
```

### ITD Temperature Columns

ITD provides **4 temperature columns** (0-3), each representing a temperature range:

| Column | Temperature Range | Boundary (from fuses) |
|--------|------------------|----------------------|
| Col 0  | Coldest          | -                    |
| Col 1  | Cool             | curve_max_temp[0]    |
| Col 2  | Warm             | curve_max_temp[1]    |
| Col 3  | Hottest          | curve_max_temp[2]    |

Each column can have **different voltage requirements** for the same P-state because:
- At higher temperatures, transistors are slower → need higher voltage
- At lower temperatures, transistors are faster → can use lower voltage

### LDO DAC Conversion

The voltage is stored as an **LDO DAC code** (digital value), not millivolts:

```c
// Convert LDO DAC code to millivolts
#define LDODACIN_TO_MV(dac)  ((dac) << 1)  // dac * 2

// Convert millivolts to LDO DAC code
#define MV_TO_LDODACIN(mv)   (((mv) >> 1) + ((mv) & 1))  // mv / 2, rounded up

// Examples:
// DAC 375 = 750mV
// DAC 425 = 850mV
// DAC 475 = 950mV
```

### Memory Assist (Memasst) Boundaries

Memory assist settings control SRAM timing parameters. Different voltage ranges require different memasst settings:

```c
typedef struct _dvfs_core_memasst_entry_t {
    uint16_t ldo_dac_in;      // Voltage boundary (DAC code)
    uint8_t hd_ema;           // HD EMA setting
    uint8_t hd_rawlm;         // HD RAWLM setting
    uint8_t hshc_ema;         // HSHC EMA setting
    uint8_t hshc_rawlm;       // HSHC RAWLM setting
    uint8_t tp_emaa;          // TP EMAA setting
    uint8_t tp_emab;          // TP EMAB setting
    uint8_t hd_emaw;          // HD EMAW setting
    bool valid_boundary;      // Is this boundary valid?
} dvfs_core_memasst_entry_t;

typedef struct _dvfs_core_memasst_entries_t {
    dvfs_core_memasst_entry_t entry[DFVS_FUSED_COREMEMASST_COUNT];  // 5 entries
} dvfs_core_memasst_entries_t;
```

The boundaries come from **fuse fields**:
- `dvfs_core_mem_asst_trims_1_bound` → 750mV (entry[1])
- `dvfs_core_mem_asst_trims_2_bound` → 850mV (entry[2])
- `dvfs_core_mem_asst_trims_3_bound` → 950mV (entry[3])

---

## The Root Cause

### Scenario: Temperature Change During DVFS Transition

Consider a P-state where:
- **Column 0** (cold): 740mV (below 750mV boundary → memasst band 0)
- **Column 1** (cool): 755mV (above 750mV boundary → memasst band 1)
- **Column 2** (warm): 760mV (above 750mV boundary → memasst band 1)
- **Column 3** (hot): 770mV (above 750mV boundary → memasst band 1)

```
Voltage Boundaries:
         Band 0        │       Band 1        │       Band 2        │       Band 3
    (< 750mV)          │  (750mV - 850mV)    │  (850mV - 950mV)    │   (>= 950mV)
                       │                     │                     │
    ─────────────────────────────────────────────────────────────────────────────
                       │                     │                     │
    Col0: 740mV ●      │  Col1: 755mV ●      │                     │
                       │  Col2: 760mV ●      │                     │
                       │  Col3: 770mV ●      │                     │
                       │                     │                     │
                    750mV                 850mV                 950mV
```

### The Problem

1. DVFS starts a transition at **Column 0** (cold temperature) with voltage 740mV
2. Hardware uses **memasst settings for Band 0**
3. Mid-transition, temperature rises, ITD switches to **Column 1**
4. Column 1 voltage is 755mV, which uses **memasst settings for Band 1**
5. **Mismatch!** The hardware's current memasst doesn't match the new target
6. DVFS crawl hangs waiting for conditions that will never be met

---

## Solution Overview

### The Fix: Voltage Band Normalization

**Ensure all 4 temperature columns for each P-state are in the same memasst band.**

If voltages straddle a boundary, raise the lower voltages to the boundary value.

### Algorithm

For each P-state, check all 4 temperature column voltages:

| Condition | Action |
|-----------|--------|
| All 4 voltages < 750mV | Do nothing (all in Band 0) |
| All 4 voltages >= 750mV and < 850mV | Do nothing (all in Band 1) |
| All 4 voltages >= 850mV and < 950mV | Do nothing (all in Band 2) |
| All 4 voltages >= 950mV | Do nothing (all in Band 3) |
| **>= 1 voltage >= 750mV AND >= 1 voltage < 750mV** | **Raise min voltage to 750mV** |
| **>= 1 voltage >= 850mV AND >= 1 voltage < 850mV** | **Raise min voltage to 850mV** |
| **>= 1 voltage >= 950mV AND >= 1 voltage < 950mV** | **Raise min voltage to 950mV** |

### Example: Before and After

**Before Normalization:**
```
P-state 15:
  Col0: 740mV (Band 0) ← Problem!
  Col1: 755mV (Band 1)
  Col2: 760mV (Band 1)
  Col3: 770mV (Band 1)
```

**After Normalization:**
```
P-state 15:
  Col0: 750mV (Band 1) ← Raised to boundary
  Col1: 755mV (Band 1)
  Col2: 760mV (Band 1)
  Col3: 770mV (Band 1)

All columns now in the same band!
```

When a voltage is raised, we also copy the **memasst values** from the highest voltage column to ensure consistency.

---

## Implementation Details

### Key Functions Added

#### 1. `dvfs_dump_vmat_table()`
Prints the VMAT table for debugging, showing voltages and memasst boundaries.

```c
static void dvfs_dump_vmat_table(const dvfs_vft_t *vft, 
                                  const dvfs_core_memasst_entries_t *memasst_entries,
                                  const char *label);
```

**Output Example:**
```
=== VMAT Table Dump: Before ITD Band Normalization ===
MemAssist Boundary Thresholds (from fuses):
  Boundary 1: 375 lsb / 750 mV
  Boundary 2: 425 lsb / 850 mV
  Boundary 3: 475 lsb / 950 mV
P-State | TempCol0 (mV/DAC) | TempCol1 (mV/DAC) | TempCol2 (mV/DAC) | TempCol3 (mV/DAC)
--------|-------------------|-------------------|-------------------|------------------
P00     |  740/370          |  755/377          |  760/380          |  770/385          |
...
```

#### 2. `dvfs_find_straddled_boundary()`
Checks if a P-state's voltages straddle a memasst boundary.

```c
static bool dvfs_find_straddled_boundary(const dvfs_vft_t *vft, 
                                          int pstate,
                                          const dvfs_core_memasst_entries_t *memasst_entries,
                                          uint16_t *boundary_dac, 
                                          int *max_voltage_col);
```

**Algorithm:**
1. Get all 4 voltages for the P-state
2. Check each boundary from highest (950mV) to lowest (750mV)
3. For each boundary, check if `has_above && has_below`
4. If straddling detected, return the boundary DAC value

#### 3. `dvfs_normalize_pstate_voltage_bands()`
Normalizes a single P-state's voltages.

```c
static bool dvfs_normalize_pstate_voltage_bands(dvfs_vft_t *vft, 
                                                 int pstate,
                                                 const dvfs_core_memasst_entries_t *memasst_entries);
```

**Algorithm:**
1. Call `dvfs_find_straddled_boundary()` to detect if normalization needed
2. If straddling detected:
   - Raise any voltage below the boundary to the boundary value
   - Copy memasst values from the max voltage column to adjusted columns

#### 4. `dvfs_normalize_vmat_voltage_bands()`
Iterates through all P-states and normalizes each.

```c
static int dvfs_normalize_vmat_voltage_bands(dvfs_vft_t *vft,
                                              const dvfs_core_memasst_entries_t *memasst_entries);
```

**Algorithm:**
1. Dump VMAT table (before)
2. For each P-state (0 to 31):
   - Call `dvfs_normalize_pstate_voltage_bands()`
3. Dump VMAT table (after)
4. Return count of adjusted P-states

### Integration Point

The normalization is called in `dvfs_fuse_based_init()` when ITD is enabled:

```c
// In dvfs_fuse_based_init():
bool itd_en = cfg->init_cfg.pex_features.itd_en;

// Create a local mutable copy of VFT for normalization
dvfs_vft_t normalized_vft;
memcpy(&normalized_vft, cfg->fuse_cfg.vft, sizeof(dvfs_vft_t));

// Apply ITD voltage band normalization when ITD is enabled
if (itd_en)
{
    dvfs_normalize_vmat_voltage_bands(&normalized_vft, cfg->fuse_cfg.memasst_entries);
}

// Use the normalized VFT for VMAT programming
const dvfs_vft_t *vft = &normalized_vft;
```

### Data Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            Fuse Reading                                      │
│  power_fuses_read_memasst() → p_fuses->memasst                              │
│  power_fuses_read_vf() → p_fuses->vf → dvfs_vft_from_fuse_data_per_itd()   │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Config Population                                    │
│  power_init_update_dvfs_cfg_core():                                         │
│    p_dvfs_cfg->fuse_cfg.vft = &p_runconfig->dvfs_vft.curveset[assigned_vft] │
│    p_dvfs_cfg->fuse_cfg.memasst_entries = &p_runconfig->fuses.memasst       │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           DVFS Init                                          │
│  dvfs_init() → dvfs_fuse_based_init():                                      │
│    if (itd_en) {                                                            │
│      dvfs_normalize_vmat_voltage_bands(&normalized_vft,                     │
│                                         cfg->fuse_cfg.memasst_entries);     │
│    }                                                                        │
│    // Program VMAT with normalized voltages                                 │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Files Modified

### 1. `.externs/repo/silibs/libraries/dvfs/src/dvfs.c`

| Change | Description |
|--------|-------------|
| Added `dvfs_dump_vmat_table()` | Debug function to print VMAT with boundary info |
| Added `dvfs_find_straddled_boundary()` | Detects if P-state voltages straddle a boundary |
| Added `dvfs_normalize_pstate_voltage_bands()` | Normalizes a single P-state |
| Added `dvfs_normalize_vmat_voltage_bands()` | Normalizes all P-states |
| Modified `dvfs_fuse_based_init()` | Calls normalization when ITD enabled |
| Added `dvfs_print_vmat_table()` | Public API wrapper |
| Added `dvfs_normalize_vmat_bands()` | Public API wrapper |

### 2. `.externs/repo/silibs/libraries/dvfs/include/dvfs_struct.h`

| Change | Description |
|--------|-------------|
| Added `memasst_entries` field | Pointer to memasst boundary entries in `dvfs_fuse_config_t` |

```c
typedef struct _dvfs_fuse_config_t {
    const dvfs_vft_t *vft;
    const dvfs_core_memasst_entries_t *memasst_entries;  // NEW
    uint8_t adclk_offset;
    bool core_disabled;
} dvfs_fuse_config_t;
```

### 3. `.externs/repo/silibs/libraries/dvfs/include/dvfs.h`

| Change | Description |
|--------|-------------|
| Added `dvfs_print_vmat_table()` declaration | Public API for debugging |
| Added `dvfs_normalize_vmat_bands()` declaration | Public API for normalization |

### 4. `src/services/power/src/power_hw_int.c`

| Change | Description |
|--------|-------------|
| Modified `power_init_update_dvfs_cfg_core()` | Sets `memasst_entries` pointer |

```c
p_dvfs_cfg->fuse_cfg.memasst_entries = &p_runconfig->fuses.memasst;
```

### 5. `src/services/power/src/power_cli_requests.c`

| Change | Description |
|--------|-------------|
| Modified force pstate path | Sets `memasst_entries` pointer for CLI override |

### 6. `tests/unit/services/power/hw_int/power_hw_int_test.cpp`

| Change | Description |
|--------|-------------|
| Updated test | Sets `memasst_entries` to match production code |

---

## Testing and Verification

### Debug Output

When ITD is enabled, you'll see debug output like:

```
=== VMAT Table Dump: Before ITD Band Normalization ===
MemAssist Boundary Thresholds (from fuses):
  Boundary 1: 375 lsb / 750 mV
  Boundary 2: 425 lsb / 850 mV
  Boundary 3: 475 lsb / 950 mV
...

VMAT Boundary Check: P15 straddles boundary 1 (375 lsb / 750 mV)
VMAT Band Normalization: P15 Col0 - raised DAC from 370 (740mV) to 375 (750mV), copied memasst from col3

=== VMAT Table Dump: After ITD Band Normalization ===
...

VMAT Band Normalization: Adjusted 5 P-states to ensure voltages don't straddle mem-assist boundaries
```

### Verification Checklist

- [ ] All P-states have all 4 temp columns in the same memasst band
- [ ] No DVFS crawl hangs during temperature changes
- [ ] Memasst values are consistent across adjusted columns
- [ ] Debug output shows correct boundary detection and normalization

### Edge Cases Handled

1. **All voltages identical**: No normalization needed
2. **No valid boundaries in fuses**: Normalization skipped (NULL check)
3. **ITD disabled**: Normalization not called
4. **Force pstate via CLI**: Uses separate path, memasst_entries still available

---

## Summary

This implementation prevents DVFS crawl hangs by ensuring voltage consistency across ITD temperature columns:

1. **Problem**: Temperature changes mid-transition can cause memasst boundary mismatches
2. **Solution**: Normalize voltages so all 4 columns are in the same memasst band
3. **Method**: Raise lower voltages to boundary threshold, copy memasst from highest voltage column
4. **Integration**: Automatically applied during DVFS init when ITD is enabled

The fix is transparent to the rest of the system and only activates when ITD is enabled, minimizing impact on non-ITD configurations.
