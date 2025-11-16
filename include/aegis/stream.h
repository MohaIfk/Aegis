/**
 * @file stream.h
 * @brief ComputeStream class
 */

#pragma once

#include <memory> // for std::unique_ptr

namespace aegis::internal {
  class IComputeStream;
}

namespace aegis {
  class ComputeContext;
  class GpuBuffer;
  class ComputeKernel;
  class ComputeEvent;

  /**
   * @brief An asynchronous stream of compute commands (a "CUDA stream").
   *
   * This is the primary class for executing work. You record commands
   * (like dispatching kernels or copying data) into the stream,
   * and then submit the stream to the GPU for execution.
   */
  class ComputeStream {
  public:
    /**
     * @brief Destroys the stream.
     * @note This will wait for any pending work in this stream to complete.
     */
    ~ComputeStream();

    /**
     * @brief Records a command to dispatch a compute kernel.
     * @param kernel The kernel to run.
     * @param threadGroupsX Number of thread groups in the X dimension.
     * @param threadGroupsY Number of thread groups in the Y dimension.
     * @param threadGroupsZ Number of thread groups in the Z dimension.
     */
    void RecordDispatch(ComputeKernel& kernel, uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ);

    /**
     * @brief Records a command to copy data from one GPU buffer to another.
     * @param dest The destination buffer.
     * @param src The source buffer.
     */
    void ResourceCopyBuffer(GpuBuffer& dest, GpuBuffer& src);

    /**
     * @brief Records a command to upload data from the CPU to a GPU buffer.
     * @param dest The destination GPU buffer (must be DEVICE_LOCAL).
     * @param srcData A pointer to the CPU data to upload.
     * @param byteSize The size of the data to upload.
     */
    void ResourceUpload(GpuBuffer& dest, const void* srcData, size_t byteSize);

    /**
     * @brief Records a command to download data from a GPU buffer to the CPU.
     * @param destData A pointer to the CPU memory to receive the data.
     * @param src The source GPU buffer (must be DEVICE_LOCAL).
     * @param byteSize The size of the data to download.
     */
    void ResourceDownload(void* destData, GpuBuffer& src, size_t byteSize);

    /**
     * @brief Binds a GPU buffer to a specific shader register (e.g., u0, u1).
     * @param slot The register slot.
     * @param buffer The buffer to bind.
     */
    void SetBuffer(uint32_t slot, GpuBuffer& buffer);

    /**
     * @brief Submits all recorded commands to the GPU for execution.
     *
     * This returns immediately, and the work executes asynchronously.
     * This also resets the stream, making it ready to record new commands.
     */
    void Submit();

    /**
     * @brief Blocks the C++ thread until all work in *this stream* is finished.
     *
     * This function also processes any pending downloads from RecordDownload().
     */
    void HostWait();

    /**
     * @brief Records a command for this stream to wait for an event.
     * @param event The event to wait on.
     */
    void StreamWait(ComputeEvent& event);

    /**
     * @brief Records a command for this stream to signal an event when it
     * reaches this point.
     * @param event The event to signal.
     */
    void RecordEvent(ComputeEvent& event);

    /**
     * @brief Gets the internal backend implementation.
     * @note For internal use by other Flux classes.
     */
  internal::IComputeStream* GetBackendStream() { return m_backendStream.get(); }
  private:
    friend class ComputeContext;

    /**
     * @brief Private constructor.
     * @param context The context that owns this stream.
     * @param backendStream The private implementation (e.g., D3D12Stream).
     */
    ComputeStream(ComputeContext* context, std::unique_ptr<internal::IComputeStream> backendStream);

    ComputeContext* m_context;
    std::unique_ptr<internal::IComputeStream> m_backendStream;
  };

}