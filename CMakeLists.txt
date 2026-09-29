cmake_minimum_required(VERSION 3.24)
project(H1External LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Build using Visual Studio C++ on Windows, x64.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(D3DX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.deps/d3dx" CACHE PATH "Microsoft.DXSDK.D3DX extracted package")
if(NOT EXISTS "${D3DX_ROOT}/build/native/include/d3dx9.h")
    message(FATAL_ERROR "Run build.ps1 to download the pinned Microsoft D3DX package first.")
endif()
set(IMGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.deps/imgui-1.91.9b")
if(NOT EXISTS "${IMGUI_ROOT}/imgui.cpp")
    message(FATAL_ERROR "Run build.ps1 to download the pinned Dear ImGui package first.")
endif()
add_library(imgui STATIC
    "${IMGUI_ROOT}/imgui.cpp" "${IMGUI_ROOT}/imgui_draw.cpp"
    "${IMGUI_ROOT}/imgui_tables.cpp" "${IMGUI_ROOT}/imgui_widgets.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_win32.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_dx9.cpp")
target_include_directories(imgui PUBLIC "${IMGUI_ROOT}" "${IMGUI_ROOT}/backends")
target_compile_definitions(imgui PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(imgui PUBLIC d3d9 dwmapi)
add_executable(h1_updated Lol1.cpp Lol2.cpp Gui.cpp)
target_include_directories(h1_updated SYSTEM PRIVATE "${D3DX_ROOT}/build/native/include")
target_compile_definitions(h1_updated PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(h1_updated PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(h1_updated PRIVATE imgui d3d9 dwmapi advapi32 user32 gdi32
    "${D3DX_ROOT}/build/native/release/lib/x64/d3dx9.lib")
set_target_properties(h1_updated PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist")
add_custom_command(TARGET h1_updated POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${D3DX_ROOT}/build/native/release/bin/x64/D3DX9_43.dll"
        "$<TARGET_FILE_DIR:h1_updated>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${D3DX_ROOT}/build/native/release/bin/x64/D3DCompiler_43.dll"
        "$<TARGET_FILE_DIR:h1_updated>")
# Do not replace the user's settings on subsequent builds.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist/Release")
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dist/Release/settings.ini")
    configure_file(settings.ini "${CMAKE_CURRENT_SOURCE_DIR}/dist/Release/settings.ini" COPYONLY)
endif()
enable_testing()
add_executable(logic_tests tests/logic_tests.cpp)
target_include_directories(logic_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(logic_tests PRIVATE /W4 /permissive- /utf-8)
add_test(NAME runtime_logic COMMAND logic_tests)
add_executable(memory_tests tests/memory_tests.cpp)
target_include_directories(memory_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(memory_tests SYSTEM PRIVATE "${D3DX_ROOT}/build/native/include")
target_compile_definitions(memory_tests PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(memory_tests PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(memory_tests PRIVATE "${D3DX_ROOT}/build/native/release/lib/x64/d3dx9.lib")
add_test(NAME memory_reads COMMAND memory_tests)
add_executable(gui_tests tests/gui_tests.cpp Gui.cpp Lol1.cpp)
target_include_directories(gui_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(gui_tests SYSTEM PRIVATE "${D3DX_ROOT}/build/native/include")
target_compile_definitions(gui_tests PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(gui_tests PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(gui_tests PRIVATE imgui d3d9 dwmapi user32 gdi32
    "${D3DX_ROOT}/build/native/release/lib/x64/d3dx9.lib")
add_custom_command(TARGET gui_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${D3DX_ROOT}/build/native/release/bin/x64/D3DX9_43.dll"
        "$<TARGET_FILE_DIR:gui_tests>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${D3DX_ROOT}/build/native/release/bin/x64/D3DCompiler_43.dll"
        "$<TARGET_FILE_DIR:gui_tests>")
add_test(NAME gui_interaction COMMAND gui_tests "${CMAKE_CURRENT_SOURCE_DIR}/docs")
add_subdirectory(offline)
