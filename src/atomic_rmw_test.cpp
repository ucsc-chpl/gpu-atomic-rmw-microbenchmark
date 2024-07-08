#include <iostream>
#include <getopt.h>
#include "easyvk.h"

#ifdef __ANDROID__
#include <android/log.h>
#define USE_VALIDATION_LAYERS false
#define APPNAME "GPURmwTests"
#else
#define USE_VALIDATION_LAYERS true
#endif

using namespace std;
using easyvk::Instance;
using easyvk::Device;
using easyvk::Buffer;
using easyvk::Program;
using easyvk::vkDeviceType;

// Validate output buffer with expected result at each atomic location
uint32_t validate_kernel(easyvk::Buffer resultBuf, uint32_t rmw_iters, uint32_t num_trials, uint32_t contention, 
                            uint32_t padding, uint32_t size) {
    uint32_t error_count = 0;
    for (int access = 0; access < size; access += padding) {
            uint32_t observed_output = resultBuf.load<uint32_t>(access);
            uint32_t expected_output = rmw_iters * num_trials * contention;
            if (observed_output != expected_output) {
                 error_count++;
            }
    }
    return error_count;
}

// Simple microbenchmark that performs atomic_fetch_add under a contiguous access pattern
extern "C" void atomic_rmw_microbenchmark(easyvk::Device device, uint32_t contention, uint32_t padding, uint32_t workgroups, uint32_t rmw_iters) { 

    uint32_t num_trials = 3;
    uint32_t workgroup_size = device.properties.limits.maxComputeWorkGroupInvocations;    
    uint32_t buf_size = ((workgroup_size * workgroups) * padding) / contention;
    vector<uint32_t> spv_code =  
    #include "atomic_fa_relaxed.cinit"
    ;

   cout << "\nDevice: " << device.properties.deviceName << ", workgroups (" << workgroup_size << ", 1) x " << workgroups << endl
     << "Running " << rmw_iters << " RMW iterations on contention " << contention
     << ", padding " << padding << flush;

    Buffer result_buf = Buffer(device, buf_size, sizeof(uint32_t));
    Buffer rmw_iters_buf = Buffer(device, 1, sizeof(uint32_t));
    Buffer strat_buf = Buffer(device, workgroup_size * workgroups, sizeof(uint32_t)); 

    result_buf.clear();
    rmw_iters_buf.store<uint32_t>(0, rmw_iters);
    for (int i = 0; i < workgroup_size * workgroups; i += 1) strat_buf.store<uint32_t>(i, (i / contention) * padding);

    vector<Buffer> buffers = {result_buf, rmw_iters_buf, strat_buf};

    Program rmw_program = Program(device, spv_code, buffers);
    rmw_program.setWorkgroups(workgroups);
    rmw_program.setWorkgroupSize(workgroup_size);
    rmw_program.initialize("rmw_test");

    float total_rate = 0.0;
    float total_duration = 0.0;
    for (int i = 1; i <= num_trials; i++) {
        auto kernel_time = rmw_program.runWithDispatchTiming();
        total_duration += (kernel_time / (double) 1000.0);
        total_rate += ((static_cast<float>(rmw_iters) * workgroup_size * workgroups) / (kernel_time / (double) 1000.0)); 
        cout << "." << flush;
    }

    cout << "\n\nThroughput: " << (total_rate / num_trials) << " atomic operations per microsecond" 
     << "\nTime: " << (total_duration / num_trials) << " microseconds" 
     << "\nKernel errors: " << validate_kernel(result_buf, rmw_iters, num_trials, contention, padding, buf_size) << endl;
    
    rmw_program.teardown();
    result_buf.teardown(); 
    rmw_iters_buf.teardown();
    strat_buf.teardown();

    return;
}

int main(int argc, char* argv[]) {
    auto instance = easyvk::Instance(USE_VALIDATION_LAYERS);
	auto physicalDevices = instance.physicalDevices(); 
    uint32_t contention = 1, padding = 1, rmw_iters = 128;
    uint32_t workgroups = 1;
    uint32_t selected_device = 0;

    if (argc == 1) {
        cout << "Usage: program_name -w <workgroups> [-d <device>] [-c <contention>] [-p <padding>] [-i <rmw_iterations>]" << endl;
        return 1;
    }
    int opt;
    while ((opt = getopt(argc, argv, "c:p:w:d:i:")) != -1) {
        switch (opt) {
            case 'c':
                contention = static_cast<uint32_t>(stoi(optarg));
                break;
            case 'p':
                padding = static_cast<uint32_t>(stoi(optarg));
                break;
            case 'w':
                workgroups = static_cast<uint32_t>(stoi(optarg));
                break;
            case 'd':
                selected_device = static_cast<uint32_t>(stoi(optarg));
                break;
            case 'i':
                rmw_iters = static_cast<uint32_t>(stoi(optarg));
                break;
        }
    }
    auto device = easyvk::Device(instance, physicalDevices.at(selected_device));
    atomic_rmw_microbenchmark(device, contention, padding, workgroups, rmw_iters);
    device.teardown();
    instance.teardown();
    return 0;
}
