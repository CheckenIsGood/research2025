#ifndef CHICKADEE_K_APIC_HH
#define CHICKADEE_K_APIC_HH

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

std::vector<uint8_t> weight_debugger::peek(uintptr_t addr, size_t num_bytes) {
    attach(name_of_model); // Ensure the model is attached
    std::vector<uint8_t> data(num_bytes);
    std::memcpy(data.data(), reinterpret_cast<void*>(addr), num_bytes);
    return data;
}

void weight_debugger::poke(uintptr_t addr, const std::vector<uint8_t>& bytes) {
    attach(name_of_model); // Ensure the model is attached
    std::memcpy(reinterpret_cast<void*>(addr), bytes.data(), bytes.size());
}

std::vector<uint8_t> weight_debugger::dump_weights() {
    attach(name_of_model); // Ensure the model is attached
    std::vector<uint8_t> flat;
    for (const auto& tensor : name_of_model->named_parameters()) 
    {
        const auto num_bytes = tensor.numel() * tensor.element_size();
        const void* src = tensor.data_ptr();
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(src);
        flat.insert(flat.end(), bytes, bytes + num_bytes);
    }
    return flat;
}

void weight_debugger::load_weights(const std::vector<uint8_t>& flat) {
    size_t offset = 0;
    for (const auto& tensor : name_of_model->named_parameters()) 
    {
        const auto num_bytes = tensor.numel() * tensor.element_size();
        std::memcpy(tensor.data_ptr(), flat.data() + offset, num_bytes);
        offset += num_bytes;
    }
}

#endif