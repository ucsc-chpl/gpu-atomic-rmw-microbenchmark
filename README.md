# Atomic-RMW: GPU Microbenchmark 

## Table of Contents
- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)

## Introduction
This is a simple microbenchmark that calculates Atomic Read-Modify-Write (RMW) throughput. It allows you to configure various parameters such as thread contention, padding, RMW iterations, and the number of workgroups. You can also generate a heatmap of various combinations of contention and padding with the scripts provided. The program will output the atomic throughput (measured in atomic operations per microsecond), the duration (in microseconds), and a count of any kernel computation errors (check kernel validation function).

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

## Usage
To run the microbenchmark on your device, follow the series of commands:

### Vulkan (Desktop)

#### Compilation

1. **Navigate to the source directory**:
    ```bash
    cd src/
    ```

2. **Compile the project**:
    ```bash
    make
    ```

#### Running the microbenchmark

#### Single configuration

To test a single configuration of the microbenchmark:

1. **Run the microbenchmark** from the `src` directory:
    ```bash
    ./atomic_rmw_test -w <workgroups> -d <device> -c <contention> -p <padding> -i <rmw_iterations>
    ```

    - `-w <workgroups>`: **(Required)** The number of workgroups to use. Defaults to 1.
    - `-d <device>`: (Optional) The index of the device to use. Defaults to 0.
    - `-c <contention>`: (Optional) The number of threads contending on the same machine word. Defaults to 1.
    - `-p <padding>`: (Optional) The number of machine words between those accessed. Defaults to 1.
    - `-i <rmw_iterations>`: (Optional) The number of RMW iterations. Defaults to 128.

#### Multiple configurations 

To test multiple configurations of thread contention and padding and produce a heatmap:

1. **Run the bash script** from the `src` directory:
    ```bash
    ./heatmap_results.sh <workgroups> <device> <rmw_iterations>
    ```
    
2. **Generate a heatmap displaying results** from the `src` directory:
    ```bash
    python3 heatmap_generator.py
    ```
  
3. **Example heatmap generation**: <br>

   For a NVIDIA Geforce RTX 4070, the following parameters work well:
   - `workgroups`: 46
   - `rmw_iterations`: 4096 
   <br>
   <table>
      <tr>
        <td><img src="https://drive.google.com/uc?id=1PDPJ38CIxa-k9hwPPcu487seezMP9NC6" alt="NVIDIA Geforce RTX 4070" width="500" ></td>
      </tr>
   </table>

### Vulkan (Android)

#### Compilation

1. **Navigate to the source directory**:
    ```bash
    cd src/
    ```

2. **Compile the project for Android**:
    ```bash
    make android
    ```

#### Running the microbenchmark

1. **Get the serial number of the connected Android device**:
    ```bash
    adb devices
    ```

2. **Get supported CPU ABIs**:
    ```bash
    adb -s [SERIAL_NUMBER] shell getprop ro.product.cpu.abilist
    # If Android is pre-Lollipop version, use:
    adb -s [SERIAL_NUMBER] shell getprop ro.product.cpu.abi
    ```

3. **Copy necessary files**:
    ```bash
    cp *.cinit *.sh build/android/obj/local/[SUPPORTED_CPU]
    ```

4. **Push files to the Android device**:
    ```bash
    adb -s [SERIAL_NUMBER] push build/android/obj/local/[SUPPORTED_CPU]/ /data/local/tmp/rmw
    ```

5. **Navigate to microbenchmark on the Android device**:
   ```bash
    adb -s [SERIAL_NUMBER] shell
    cd /data/local/tmp/rmw/[SUPPORTED_CPU]
   ```

#### Single configuration

To test a single configuration of the microbenchmark:

1. **Run the microbenchmark** from the `[SUPPORTED_CPU]` directory:
    ```bash
    ./atomic_rmw_test -w <workgroups> -d <device> -c <contention> -p <padding> -i <rmw_iterations>
    ```

    - `-w <workgroups>`: **(Required)** The number of workgroups to use. Defaults to 1.
    - `-d <device>`: (Optional) The index of the device to use. Defaults to 0.
    - `-c <contention>`: (Optional) The number of threads contending on the same machine word. Defaults to 1.
    - `-p <padding>`: (Optional) The number of machine words between those accessed. Defaults to 1.
    - `-i <rmw_iterations>`: (Optional) The number of RMW iterations. Defaults to 128.

#### Multiple configurations 

To test multiple configurations of thread contention and padding and produce a heatmap:

1. **Run the bash script** from the `[SUPPORTED_CPU]` directory:
    ```bash
    sh heatmap_results.sh <workgroups> <device> <rmw_iterations>
    ```

2. **Exit the shell and pull the results file from the Android device**:
    ```sh
    exit
    adb -s [SERIAL_NUMBER] pull /data/local/tmp/rmw/[SUPPORTED_CPU]/result.txt .
    ```

3. **Generate a heatmap displaying results** from the `src` directory:
    ```bash
    python3 heatmap_generator.py
    ```
  
4. **Example heatmap generation**: <br>

   For a Samsung Xclipse 920, the following parameters work well:
   - `workgroups`: 3
   - `rmw_iterations`: 32768
   <br>
   <table>
      <tr>
        <td><img src="https://drive.google.com/uc?id=13v95qk-PKvnlzYIGcKBhaevXfGYU4DBH" alt="Samsung Xclipse 920" width="500" ></td>
      </tr>
   </table>
