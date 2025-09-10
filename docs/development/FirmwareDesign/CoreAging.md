# Power telemetry Core aging  Design Document

## Introduction
Core aging counters are hardware-based telemetry mechanisms designed to monitor the degradation of processor cores over time due to aging effects. 

Each core in Kingsgate is equipped with:

-- 8 unaged counters: Serve as baseline references.

-- 8 aged counters: Measure the actual aging of the core. 

These counters are read periodically—typically every 24 hours—when the core is in the C2 low-power state.  On every successful read of one aging counter pair, we must configure the next pair of aging counter measurement. 
For a 24hr period, if we are not able to read all the pairs of aging counters, we should report whatever we have read successfully so far   0 filled data for the remaining counters.
Only one successful reading per counter pair is needed in a window of 24hr. 


Power telemetry data processing component during preparation of power package, calls 'data_smpl_update_metrics_for_cores_aging_counters',  which calls the comp_metrics_for_single_core_aging_counter for each core and checks if a counter is armed and  invoke the aging_counter_read() function for enabled and armed core, this function call SILIBS DVFS APIs in a sequence to meet the reading conditions. ATU translation for AP core dvfs address space and setup default configuration for counter registers. Once the data is read,  the comp_metrics_for_single_core_aging_counters() api to store the data into core compute matrices for 24 hr records and armed the core for next counter reading by calling  aging_counter_enable_sensor_measurement.

When 24hr timer expires , executive component trigger a 24hr power record collection and call packaging api to read aging counter from compute matrics. 

## Dependencies

- Driver Framework
- DVFS

## Note 
In the C2 PCM aging measurement mode, FW MUST ENSURE that ONLY ONE pair of ROs from the two banks
is selected for reading. If FW sets multiple read , the HW will attempt to collect data from all the specified
ROs as supported in the regular DVFS PCM FSM mode, and this will take time to iterate over all the specified ROs. This may
mean a higher probability of not completing the measurement since the core exits C2 in the middle of the long time
interval required to make multiple RO measurements, and therefore not being able to get any C2 aging data.
Section: 3.18.4.4 : ref: https://microsoft.sharepoint.com/:w:/r/teams/Kingsgate/Shared%20Documents/MicroArchitecture%20Specs/MAS/Pwr%20Mgmt%20%26%20Sensors%20MASes/Kingsgate%20Sensors%20MAS.docx?d=wfc54fea30c604d31aa28f4a8922b1218&csf=1&web=1&e=2GL5Ad
    


### Terms

| Term   | Description                     |
| ------ | ------------------------------- |
| RO                   | Ring Oscillators                      |
| DVFS                   | Dynamic Voltage and Frequency Scaling                        |
| PCM                   | Process Control Monitor                      |
| FSM                   | Finite State Machine                 |



# Data collection for aging counter 

:::mermaid
sequenceDiagram
participant Exec as Exec_Cmpnt
participant Dataproc.Cmpnt as DataProc Cmpnt
participant Dataproc.Sampler as DataProc sampler 
participant Dataproc.compute as DataProc compute matrics
participant Dataproc.HW as DataProc.aging counter
participant dvfs as Silib.Dvfs

Exec->>Exec: pwr_pkg_timer Expired(_pwr_pkg_gen_count==0)
Exec->>Dataproc.Cmpnt: data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg
Dataproc.Cmpnt->>Dataproc.Sampler: data_smpl_update_metrics_for_cores_aging_counters
Dataproc.Sampler->>Dataproc.Sampler: data_smpl_process_aging_data

loop for core = 0 to 67
    Dataproc.Sampler->>Dataproc.compute: comp_metrics_for_single_core_aging_counters()
    Dataproc.compute->>Dataproc.HW: aging_counter_get_sensor_status()
    alt status == true
        Dataproc.compute->>Dataproc.HW: aging_counter_read()
        Dataproc.HW->>dvfs: dvfs_read
        dvfs-->>Dataproc.HW: complete
        Dataproc.HW-->>Dataproc.compute: read_complete
        alt success on read
            Note right of Dataproc.compute: Configure next counter pair for same core 0 at end of current iteration
            Dataproc.compute->>Dataproc.HW: aging_counter_enable_sensor_measurement
        else
        end
    else
    end
end
:::


# packaging of data for aging counter 

:::mermaid
sequenceDiagram
participant Exec as Exec_Cmpnt

participant Dataproc.compute as DataProc compute matrics
participant Dataproc.Pkg_Intf as DataProc Inband PkgInft
participant InBand as In Band Tlm Cmpnt
participant PkgCr as In Band Tlm Cmpnt.Package Creation
participant DdrMgr as In Band Tlm Cmpnt.DDR Manager
participant MtsMgr as In Band Tlm Cmpnt.MTS Manager
participant MTS as MTS
Exec->>Exec: Power 24hr Package Timer Expired
Exec->>InBand:in_band_tlm_cmpnt_generate_24hr_pkg()
InBand->>DdrMgr: ddr_manager_allocate_mem_for_pwr_pkg()
InBand->>PkgCr :package_create_24hr_pkg()
PkgCr->>PkgCr: Loop through all 24 hr Power Records
loop All 24hr power Records
    alt [Pwr Event is enabled]
        PkgCr->>Dataproc.Pkg_Intf: Get Event Data for Record
        Dataproc.Pkg_Intf->>Dataproc.compute: read compute matrics
        Dataproc.Pkg_Intf->>PkgCr: Data
        PkgCr->>PkgCr: Add record to Package
    else [Pwr Event is not enabled]
        PkgCr->>PkgCr: Skip record
    end
end
PkgCr->>MtsMgr: mts_manager_queue_tlm_package()
MtsMgr->>MtsMgr: Queue package info in mts_pkg_active_list

:::
