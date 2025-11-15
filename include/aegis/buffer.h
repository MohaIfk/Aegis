/**
 * @file buffer.h
 * @brief GpuBuffer class
 */

#pragma once

#include <memory> // for std::unique_ptr

namespace aegis::internal {
  class IGpuBuffer;
}

namespace aegis {
  class ComputeContext;

  /**
   * @brief Represents a block of memory on the GPU (a "variable").
   *
   * This is a handle to a GPU resource. It can be used to upload data,
   * download data, and be bound to a ComputeKernel for processing.
   */
  class GpuBuffer {
  public:
    /**
     * @brief Destroys the GPU buffer.
     */
    ~GpuBuffer();

    /**
     * @brief Gets the size of the buffer in bytes.
     * @return size_t The total size of the buffer.
     */
    [[nodiscard]] size_t GetSizeInBytes() const;

    /**
     * @brief Maps the buffer's memory for CPU access.
     *
     * @warning This is a blocking operation and should only be used
     * on buffers created with UPLOAD or READBACK memory types.
     *
     * @return void* A CPU-writable/readable pointer to the buffer's memory.
     */
    void* Map();

    /**
     * @brief Unmaps the buffer's memory.
     * Must be called after Map() when CPU access is finished.
     */
    void Unmap();

    /**
     * @brief Defines the type of memory for a GPU buffer.
     * This is a hint for the backend to optimize memory placement.
     */
    enum class MemoryType {
      /** @brief Default GPU-only memory. Fast for GPU R/W, inaccessible by CPU. */
      DEVICE_LOCAL,
      /** @brief CPU-visible memory for uploading data (CPU-to-GPU). */
      UPLOAD,
      /** @brief CPU-visible memory for reading data back (GPU-to-CPU). */
      READBACK
    };

    /**
     * @brief Gets the internal backend implementation.
     * @note For internal use by other Flux classes.
     */
    internal::IGpuBuffer* GetBackendBuffer() { return m_backendBuffer.get(); }
  private:
    friend class ComputeContext;

    /**
     * @brief Private constructor.
     * @param context The context that owns this buffer.
     * @param backendBuffer The private implementation (e.g., D3D12Buffer).
     */
    GpuBuffer(ComputeContext* context, std::unique_ptr<internal::IGpuBuffer> backendBuffer);

    ComputeContext* m_context;
    std::unique_ptr<internal::IGpuBuffer> m_backendBuffer;
  };
}