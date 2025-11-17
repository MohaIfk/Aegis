set(AEGIS_CORE_SOURCE
        aegis_context.cpp
        aegis_buffer.cpp
        aegis_kernel.cpp
        aegis_event.cpp
        aegis_stream.cpp
)

add_library(Aegis ${AEGIS_CORE_SOURCE})

if (BUILD_SHARED_LIBS)
    target_compile_definitions(Aegis PRIVATE AEGIS_BUILD_DLL)
endif ( )

target_include_directories(Aegis
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

if (AEGIS_BUILD_BACKEND_D3D121)

    target_compile_definitions(Aegis PRIVATE AEGIS_ENABLE_D3D12)

    set(AEGIS_D3D12_SOURCES
            backend/d3d12/d3d12_backend.cpp
            backend/d3d12/d3d12_buffer.cpp
            backend/d3d12/d3d12_kernel.cpp
            backend/d3d12/d3d12_event.cpp
            backend/d3d12/d3d12_stream.cpp
    )

    target_sources(Aegis PRIVATE ${AEGIS_D3D12_SOURCES})

    target_link_libraries(Aegis PRIVATE
            d3d12
            dxgi
            dxcompiler
    )

    target_include_directories(Aegis PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/backend/d3d12
    )

endif ()

if (AEGIS_BUILD_BACKEND_VULKAN)
endif ()