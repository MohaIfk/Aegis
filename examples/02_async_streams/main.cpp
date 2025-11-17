#include <aegis/aegis.h>
#include <vector>
#include <iostream>
#include <string>

int main()
{
    try {
        const int ELEMENT_COUNT = 128;
        size_t bufferSize = ELEMENT_COUNT * sizeof(float);

        std::cout << "Creating Aegis context..." << std::endl;
        auto context = aegis::ComputeContext::Create();

        // Create Data and Buffers
        std::vector<float> h_data(ELEMENT_COUNT);
        for (int i = 0; i < ELEMENT_COUNT; ++i) h_data[i] = (float)i;
        std::vector<float> h_results(ELEMENT_COUNT, 0.0f);

        auto buffer = context->CreateBuffer(bufferSize, aegis::GpuBuffer::MemoryType::DEVICE_LOCAL);

        // Create Kernels
        std::cout << "Compiling kernels..." << std::endl;
        auto kernelA = context->CreateKernel("shader_A.hlsl", "main_A");
        auto kernelB = context->CreateKernel("shader_B.hlsl", "main_B");
        if (!kernelA || !kernelB) {
            throw std::runtime_error("Failed to compile kernels.");
        }

        // Create Streams and Event
        auto streamA = context->CreateStream();
        auto streamB = context->CreateStream();
        auto event = context->CreateEvent();

        // Record Work on Stream A
        std::cout << "Recording work for Stream A..." << std::endl;
        streamA->ResourceUpload(*buffer, h_data.data(), bufferSize);
        streamA->SetKernel(*kernelA);
        streamA->SetBuffer(0, *buffer);
        streamA->RecordDispatch(2, 1, 1); // (128 / 64 threads = 2 groups)

        // When Stream A is done, signal the event
        streamA->RecordEvent(*event);

        // Record Work on Stream B
        std::cout << "Recording work for Stream B..." << std::endl;

        // Wait for the event from Stream A before starting
        streamB->StreamWait(*event);

        streamB->SetKernel(*kernelB);
        streamB->SetBuffer(0, *buffer);
        streamB->RecordDispatch(2, 1, 1);
        streamB->ResourceDownload(h_results.data(), *buffer, bufferSize);

        // Submit All Work
        std::cout << "Submitting work..." << std::endl;
        // We submit A first, but B is already waiting.
        streamA->Submit();
        streamB->Submit();

        // Wait for the FINAL result
        std::cout << "Waiting for Stream B to finish..." << std::endl;
        streamB->HostWait(); // This will only finish after A is done

        std::cout << "Work finished! Verifying..." << std::endl;

        // Verify
        bool success = true;
        for (int i = 0; i < ELEMENT_COUNT; ++i) {
            float expected = ((float)i + 10.0f) * 2.0f;
            if (h_results[i] != expected) {
                std::cerr << "Verification FAILED at index " << i << "! "
                          << "Expected " << expected << ", got " << h_results[i] << std::endl;
                success = false;
                break;
            }
        }
        if (success) {
            std::cout << "Verification SUCCEEDED!" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}