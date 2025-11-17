/**
 * @file event.h
 * @brief ComputeEvent class
 */

#pragma once

#include <memory> // for std::unique_ptr
#include "api.h"

namespace aegis::internal {
  class IComputeEvent;
}

namespace aegis {
  class ComputeContext;

  /**
   * @brief A synchronization primitive for coordinating GPU work.
   *
   * An event can be recorded in a ComputeStream and waited on by another
   * stream or by the host (CPU). This is the primary tool for
   * managing asynchronous execution.
   */
  class AEGIS_API ComputeEvent {
  public:
    /**
     * @brief Destroys the event.
     */
    ~ComputeEvent();

    /**
     * @brief Gets the internal backend implementation.
     * @note For internal use by other Flux classes.
     */
    internal::IComputeEvent* GetBackendEvent() const { return m_backendEvent.get(); }
  private:
    friend class ComputeContext;

    /**
     * @brief Private constructor.
     * @param context The context that owns this event.
     * @param backendEvent The private implementation (e.g., D3D12Event).
     */
    ComputeEvent(ComputeContext* context, std::unique_ptr<internal::IComputeEvent> backendEvent);

    ComputeContext* m_context;
    std::unique_ptr<internal::IComputeEvent> m_backendEvent;
  };
}