#include "weight_api.hh"
#include <sys/ptrace.h>
#include <map>
#include <vector>
#include <cstdint>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <fstream>
#include <iomanip>

namespace weight_debugger {

    static std::map<std::string, WeightEntry> name_to_entry;

    bool attach(pid_t pid) {
        if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1) {
            perror("ptrace attach");
            return false;
        }
        waitpid(pid, nullptr, 0); // wait for stop
        return true;
    }

    void detach(pid_t pid) {
        // Detach and resume execution with SIGCONT
        if (ptrace(PTRACE_DETACH, pid, nullptr, SIGCONT) == -1) {
            perror("ptrace detach");
        } else {
            std::cout << "Detached from process " << pid << " and resumed execution.\n";
        }
    }

    std::vector<uint8_t> peek(pid_t pid, uintptr_t addr, size_t num_bytes) {
        std::vector<uint8_t> buffer;
        buffer.reserve(num_bytes);

        size_t bytes_read = 0;
        while (bytes_read < num_bytes) {
            errno = 0;
            long word = ptrace(PTRACE_PEEKDATA, pid, addr + bytes_read, nullptr);
            if (word == -1 && errno != 0) {
                perror("ptrace peek");
                break;
            }

            size_t copy_size = std::min(sizeof(long), num_bytes - bytes_read);
            uint8_t* bytes = reinterpret_cast<uint8_t*>(&word);
            buffer.insert(buffer.end(), bytes, bytes + copy_size);
            bytes_read += copy_size;
        }

        return buffer;
    }

    
    bool poke(pid_t pid, uintptr_t remote_addr, const void* local_src, size_t num_bytes) {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(local_src);
        size_t i = 0;

        while (i < num_bytes) {
            uint64_t word = 0;
            size_t chunk = std::min(sizeof(long), num_bytes - i);
            std::memcpy(&word, src + i, chunk);

            if (ptrace(PTRACE_POKEDATA, pid, remote_addr + i, word) == -1) {
                perror("ptrace poke");
                return false;
            }
            i += chunk;
        }

        return true;
    }

    std::string dump_weights(pid_t pid) {
        std::string flat;

        for (const auto& [name, entry] : name_to_entry) {
            std::vector<uint8_t> data = peek(pid, entry.address, entry.size);
            flat.append(reinterpret_cast<const char*>(data.data()), data.size());
        }

        return flat;
    }

    bool load_weights(pid_t pid, const void* flat, size_t size) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(flat);
        size_t offset = 0;

        for (const auto& [name, entry] : name_to_entry) {
            if (offset + entry.size > size) {
                std::cerr << "Flat buffer too short for weight: " << name << std::endl;
                return false;
            }

            const void* src = data + offset;
            if (!poke(pid, entry.address, src, entry.size)) {
                std::cerr << "Failed to poke weight: " << name << std::endl;
                return false;
            }

            offset += entry.size;
        }

        if (offset != size) {
            std::cerr << "Warning: unused bytes remain in flat buffer." << std::endl;
        }

        return 1;
    }

    void set_weight_map(const std::map<std::string, WeightEntry>& map) {
        name_to_entry = map;
    }

    const std::map<std::string, WeightEntry>& weight_map() {
        return name_to_entry;
    }

    pid_t launch() {
        const char* program = "./simple_resnet";  // Hardcoded executable name

        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            return -1;
        }

        if (child_pid == 0) {
            // ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
            char* argv[] = { const_cast<char*>(program), nullptr };
            execvp(program, argv);
            perror("execvp"); // If exec fails
            _exit(1);
        } else {
            // Parent: wait for child to stop at exec
            int status;
            waitpid(child_pid, &status, 0);
            if (WIFSTOPPED(status)) {
                std::cout << "Child " << child_pid << " stopped at exec. Ready for ptrace.\n";
                return child_pid;
            } else {
                std::cerr << "Child did not stop correctly.\n";
                return -1;
            }
        }
    }


}