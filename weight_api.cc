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

    static std::map<uintptr_t, std::string> weight_map_;

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

    bool poke(pid_t pid, uintptr_t addr, const std::vector<uint8_t>& bytes) {
        size_t i = 0;
        while (i < bytes.size()) {
            uint64_t word = 0;
            size_t chunk = std::min(sizeof(long), bytes.size() - i);
            std::memcpy(&word, &bytes[i], chunk);

            if (ptrace(PTRACE_POKEDATA, pid, addr + i, word) == -1) {
                perror("ptrace poke");
                return false;
            }
            i += chunk;
        }
        return true;
    }

    void set_weight_map(const std::map<uintptr_t, std::string>& map) {
        weight_map_ = map;
    }

    const std::map<uintptr_t, std::string>& weight_map() {
        return weight_map_;
    }

    pid_t launch() {
        const char* program = "./simple_resnet";  // Hardcoded executable name

        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            return -1;
        }

        if (child_pid == 0) {
            // Child: request to be traced and exec the program
            ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
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