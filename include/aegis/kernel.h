/**
 * @file kernel.h
 * @brief ComputeKernel class
 */

#pragma once

#include <memory> // for std::unique_ptr

namespace aegis::internal {
  class IComputeKernel;
}

namespace aegis {
  class ComputeContext;

  /**
   * @brief Represents a compiled compute shader "function" ready to be
   * executed on the GPU.
   *
   * This is a handle to a compiled kernel. It is created by
   * ComputeContext::CreateKernel and is bound to a ComputeStream to be run.
   */
  class ComputeKernel {
  public:
    /**
     * @brief Destroys the compute kernel.
     */
    ~ComputeKernel() = default;

    /**
     * @brief Gets the internal backend implementation.
     * @note For internal use by other Flux classes.
     */
    internal::IComputeKernel* GetBackendKernel() const { return m_backendKernel.get(); }
  private:
    friend class ComputeContext;

    /**
     * @brief Private constructor.
     * @param context The context that owns this kernel.
     * @param backendKernel The private implementation (e.g., D3D12Kernel).
     */
    ComputeKernel(ComputeContext* context, std::unique_ptr<internal::IComputeKernel> backendKernel);

    ComputeContext* m_context;
    std::unique_ptr<internal::IComputeKernel> m_backendKernel;
  };
}