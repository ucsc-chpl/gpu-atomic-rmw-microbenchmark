#include "vk_utils.h"
#include <iostream>
#include <sstream>
using namespace std;
using easyvk::Instance;
using easyvk::Device;
using easyvk::Buffer;
using easyvk::Program;
using easyvk::vkDeviceType;

#ifdef __ANDROID__
#define APPNAME "GPURmwTests"
#endif

uint32_t validate_output(easyvk::Buffer resultBuf, uint32_t rmw_iters, uint32_t num_trials, uint32_t contention, 
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

uint32_t occupancy_discovery(easyvk::Device device, uint32_t workgroup_size, uint32_t workgroups, vector<uint32_t> spv_code, 
                                uint32_t num_trials, uint32_t rmw_iters) {
        int maxOccupancyBound = -1;
        for (int i = 0; i < num_trials; i++) {
            Buffer result_buf =  Buffer(device, workgroup_size * workgroups, sizeof(uint32_t));
            result_buf.clear();
            Buffer rmw_iters_buf = Buffer(device, 1, sizeof(uint32_t));
            rmw_iters_buf.store<uint32_t>(0, rmw_iters);
            Buffer strat_buf = Buffer(device, workgroup_size * workgroups, sizeof(uint32_t)); 
            for (int i = 0; i < workgroup_size * workgroups; i += 1) strat_buf.store<uint32_t>(i, i);
            Buffer count_buf = Buffer(device, 1, sizeof(uint32_t));
            count_buf.store<uint32_t>(0, 0);
            Buffer poll_open_buf = Buffer(device, 1, sizeof(uint32_t));
            poll_open_buf.store<uint32_t>(0, 1); // Poll is initially open.
            Buffer M_buf = Buffer(device, workgroups, sizeof(uint32_t));
            Buffer now_serving_buf = Buffer(device, 1, sizeof(uint32_t));
            now_serving_buf.store<uint32_t>(0, 0);
            Buffer next_ticket_buf = Buffer(device, 1, sizeof(uint32_t));
            next_ticket_buf.store<uint32_t>(0, 0);
            vector<Buffer> kernelInputs = {             result_buf, rmw_iters_buf, strat_buf,
                                                        count_buf, 
                                                        poll_open_buf,
                                                        M_buf,
                                                        now_serving_buf,
                                                        next_ticket_buf};
            auto program = Program(device, spv_code, kernelInputs);
            program.setWorkgroups(workgroups);
            program.setWorkgroupSize(workgroup_size);
            program.initialize("occupancy_discovery");
            program.run();
            if ((int) count_buf.load<uint32_t>(0) > maxOccupancyBound) {
                maxOccupancyBound = count_buf.load<uint32_t>(0);
            }
            program.teardown();
            result_buf.teardown();
            rmw_iters_buf.teardown();
            strat_buf.teardown();
            count_buf.teardown();
            poll_open_buf.teardown();
            M_buf.teardown();
            next_ticket_buf.teardown();
            now_serving_buf.teardown();
        }
        return (uint32_t) maxOccupancyBound;
}

vector<uint32_t> get_spv_code(const string& filename) {
    ifstream file(filename);
    vector<uint32_t> spv_code;
    char ch;
    while (file >> ch) {
        if (isdigit(ch)) {
            file.unget();
            uint32_t value;
            file >> value;
            spv_code.push_back(value);
        }
    }
    file.close();
    return spv_code;
}

vector<int> select_configurations(vector<string> options, string prompt) {
    cout << prompt << endl;
    for (size_t i = 0; i < options.size(); ++i) {
        cout << i + 1 << ". " << options[i] << endl;
    }
    vector<int> selections;
    string input;
    cout << "Enter choices separated by space: ";
    getline(cin, input); 
    stringstream ss(input);
    int choice;
    while (ss >> choice) {
        if (choice >= 1 && choice <= options.size()) {
            selections.push_back(choice - 1);
        }
    }
    return selections;
}

uint32_t get_params(string prompt) {
    int value;
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        stringstream ss(input);
        if (ss >> value && ss.eof() && value > 0) {
            break;
        }
        cout << "Invalid input" << endl;
    }
    return value;
}
