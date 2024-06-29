#include <iostream>
#include "vk_utils.h"

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

extern "C" void atomic_rmw_microbenchmark(easyvk::Device device, uint32_t contention, uint32_t padding) { 

    uint32_t num_trials = 8;
    uint32_t workgroup_size = device.properties.limits.maxComputeWorkGroupInvocations;
    uint32_t workgroups = occupancy_discovery(device, workgroup_size, 256, get_spv_code("occupancy_discovery.cinit"), 3, 1024);
    uint32_t global_work_size = workgroup_size * workgroups;
    uint32_t buf_size = ((global_work_size) * padding) / contention;
    vector<uint32_t> spv_code = get_spv_code("atomic_fa_relaxed.cinit");

    cout << '\n' << device.properties.deviceName << ": workgroups (" << workgroup_size << ", 1) x " << workgroups << endl;

    Buffer result_buf = Buffer(device, buf_size, sizeof(uint32_t));
    Buffer rmw_iters_buf = Buffer(device, 1, sizeof(uint32_t));
    Buffer strat_buf = Buffer(device, global_work_size, sizeof(uint32_t)); 
    
    for (int i = 0; i < global_work_size; i += 1) strat_buf.store<uint32_t>(i, (i / contention) * padding);

    uint32_t rmw_iters = 128; 
    while(1) {
        rmw_iters_buf.store<uint32_t>(0, rmw_iters);
        result_buf.clear();
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
        }
        if ((total_duration/num_trials) < 1000000.0) {
            rmw_iters *= 2;
            rmw_program.teardown();
        } else {
            cout << "contention " << contention << ", padding " << padding << ": " << total_rate / num_trials << " atomic operations per microsecond" << endl;
            rmw_program.teardown();
            break;
        }
    }

    uint32_t error_count = validate_output(result_buf, rmw_iters, num_trials, contention, padding, buf_size);
    cout << "kernel error count: " << error_count << endl;
    result_buf.teardown(); 
    rmw_iters_buf.teardown();
    strat_buf.teardown();

    return;
}

int main() {
    cout << "Atomic RMW Microbenchmark: Contiguous Access, Atomic_FA_Relaxed\n";
    auto instance = easyvk::Instance(USE_VALIDATION_LAYERS);
	auto physicalDevices = instance.physicalDevices();

    vector<string> device_options;
    for (size_t i = 0; i < physicalDevices.size(); i++) {
        auto device = easyvk::Device(instance, physicalDevices.at(i));
        device_options.push_back(device.properties.deviceName);
        device.teardown();
    }

    auto selected_devices = select_configurations(device_options, "Select devices:");

    uint32_t contention = get_params("Enter a value for Contention: ");
    uint32_t padding = get_params("Enter a value for Padding: ");

    for (const auto& choice : selected_devices) {
        auto device = easyvk::Device(instance, physicalDevices.at(choice));
        atomic_rmw_microbenchmark(device, contention, padding);
        device.teardown();
    }

    instance.teardown();
    return 0;
}
