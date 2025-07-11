#ifndef WEIGHT_API_HH
#define WEIGHT_API_HH

#include <torch/torch.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

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
    bool poke(pid_t pid, uintptr_t addr, const std::vector<uint8_t>& bytes);

    // Optional: map from weight addresses to names
    void set_weight_map(const std::map<uintptr_t, std::string>& map);
    const std::map<uintptr_t, std::string>& weight_map();

}

#endif