add_executable(HelloAsync main.cpp)

target_link_libraries(HelloAsync PRIVATE Aegis)

get_filename_component (DXC_DIR ${AEGIS_DXC_EXECUTABLE} DIRECTORY)

add_custom_command(
    TARGET HelloAsync POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Aegis>
    $<TARGET_FILE_DIR:HelloAsync>
    COMMENT "Copying Aegis.dll to executable directory"
)

add_custom_command(
    TARGET HelloAsync POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DXC_DIR}/dxcompiler.dll"
    $<TARGET_FILE_DIR:HelloAsync>
    COMMENT "Copying dxcompiler.dll to executable directory"
)

add_custom_command(
    TARGET HelloAsync POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DXC_DIR}/dxil.dll"
    $<TARGET_FILE_DIR:HelloAsync>
    COMMENT "Copying dxil.dll to executable directory"
)

add_custom_command(TARGET HelloAsync POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/shader_A.hlsl" $<TARGET_FILE_DIR:HelloAsync>)
add_custom_command(TARGET HelloAsync POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/shader_B.hlsl" $<TARGET_FILE_DIR:HelloAsync>)