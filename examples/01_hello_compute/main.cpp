#include <aegis/aegis.h> // The main library header
#include <vector>
#include <iostream>
#include <string>

std::string getShaderPath() {
    return "add_vectors.hlsl";
}

int main()
{
  try {
    const int ELEMENT_COUNT = 128;
    size_t bufferSize = ELEMENT_COUNT * sizeof(float);

    // Initialize the "Aegis" library
    std::cout << "Creating Aegis context..." << std::endl;
    std::unique_ptr<aegis::ComputeContext> context = aegis::ComputeContext::Create();
    if (!context) {
      std::cerr << "Failed to create Aegis compute context!" << std::endl;
      return 1;
    }

    // Create Data and "Allocate Variables" (Buffers)
    std::cout << "Allocating buffers..." << std::endl;

    // Create host-side (CPU) data
    std::vector<float> dataA(ELEMENT_COUNT);
    std::vector<float> dataB(ELEMENT_COUNT);
    std::vector<float> dataC_results(ELEMENT_COUNT, 0.0f);

    for (int i = 0; i < ELEMENT_COUNT; ++i) {
      dataA[i] = static_cast<float>(i);
      dataB[i] = static_cast<float>(i * 2);
    }

    // Create GPU-side buffers
    auto bufferA = context->CreateBuffer(bufferSize, aegis::GpuBuffer::MemoryType::DEVICE_LOCAL);
    auto bufferB = context->CreateBuffer(bufferSize, aegis::GpuBuffer::MemoryType::DEVICE_LOCAL);
    auto bufferC = context->CreateBuffer(bufferSize, aegis::GpuBuffer::MemoryType::DEVICE_LOCAL);

    // "Register Function" (Compile Kernel)
    std::cout << "Compiling kernel..." << std::endl;
    auto kernel = context->CreateKernel(getShaderPath(), "main_cs");
    if (!kernel) {
      std::cerr << "Failed to create compute kernel!" << std::endl;
      return 1;
    }

    // Create a Stream and Record Work
    std::cout << "Recording work..." << std::endl;
    auto stream = context->CreateStream();

    // Upload initial data
    stream->ResourceUpload(*bufferA, dataA.data(), bufferSize);
    stream->ResourceUpload(*bufferB, dataB.data(), bufferSize);

    // Set the kernel.
    stream->SetKernel(*kernel);

    // Set up the kernel for execution
    stream->SetBuffer(0, *bufferA); // Bind bufferA to u0
    stream->SetBuffer(1, *bufferB); // Bind bufferB to u1
    stream->SetBuffer(2, *bufferC); // Bind bufferC to u2

    // We have 128 elements and 64 threads/group, so we need 2 groups.
    stream->RecordDispatch(2, 1, 1);

    // Download the results
    stream->ResourceDownload(dataC_results.data(), *bufferC, bufferSize);

    // Submit and Wait
    std::cout << "Submitting work to GPU." << std::endl;
    stream->Submit();
    std::cout << "Waiting for the GPU..." << std::endl;
    stream->HostWait(); // Wait for this stream to finish

    std::cout << "Work finished!" << std::endl;

    // Verify Results
    bool success = true;
    for (int i = 0; i < ELEMENT_COUNT; ++i) {
      float expected = dataA[i] + dataB[i];
      if (dataC_results[i] != expected) {
        std::cerr << "Verification FAILED at index " << i << "! "
                  << "Expected " << expected << ", got " << dataC_results[i] << std::endl;
        success = false;
        break;
      }
    }

    if (success) {
      std::cout << "Verification SUCCEEDED!" << std::endl;
      std::cout << "Example: [0] " << dataC_results[0] << ", [1] " << dataC_results[1] << "..." << std::endl;
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "An exception occurred! " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "An unknown exception occurred!" << std::endl;
  }
}