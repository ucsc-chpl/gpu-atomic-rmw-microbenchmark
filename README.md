# Atomic-RMW: GPU Microbenchmark 

## Table of Contents
- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Running Microbenchmark](#running-microbenchmark)

## Introduction
This is a simple microbenchmark that calculates Atomic Read-Modify-Write (RMW) throughput. It allows you to configure various parameters such as contention, padding, RMW iterations, and the number of workgroups. You can also generate a heatmap of various combinations of contention and padding with the scripts provided.

## Prerequisites
Before you begin, ensure you have setup the following requirements:

### Frameworks and Tools
- **Vulkan:**
  - [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
  - [clspv](https://github.com/google/clspv)
  - [Android ADB](https://developer.android.com/studio/command-line/adb) / [NDK](https://developer.android.com/ndk)
  - `make` utility for building
  - [Python](https://www.python.org/)

## Installation
To install the necessary dependencies, follow these steps:

1. Clone the repository:
    ```bash
    git clone https://github.com/ucsc-chpl/gpu-atomic-rmw-microbenchmark.git
    ```
    
2. Build easyvk:
    ```bash
    cd gpu-atomic-rmw-microbenchmark/easyvk/
    git submodule update --init --recursive
    make
    ```

## Running Microbenchmark
To run the microbenchmark, execute the following series of commands:

### Vulkan

```bash
cd gpu-atomic-rmw-microbenchmark/src/
make
./atomic_rmw_test -w <workgroups> -d <device> -c <contention> -p <padding> -i <rmw_iterations> # test a single configuration
./heatmap_results.sh <workgroups> <device> <rmw_iterations> # test multiple configurations of contention and padding
python3 heatmap_generator.py # generate a heatmap displaying results
```

### Vulkan (Android)
```bash
cd gpu-atomic-rmw-microbenchmark/src/
make android
adb devices # get serial number
adb -s [SERIAL_NUMBER] shell getprop ro.product.cpu.abilist # get supported CPU, use ro.product.cpu.abi if pre-lollipop version
cp *.cinit *.sh build/android/obj/local/[SUPPORTED_CPU]
adb -s [SERIAL_NUMBER] push build/android/obj/local/[SUPPORTED_CPU]/ /data/local/tmp/rmw
adb -s [SERIAL_NUMBER] shell
  cd data/local/tmp/rmw/[SUPPORTED_CPU]
  ./rmw_benchmark -w <workgroups> -d <device> -c <contention> -p <padding> -i <rmw_iterations>
  sh heatmap_results.sh <workgroups> <device> <rmw_iterations> # generates a text file of results needed if heatmap is generated
  exit
adb -s [SERIAL_NUMBER] pull /data/local/tmp/rmw/[SUPPORTED_CPU]/result.txt .
python3 heatmap_generator.py
```
