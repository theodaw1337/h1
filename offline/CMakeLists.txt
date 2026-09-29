add_executable(offline_client Console.cpp)
add_executable(offline_tests Tests.cpp)
add_executable(offline_lab View.cpp ../Gui.cpp ../Lol1.cpp)
foreach(target offline_client offline_tests offline_lab)
    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/offline")
endforeach()
target_include_directories(offline_lab SYSTEM PRIVATE "${D3DX_ROOT}/build/native/include")
target_link_libraries(offline_lab PRIVATE imgui d3d9 dwmapi user32 gdi32 "${D3DX_ROOT}/build/native/release/lib/x64/d3dx9.lib")
add_custom_command(TARGET offline_lab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${D3DX_ROOT}/build/native/release/bin/x64/D3DX9_43.dll" "$<TARGET_FILE_DIR:offline_lab>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${D3DX_ROOT}/build/native/release/bin/x64/D3DCompiler_43.dll" "$<TARGET_FILE_DIR:offline_lab>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${D3DX_ROOT}/LICENSE.txt" "$<TARGET_FILE_DIR:offline_lab>/D3DX-LICENSE.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${IMGUI_ROOT}/LICENSE.txt" "$<TARGET_FILE_DIR:offline_lab>/IMGUI-LICENSE.txt")
add_test(NAME offline_protocol COMMAND offline_tests)
add_test(NAME offline_render COMMAND offline_lab --capture "${CMAKE_SOURCE_DIR}/docs/offline-preview.png")
set_tests_properties(offline_render PROPERTIES TIMEOUT 30)
