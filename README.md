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

- `600.perlbench_s-210B.champsimtrace.xz`: A condensed version of the Perl v5.22 scripting language 
- `602.gcc_s-734B.champsimtrace.xz`: Based off the GCC 4.5.0 compiler 
- `603.bwaves_s-3699B.champsimtrace.xz` : A numerical solver that simulates cubic cell high pressure blast waves
- `605.mcf_s-665B.champsimtrace.xz` : Based on the MCF program that is used for scheduling public mass transportation
- `619.lbm_s-665B.champsimtrace.xz` : A 3D simulation of incompressible fluids
- `625.x264_s-18B.champsimtrace.xz` : A software library that encodes video streams to H.264/MPEG-4 AVC format
- `625.pop2_s-18B.champsimtrace.xz` : The parallel Ocean Program climate model that simulates the earth’s climate states
- `641.leela_s-800B.champsimtrace.xz` : A simulation of the Go playing engine implementing AI and position estimators
- `644.nab_s.champsimtrace.xz` : Based on the Nucleic Acid Builder to perform molecular modeling
- `648.exchange2_s-1699B.champsimtrace.xz` : Fotonik3D is used to compute photonic waveguide transmission coefficients

These benchmarks are chosen to span a range of CPU and memory intensities, helping to expose strengths and weaknesses of each policy.

## Analysis Approach

For each benchmark, we collect performance statistics such as:

- **IPC (Instructions Per Cycle)**
- **Cache hit and miss rates**
- **Total CPU cycles consumed**
  

## Final Results
<br>Results are saved in the `outputs/` folder. 
<br>Analysis are saved in the `Analysis.pdf`.
<br>[Link to Project Video](https://drive.google.com/file/d/13QHVTJSScyHyREM_gm2y3EeAkBKpsID9/view?usp=drive_link)
