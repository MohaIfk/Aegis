/**
 * @file context.h
 * @brief ComputeContext class
 */

#pragma once
#include <string>
#include <memory> // for std::unique_ptr

#include "aegis/buffer.h"

namespace aegis {
  class ComputeStream;
  class GpuBuffer;
  class ComputeKernel;
  class ComputeEvent;
}

namespace aegis::internal {
  class IComputeBackend;
}

namespace aegis {
  class ComputeContext {
  public:
    /**
     * @brief Destroys the compute context and all associated resources.
     * @note This will block until the GPU is idle.
     */
    ~ComputeContext();

    /**
     * @brief Creates and initializes a new ComputeContext.
     * @return A unique_ptr to the new ComputeContext, or nullptr if
     * initialization fails (e.g., no compatible GPU found).
     */
    static std::unique_ptr<ComputeContext> Create();

    /**
     * @brief Creates a new asynchronous compute stream.
     * @return A new ComputeStream object.
     */
    std::unique_ptr<ComputeStream> CreateStream();

    /**
     * @brief Creates a new synchronization event.
     * @return A new ComputeEvent object.
     */
    std::unique_ptr<ComputeEvent> CreateEvent();

    /**
     * @brief Creates a new GPU memory buffer.
     * @param byteSize The size of the buffer in bytes.
     * @param memoryType The type of memory (Default, Upload, Readback).
     * @return A new GpuBuffer object.
     */
    std::unique_ptr<GpuBuffer> CreateBuffer(
        size_t byteSize,
        // TODO: define this enum in a common header
        // For now, i will assume 0 = DEVICE_LOCAL, 1 = UPLOAD, 2 = READBACK
        GpuBuffer::MemoryType memoryType
    );

    /**
     * @brief Compiles an HLSL shader and creates a compute kernel.
     * @param hlslFilePath Path to the .hlsl shader file.
     * @param entryPoint The name of the [shader("compute")] function.
     * @return A new ComputeKernel object, or nullptr on compilation failure.
     */
    std::unique_ptr<ComputeKernel> CreateKernel(
        const std::string& hlslFilePath,
        const std::string& entryPoint);

    /**
     * @brief Blocks the CPU thread until all submitted work on all streams
     * is finished.
     */
    void WaitForIdle();

    /**
     * @brief Gets the internal backend implementation.
     * @note This is for internal use by other Aegis classes (Stream, Buffer)
     * so they can call their backend counterparts.
     */
    internal::IComputeBackend* GetBackend() const { return m_backend.get(); }

  private:
    /**
     * @brief Private constructor. Use ComputeContext::Create().
     * @param backend A unique_ptr to a concrete backend implementation
     * (e.g., D3D12Backend).
     */
    ComputeContext(std::unique_ptr<internal::IComputeBackend> backend);

    /**
     * @brief The private implementation (e.g., D3D12Backend or VulkanBackend).
     */
    std::unique_ptr<internal::IComputeBackend> m_backend;
  };
}
