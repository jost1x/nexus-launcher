cmake_minimum_required(VERSION 3.20)
project(nexus-ui LANGUAGES C CXX)

option(NEXUS_UI_FBDEV "Render directly to Linux /dev/fb0 instead of opening an SDL window" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config/lv_conf.h" CACHE STRING "LVGL configuration")
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
set(LV_USE_SDL ON CACHE BOOL "" FORCE)

find_package(SDL2 CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Cross builds run the generators on the build host while compiling the generated
# sources for the target architecture. Native builds use the package targets.
set(NEXUS_PROTOC_EXECUTABLE "" CACHE FILEPATH "Host protoc executable for cross builds")
set(NEXUS_GRPC_CPP_PLUGIN_EXECUTABLE "" CACHE FILEPATH "Host grpc_cpp_plugin executable for cross builds")
if(NEXUS_PROTOC_EXECUTABLE)
    set(NEXUS_PROTOC_COMMAND "${NEXUS_PROTOC_EXECUTABLE}")
else()
    set(NEXUS_PROTOC_COMMAND "$<TARGET_FILE:protobuf::protoc>")
endif()
if(NEXUS_GRPC_CPP_PLUGIN_EXECUTABLE)
    set(NEXUS_GRPC_CPP_PLUGIN_COMMAND "${NEXUS_GRPC_CPP_PLUGIN_EXECUTABLE}")
else()
    set(NEXUS_GRPC_CPP_PLUGIN_COMMAND "$<TARGET_FILE:gRPC::grpc_cpp_plugin>")
endif()

set(PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../packages/proto")
set(PROTO_FILE "${PROTO_ROOT}/nexus/v1/nexus.proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(PROTO_SOURCES
    "${PROTO_GEN_DIR}/nexus/v1/nexus.pb.cc"
    "${PROTO_GEN_DIR}/nexus/v1/nexus.grpc.pb.cc")
set(PROTO_HEADERS
    "${PROTO_GEN_DIR}/nexus/v1/nexus.pb.h"
    "${PROTO_GEN_DIR}/nexus/v1/nexus.grpc.pb.h")

add_custom_command(
    OUTPUT ${PROTO_SOURCES} ${PROTO_HEADERS}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROTO_GEN_DIR}"
    COMMAND ${NEXUS_PROTOC_COMMAND}
        --proto_path=${PROTO_ROOT}
        --cpp_out=${PROTO_GEN_DIR}
        --grpc_out=${PROTO_GEN_DIR}
        --plugin=protoc-gen-grpc=${NEXUS_GRPC_CPP_PLUGIN_COMMAND}
        "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
    VERBATIM)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG c98ab243621a2a948674da5339c15da88832f928 # v9.2.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lvgl)

add_executable(nexus-ui
    src/app/main.cpp
    src/platform/controller_input.cpp
    src/platform/display_backend.cpp
    src/platform/memory_monitor.cpp
    src/ui/ui.cpp
    src/ui/ui_navigation.cpp
    src/ui/ui_input.cpp
    src/ui/ui_rpc.cpp
    src/ui/ui_controls.cpp
    src/ui/views/views.cpp
    src/ui/views/view_shell.cpp
    src/ui/views/views_home.cpp
    src/ui/views/views_library.cpp
    src/ui/views/views_settings.cpp
    src/ui/views/view_wifi.cpp
    src/ui/views/view_controls.cpp
    src/ui/views/view_system.cpp
    src/ui/views/view_about.cpp
    src/ui/components/components.cpp
    src/rpc/rpc_client.cpp
    src/rpc/rpc_hardware_watch.cpp
    src/rpc/rpc_wifi_watch.cpp
    src/rpc/rpc_operations.cpp
    src/assets/icons/lucide_icons.c
    src/assets/fonts/nexus_font_16.c
    src/assets/fonts/nexus_font_28.c
    ${PROTO_SOURCES})
target_link_libraries(nexus-ui PRIVATE lvgl SDL2::SDL2 gRPC::grpc++ protobuf::libprotobuf)
target_include_directories(nexus-ui PRIVATE "${PROTO_GEN_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(nexus-ui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config")
target_compile_definitions(nexus-ui PRIVATE LV_CONF_INCLUDE_SIMPLE)
if(NEXUS_UI_FBDEV)
    target_compile_definitions(nexus-ui PRIVATE NEXUS_UI_FBDEV=1)
endif()

