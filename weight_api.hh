#ifndef WEIGHT_API_HH
#define WEIGHT_API_HH

#include <torch/torch.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct WeightEntry {
    uintptr_t address;
    size_t size;
};

namespace weight_debugger {

    // Launch a child process and attach to it using ptrace
    pid_t launch();

    // Attach to a running process
    bool attach(pid_t pid);

    // Detach and resume the process
    void detach(pid_t pid);

    // Read memory from a traced process
    std::vector<uint8_t> peek(pid_t pid, uintptr_t addr, size_t num_bytes);

    // Write memory to a traced process
    bool poke(pid_t pid, uintptr_t remote_addr, const void* local_src, size_t num_bytes);

    void set_weight_map(const std::map<std::string, WeightEntry>& map);
    const std::map<std::string, WeightEntry>& weight_map();

}

#endif