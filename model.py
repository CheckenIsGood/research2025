import torch
import torch.nn as nn
import torchvision.transforms as transforms
import torchvision.datasets as datasets
from torch.utils.data import DataLoader
import ctypes

# Disable GPU
# device = torch.device("cpu")

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"Using device: {device}")

# Simple Residual Block
class ResidualBlock(nn.Module):
    def __init__(self, in_channels, out_channels, stride=1):
        super().__init__()
        self.conv1 = nn.Conv2d(in_channels, out_channels, kernel_size=3,
                               stride=stride, padding=1, bias=False)
        self.bn1 = nn.BatchNorm2d(out_channels)
        self.relu = nn.ReLU(inplace=True)

        self.conv2 = nn.Conv2d(out_channels, out_channels, kernel_size=3,
                               stride=1, padding=1, bias=False)
        self.bn2 = nn.BatchNorm2d(out_channels)

        # Skip connection adjustment
        self.skip = nn.Sequential()
        if stride != 1 or in_channels != out_channels:
            self.skip = nn.Sequential(
                nn.Conv2d(in_channels, out_channels, kernel_size=1,
                          stride=stride, bias=False),
                nn.BatchNorm2d(out_channels)
            )

    def forward(self, x):
        out = self.relu(self.bn1(self.conv1(x)))
        out = self.bn2(self.conv2(out))
        out += self.skip(x)
        return self.relu(out)

# Mini ResNet
class SimpleResNet(nn.Module):
    def __init__(self, num_classes=10):
        super().__init__()
        self.layer1 = nn.Sequential(
            nn.Conv2d(3, 16, kernel_size=3, stride=1, padding=1, bias=False),
            nn.BatchNorm2d(16),
            nn.ReLU(inplace=True)
        )
        self.res1 = ResidualBlock(16, 16)
        self.res2 = ResidualBlock(16, 32, stride=2)
        self.pool = nn.AdaptiveAvgPool2d((1, 1))
        self.fc = nn.Linear(32, num_classes)

    def forward(self, x):
        out = self.layer1(x)
        out = self.res1(out)
        out = self.res2(out)
        out = self.pool(out)
        out = torch.flatten(out, 1)
        return self.fc(out)

# Load dummy image data
transform = transforms.Compose([
    transforms.Resize((64, 64)),
    transforms.ToTensor()
])
dataset = datasets.FakeData(transform=transform)
loader = DataLoader(dataset, batch_size=4)

# Initialize model and send to CPU
model = SimpleResNet().to(device)
model.eval()

print("\n=== Model Weight Virtual Memory Addresses ===")
for name, param in model.named_parameters():
    data_ptr = param.data.data_ptr()
    addr = hex(data_ptr)
    size = param.numel() * param.element_size()
    print(f"{name:<30} @ {addr} | size: {size} bytes | shape: {tuple(param.shape)}")

# Run a forward pass
with torch.no_grad():
    for images, labels in loader:
        images = images.to(device)
        outputs = model(images)
        print("Input shape:", images.shape)
        print("Output shape:", outputs.shape)
        break