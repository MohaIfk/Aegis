set(AEGIS_CORE_SOURCE
        aegis_context.cpp
)

add_library(Aegis ${AEGIS_CORE_SOURCE})

# target_sources(Aegis PRIVATE ${AEGIS_CORE_SOURCE})

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