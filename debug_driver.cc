#include "weight_api.hh"
#include <sys/ptrace.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <fstream>
#include <string>
#include <unistd.h> 

void print_proc_maps_with_pid(pid_t pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps(path);
    if (!maps) {
        std::cerr << "Failed to open " << path << std::endl;
        return;
    }

    std::string line;
    while (std::getline(maps, line)) {
        std::cout << line << std::endl;
    }
}

int main() {
    // Launch and attach to the hardcoded model binary ("./simple_resnet")
    pid_t pid = weight_debugger::launch();
    if (pid == -1) {
        std::cerr << "Failed to launch model process." << std::endl;
        return 1;
    }

    // if (!weight_debugger::attach(pid)) {
    //     std::cerr << "Failed to attach to process with PID " << pid << std::endl;
    //     return 1;
    // }

    std::cout << "[+] Attached to PID: " << pid << std::endl;

    std::cout << "=== /proc/self/maps ===" << std::endl;
    print_proc_maps_with_pid(pid);
    
    uintptr_t addr;
    std::string addr_input;

    std::cout << "Enter address to inspect (hex, e.g. 0x7ffdf0000000): ";
    std::cin >> addr_input;

    // Support both 0x-prefixed and bare hex
    std::stringstream ss;
    ss << std::hex << addr_input;
    ss >> addr;

    size_t num_bytes = 32;

   
    std::vector<uint8_t> original = weight_debugger::peek(pid, addr, num_bytes);
    std::cout << "[+] Peeked " << num_bytes << " bytes from " << std::hex << addr << ":\n";
    for (size_t i = 0; i < original.size(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)original[i] << " ";
    }
    std::cout << std::dec << "\n";

    
    std::vector<uint8_t> modified = original;
    modified[0] ^= 0xFF;  // Flip first byte
    if (weight_debugger::poke(pid, addr, modified.data(), modified.size())) {
        std::cout << "[+] Poke succeeded. Modified first byte.\n";
    } else {
        std::cerr << "[-] Poke failed.\n";
    }

    weight_debugger::detach(pid);
    std::cout << "[+] Detached from PID " << pid << ", process resumed.\n";

    return 0;
}