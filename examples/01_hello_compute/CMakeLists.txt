add_executable(HelloCompute main.cpp)

target_link_libraries(HelloCompute PRIVATE Aegis)

get_filename_component (DXC_DIR ${AEGIS_DXC_EXECUTABLE} DIRECTORY)

add_custom_command(
    TARGET HelloCompute POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Aegis>
    $<TARGET_FILE_DIR:HelloCompute>
    COMMENT "Copying Aegis.dll to executable directory"
)

add_custom_command(
    TARGET HelloCompute POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DXC_DIR}/dxcompiler.dll"
    $<TARGET_FILE_DIR:HelloCompute>
    COMMENT "Copying dxcompiler.dll to executable directory"
)

add_custom_command(
    TARGET HelloCompute POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DXC_DIR}/dxil.dll"
    $<TARGET_FILE_DIR:HelloCompute>
    COMMENT "Copying dxil.dll to executable directory"
)

set(SHADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/add_vectors.hlsl")

add_custom_command(
    TARGET HelloCompute POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SHADER_FILE}"
    "$<TARGET_FILE_DIR:HelloCompute>/add_vectors.hlsl"
    COMMENT "Copying shader to executable directory"
)