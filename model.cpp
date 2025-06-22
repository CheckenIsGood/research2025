#include <torch/torch.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <unistd.h>

struct ResidualBlockImpl : torch::nn::Module {
    torch::nn::Conv2d conv1{nullptr}, conv2{nullptr}, skip_conv{nullptr};
    torch::nn::BatchNorm2d bn1{nullptr}, bn2{nullptr}, skip_bn{nullptr};
    bool use_skip;

    ResidualBlockImpl(int in_channels, int out_channels, int stride = 1) {
        conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels, out_channels, 3)
                                                           .stride(stride).padding(1).bias(false)));
        bn1 = register_module("bn1", torch::nn::BatchNorm2d(out_channels));
        conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(out_channels, out_channels, 3)
                                                           .stride(1).padding(1).bias(false)));
        bn2 = register_module("bn2", torch::nn::BatchNorm2d(out_channels));

        use_skip = (stride != 1) || (in_channels != out_channels);
        if (use_skip) {
            skip_conv = register_module("skip_conv", torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels, out_channels, 1)
                                                                       .stride(stride).bias(false)));
            skip_bn = register_module("skip_bn", torch::nn::BatchNorm2d(out_channels));
        }
    }

    torch::Tensor forward(torch::Tensor x) {
        auto out = torch::relu(bn1(conv1(x)));
        out = bn2(conv2(out));
        auto skip_out = use_skip ? skip_bn(skip_conv(x)) : x;
        return torch::relu(out + skip_out);
    }
};
TORCH_MODULE(ResidualBlock);

struct SimpleResNetImpl : torch::nn::Module {
    torch::nn::Sequential layer1;
    torch::nn::AdaptiveAvgPool2d pool{nullptr};
    ResidualBlock res1{nullptr}, res2{nullptr};
    torch::nn::Linear fc{nullptr};

    SimpleResNetImpl(int num_classes = 10) {
        layer1 = register_module("layer1", torch::nn::Sequential(
            torch::nn::Conv2d(torch::nn::Conv2dOptions(3, 16, 3).stride(1).padding(1).bias(false)),
            torch::nn::BatchNorm2d(16),
            torch::nn::ReLU(true)
        ));

        res1 = register_module("res1", ResidualBlock(16, 16));
        res2 = register_module("res2", ResidualBlock(16, 32, 2));

        pool = register_module("pool", torch::nn::AdaptiveAvgPool2d(torch::nn::AdaptiveAvgPool2dOptions({1, 1})));
        fc = register_module("fc", torch::nn::Linear(32, num_classes));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = layer1->forward(x);
        x = res1->forward(x);
        x = res2->forward(x);
        x = pool->forward(x);
        x = x.view({x.size(0), -1});
        return fc->forward(x);
    }
};
TORCH_MODULE(SimpleResNet);

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
    std::cout << "CUDA available? " << torch::cuda::is_available() << std::endl;
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    std::cout << "Using device: " << device << std::endl;

    SimpleResNet model;
    model->to(device);
    model->eval();

    std::cout << "\n=== Model Weight Virtual Memory Addresses ===\n";
    for (const auto& param : model->named_parameters()) 
    {
        void* addr = param.value().data_ptr();
        size_t size = param.value().numel() * param.value().element_size();
        std::cout << std::left << std::setw(30) << param.key()
                  << " @ " << addr
                  << " | size: " << size << " bytes"
                  << " | shape: " << param.value().sizes()
                  << std::endl;
    }

    auto inputs = torch::randn({4, 3, 64, 64}).to(device);
    auto outputs = model->forward(inputs);

    std::cout << "Input shape: " << inputs.sizes() << std::endl;
    std::cout << "Output shape: " << outputs.sizes() << std::endl;

    pid_t pid = getpid();
    std::cout << "Current PID: " << pid << std::endl;

    std::cout << "=== /proc/self/maps ===" << std::endl;
    print_proc_maps_with_pid(pid);

    return 0;
}