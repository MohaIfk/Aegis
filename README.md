# Aegis

Hey! This is Aegis. It's my personal project to build a CUDA-like compute library using C++ and Direct3D 12.

I've always loved how simple it is to get started with CUDA, but I really wanted to build something that could run on any D3D12 GPU (NVIDIA, AMD, Intel). I was curious how much of the insane D3D12 complexity I could hide behind a clean, simple API.

This is a total **work-in-progress** and a massive learning project for me, but it's finally working, and I'm pretty proud of it.

# The Goal

The idea was to make D3D12 compute feel like CUDA:

- Give me an API with Streams, Buffers, and Kernels.
- Let me just write simple HLSL shaders.
- Handle all the awful D3D12 boilerplate automatically. I'm talking barriers, command lists, PSOs, root signatures... all the stuff that takes hundreds of lines of code.

# What Works So Far

It's still early, but the core architecture is alive!

- [x] You can create a `ComputeContext`.
- [x] You can `CreateBuffers` on the GPU.
- [x] You can `CreateKernel` just by pointing it at an `.hlsl` file. It compiles it at runtime and auto-reflects the shader to build its own Root Signature (this part was a nightmare, lol).
- [x] You can create a `ComputeStream` to record all your work.
- [x] `RecordUpload` and `RecordDownload` get data to and from the GPU.
- [x] `SetKernel`, `SetBuffer`, and `RecordDispatch` run your code.
- [x] `HostWait` actually waits for the GPU and copies the data back!
- [x] Real Async! `StreamWait` and `RecordEvent` are now fully implemented with `ID3D12Fences`. You can properly synchronize work between multiple streams.

# Look! It's Working!

This is what the API looks like in the HelloCompute example. I'm so happy with how clean this turned out.

```c++
// main.cpp
#include <aegis/aegis.h> // My main header
#include <vector>
#include <iostream>

int main()
{
    // Initialize Aegis
    auto context = aegis::ComputeContext::Create();

    // Create some data and GPU buffers
    std::vector<float> dataA = { ... };
    std::vector<float> dataB = { ... };
    std::vector<float> dataC_results(ELEMENT_COUNT);

    auto bufferA = context->CreateBuffer(size, aegis::GpuBuffer::MemoryType::DEVICE_LOCAL);
    auto bufferB = context->CreateBuffer(size, aegis::GpuBuffer::MemoryType::DEVICE_LOCAL);
    auto bufferC = context->CreateBuffer(size, aegis::GpuBuffer::MemoryType::DEVICE_LOCAL);

    // Compile the kernel (this is the magic part)
    auto kernel = context->CreateKernel("add_vectors.hlsl", "main_cs");

    // Get a stream and record all the work
    auto stream = context->CreateStream();

    stream->RecordUpload(*bufferA, dataA.data(), size);
    stream->RecordUpload(*bufferB, dataB.data(), size);

    // This is the "CUDA" bit
    stream->SetKernel(*kernel);
    stream->SetBuffer(0, *bufferA); // Binds to u0
    stream->SetBuffer(1, *bufferB); // Binds to u1
    stream->SetBuffer(2, *bufferC); // Binds to u2
    stream->RecordDispatch(2, 1, 1); // Run 2 thread groups

    stream->RecordDownload(dataC_results.data(), *bufferC, size);

    // Go
    stream->Submit();
    stream->HostWait(); // Blocks until GPU is done and copies data

    std::cout << "Work finished!" << std::endl;
    // verification code
    return 0;
}
```

And the shader is just plain HLSL, no tricks:

```hlsl
// add_vectors.hlsl
RWStructuredBuffer<float> a : register(u0);
RWStructuredBuffer<float> b : register(u1);
RWStructuredBuffer<float> c : register(u2);

[numthreads(64, 1, 1)]
void main_cs(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint i = dispatchThreadID.x;
    c[i] = a[i] + b[i];
}
```

# Building

It's just a standard CMake project.

- You'll need the Windows 10/11 SDK (for `D3D12`) and the **DXC** compiler (`dxcompiler.dll`).
- git clone https://github.com/Mohaifk/Aegis.git
- mkdir build && cd build
- cmake .. (I'm building it as a static lib, so BUILD_SHARED_LIBS=OFF is the default).
- Build the HelloCompute example in Visual Studio.
- The build script should copy `dxcompiler.dll`, `dxil.dll`, and `add_vectors.hlsl` into the exe directory.
- Run `HelloCompute.exe` from the build folder. It should just work!

# Future / TODO

This is just the beginning. There's a lot of stuff that's super inefficient and needs to be fixed.

- [ ] **Vulkan Backend**: This is the big one. The whole design was for a pluggable backend, so I really want to add a Vulkan backend to make this cross-platform.
- [ ] **Upload Heaps**: `RecordUpload` and `RecordDownload` create a new temp buffer every single time. This is terrible for performance. I need to build a proper ring-buffer or upload heap.
- [ ] **Descriptor Heaps**: The root signature part is a simple hack that only supports root UAVs. This is not how you're supposed to do it. It needs to use real descriptor heaps to support hundreds of resources, constant buffers (CBVs), SRVs, etc.

*Anyway, that's it for now. I'm just happy it's not crashing anymore.*