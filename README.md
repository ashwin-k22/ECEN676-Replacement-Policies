# ECEN676-Replacement-Policies in Champsim 

This repository contains our implementation and performance analysis of various cache replacement policies using the ChampSim simulator as part of the ECEN676 course project.

The primary goals are:
- To implement multiple replacement policies within ChampSim
- To evaluate how each performs across diverse benchmarks
- To draw insights about which policies are more suitable for certain types of workloads or cache behaviors

##  Replacement Policies Implemented

The following policies have been implemented and integrated into ChampSim for simulation:

- **SHiP (Signature-based Hit Predictor):** Uses program counters to predict whether a cache block will be reused.
- **LRU (Least Recently Used):** Evicts the block that has not been used for the longest time.
- **Seg-LRU (Segmented LRU):** Splits the cache into segments to better track temporal locality.
- **DRRIP (Dynamic Re-reference Interval Prediction):** Balances between static policies using dynamic set dueling to adapt at runtime.

##  Simulator: ChampSim

[ChampSim](https://github.com/ChampSim/ChampSim) is a trace-based simulator used for evaluating memory hierarchy designs. It supports user-defined replacement policies and allows repeatable, deterministic experiments using compressed memory access traces.

## Benchmarks

To evaluate the effectiveness of each policy, we selected a diverse set of memory access traces that represent different workload characteristics:

- `600.perlbench_s-210B.champsimtrace.xz`
- `602.gcc_s-734B.champsimtrace.xz`
- `603.bwaves_s-3699B.champsimtrace.xz`
- `605.mcf_s-665B.champsimtrace.xz`
- `625.x264_s-18B.champsimtrace.xz`
- `641.leela_s-800B.champsimtrace.xz`
- `644.nab_s.champsimtrace.xz`
- `648.exchange2_s-1699B.champsimtrace.xz`

These benchmarks are chosen to span a range of CPU and memory intensities, helping to expose strengths and weaknesses of each policy.

## Analysis Approach

For each benchmark, we collect performance statistics such as:

- **IPC (Instructions Per Cycle)**
- **Cache hit and miss rates**
- **Total CPU cycles consumed**

Results are saved in the `output/` folder. 

## Repository Structure



├── README.md                   # Project overview
├── configs/                    # ChampSim config files for each policy
│   ├── ship_config.json
│   ├── lru_config.json
│   ├── seglru_config.json
│   └── drrip_config.json
├── replacement/                # Source code for replacement policies
│   ├── ship/
│   ├── lru/
│   ├── seg_lru/
│   └── drrip/
├── outputs/                     # Simulation results (e.g., CSV logs)
│   ├── 648.txt
│   ├── 604.txt
│   ├── ...
