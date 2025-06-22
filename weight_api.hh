#ifndef WEIGHT_API_HH
#define WEIGHT_API_HH

#include <torch/torch.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace weight_debugger {

    // Attach debugger to a model and cache named weight pointers
    void attach(const torch::nn::Module& model);

    // Read the value at a specific weight address
    std::vector<uint8_t> peek(uintptr_t addr, size_t num_bytes);

    // Overwrite weight memory with new data at a given address
    void poke(uintptr_t addr, const std::vector<uint8_t>& bytes);

    // Serialize all weights in the model into a flat byte buffer
    std::vector<uint8_t> dump_weights();

    // Load weights from a flat byte buffer back into the model
    void load_weights(const std::vector<uint8_t>& flat_bytes);
    
    // Map from address to weight name (for annotation)
    const std::map<uintptr_t, std::string>& weight_map();

}

#endif