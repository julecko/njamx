cmake_minimum_required(VERSION 3.18)

# When stable set project version to 1.0.0
project(njamx VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_C_COMPILER)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

if (CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_C_COMPILER)
    message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID}")
endif()
if (CMAKE_CXX_COMPILER)
    message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if(WIN32)
    # Npcap's SDK (headers + wpcap.lib/Packet.lib) can't be redistributed
    # via vcpkg/apt due to its license, so it's fetched directly from
    # npcap.com the first time, same as the well-known FindPCAP.cmake
    # convention used by tcpdump/Wireshark (PCAP_ROOT-style layout).
    set(NPCAP_SDK_DIR "${CMAKE_SOURCE_DIR}/third_party/npcap-sdk" CACHE PATH
        "Path to an extracted Npcap SDK (Include/ and Lib/ subfolders)")

    if (NOT EXISTS "${NPCAP_SDK_DIR}/Include/pcap.h")
        set(NPCAP_SDK_URL "https://npcap.com/dist/npcap-sdk-1.16.zip")
        set(NPCAP_SDK_ZIP "${CMAKE_BINARY_DIR}/npcap-sdk.zip")
        message(STATUS "Npcap SDK not found at ${NPCAP_SDK_DIR}, downloading from ${NPCAP_SDK_URL} ...")
        file(DOWNLOAD "${NPCAP_SDK_URL}" "${NPCAP_SDK_ZIP}" STATUS NPCAP_DL_STATUS)
        list(GET NPCAP_DL_STATUS 0 NPCAP_DL_CODE)
        if (NOT NPCAP_DL_CODE EQUAL 0)
            message(FATAL_ERROR
                "Could not download the Npcap SDK automatically. Download it "
                "yourself from https://npcap.com/#download and extract it to "
                "${NPCAP_SDK_DIR}, or point -DNPCAP_SDK_DIR=... at an existing copy.")
        endif()
        file(ARCHIVE_EXTRACT INPUT "${NPCAP_SDK_ZIP}" DESTINATION "${NPCAP_SDK_DIR}")
    endif()
else()
    find_package(Threads REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PCAP REQUIRED libpcap)
endif()

set(SOURCES)

if (CMAKE_C_COMPILER)
    list(APPEND SOURCES
        src/main.c
        src/cli_args.c
        src/network/device.c
        src/network/network.c
        src/network/socket.c
        src/network/arp.c
        src/network/scanner.c
        src/network/iface.c
        src/network/forgery.c
        src/process/cli.c
        src/process/cli_commands.c
        src/process/jammer.c
        src/tools/mac_change.c
        src/tools/privileges.c
    )
    if (WIN32)
        list(APPEND SOURCES
            src/platform/win_adapter.c
        )
    endif()
endif()

if (CMAKE_CXX_COMPILER)
    list(APPEND SOURCES
        # src/main.cpp
        # add other C++ files here
    )
endif()

# Default exec name
if(NOT EXECUTABLE_OUTPUT_NAME)
    set(EXECUTABLE_OUTPUT_NAME "njamx")
endif()

add_executable(njamx ${SOURCES})

set_target_properties(njamx PROPERTIES OUTPUT_NAME ${EXECUTABLE_OUTPUT_NAME})

if(WIN32)
    target_include_directories(njamx PRIVATE "${NPCAP_SDK_DIR}/Include")
    target_link_directories(njamx PRIVATE "${NPCAP_SDK_DIR}/Lib/x64")
    target_link_libraries(njamx PRIVATE wpcap Packet ws2_32 iphlpapi)
else()
    target_include_directories(njamx PRIVATE ${PCAP_INCLUDE_DIRS})
    target_link_libraries(njamx PRIVATE ${PCAP_LIBRARIES} Threads::Threads)
endif()

target_include_directories(njamx PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(njamx PRIVATE
    NJAMX_VERSION=\"${PROJECT_VERSION}\"
)

if(WIN32)
    target_compile_definitions(njamx PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# ==============================
# Compiler-specific configuration
# ==============================

if (MSVC)

    # C11 stdatomic.h support is opt-in on MSVC.
    target_compile_options(njamx PRIVATE
        $<$<COMPILE_LANGUAGE:C>:/experimental:c11atomics>
    )

    # Debug
    target_compile_options(njamx PRIVATE
        $<$<CONFIG:Debug>:/Zi /Od>
    )
    target_link_options(njamx PRIVATE
        $<$<CONFIG:Debug>:/DEBUG>
    )
    target_compile_definitions(njamx PRIVATE
        $<$<CONFIG:Debug>:DEBUG_MODE=1>
    )

    # Release
    target_compile_options(njamx PRIVATE
        $<$<CONFIG:Release>:/O2>
    )
    target_compile_definitions(njamx PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
    )

else() # GCC / Clang

    # Debug
    target_compile_options(njamx PRIVATE
        $<$<CONFIG:Debug>:-g -O0 -fno-omit-frame-pointer>
    )
    target_compile_definitions(njamx PRIVATE
        $<$<CONFIG:Debug>:DEBUG_MODE=1>
    )

    # Optional ASAN (ONLY here)
    target_compile_options(njamx PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
    )
    target_link_options(njamx PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
    )

    # Release
    target_compile_options(njamx PRIVATE
        $<$<CONFIG:Release>:-O3>
    )
    target_compile_definitions(njamx PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
    )

endif()
