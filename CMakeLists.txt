cmake_minimum_required(VERSION 3.18)

# When stable set project version to 1.0.0
project(njamx VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_C_COMPILER)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# CXX_STANDARD is set as a target property on njamx itself, further down
# (not here, globally) so it doesn't leak into FLTK's own subdirectory
# build below - FLTK isn't guaranteed to compile cleanly as C++23.

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if (CMAKE_C_COMPILER)
    message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID}")
endif()
if (CMAKE_CXX_COMPILER)
    message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if(WIN32)
    # Needed to compile njamx.rc below (embeds the app icon into the exe).
    enable_language(RC)

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

# Optional GUI (process/gui/, C++/FLTK): a window driving the same
# cli_run_command() dispatcher the terminal UI uses. FLTK builds the same
# way on every platform via CMake, so it's fetched from source here
# instead of depending on a system package (there's no Windows package
# manager to depend on anyway) - same approach as the Npcap SDK above.
# Turn this off to build a GUI-free, FLTK-free binary.
option(NJAMX_BUILD_GUI "Build the FLTK-based GUI (-g/--gui flag)" ON)

if (NJAMX_BUILD_GUI)
    include(FetchContent)

    # Only the core "fltk" library is needed (windows/buttons/inputs/
    # Fl_Table_Row) - trim everything else FLTK's own CMakeLists would
    # otherwise build by default.
    set(FLTK_BUILD_TEST OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_FLUID OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_FLTK_OPTIONS OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_FORMS OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_GL OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_HTML_DOCS OFF CACHE BOOL "" FORCE)
    set(FLTK_BUILD_PDF_DOCS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(fltk
        GIT_REPOSITORY https://github.com/fltk/fltk.git
        GIT_TAG release-1.4.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(fltk)
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
        src/network/packet_log.c
        src/process/cli.c
        src/process/cli_commands.c
        src/process/jammer.c
        src/process/runtime_options.c
        src/platform/detach.c
        src/tools/mac_change.c
        src/tools/privileges.c
    )
    if (WIN32)
        list(APPEND SOURCES
            src/platform/win_adapter.c
        )

        set(NJAMX_ICON_ICO "${CMAKE_SOURCE_DIR}/packaging/icons/njamx.ico")
        configure_file("${CMAKE_SOURCE_DIR}/packaging/windows/njamx.rc.in"
            "${CMAKE_BINARY_DIR}/njamx.rc" @ONLY)
        list(APPEND SOURCES "${CMAKE_BINARY_DIR}/njamx.rc")
    endif()
endif()

if (CMAKE_CXX_COMPILER AND NJAMX_BUILD_GUI)
    list(APPEND SOURCES
        src/process/gui/gui_entry.cpp
        src/process/gui/format.cpp
        src/process/gui/device_table.cpp
        src/process/gui/packet_view.cpp
        src/process/gui/setup_window.cpp
        src/process/gui/main_window.cpp
    )
endif()

# Default exec name
if(NOT EXECUTABLE_OUTPUT_NAME)
    set(EXECUTABLE_OUTPUT_NAME "njamx")
endif()

add_executable(njamx ${SOURCES})

set_target_properties(njamx PROPERTIES OUTPUT_NAME ${EXECUTABLE_OUTPUT_NAME})

if (CMAKE_CXX_COMPILER)
    set_target_properties(njamx PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
    )
endif()

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

if (NJAMX_BUILD_GUI)
    target_link_libraries(njamx PRIVATE fltk)
    target_compile_definitions(njamx PRIVATE NJAMX_HAVE_GUI=1)
endif()

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

# ==============================
# Packaging (cpack)
# ==============================

include(GNUInstallDirs)

if(WIN32)
    install(TARGETS njamx RUNTIME DESTINATION .)

    # Npcap can't be redistributed as a library dependency (no Debian-style
    # package manager on Windows), so the installer bundles Npcap's own
    # installer and runs it, same as the SDK zip above is fetched from
    # npcap.com on first configure.
    set(NPCAP_INSTALLER_URL "https://npcap.com/dist/npcap-1.88.exe" CACHE STRING
        "URL of the Npcap installer to bundle with the njamx installer")
    set(NPCAP_INSTALLER_EXE "${CMAKE_BINARY_DIR}/npcap-installer.exe")
    if (NOT EXISTS "${NPCAP_INSTALLER_EXE}")
        message(STATUS "Downloading Npcap installer from ${NPCAP_INSTALLER_URL} ...")
        file(DOWNLOAD "${NPCAP_INSTALLER_URL}" "${NPCAP_INSTALLER_EXE}" STATUS NPCAP_EXE_DL_STATUS)
        list(GET NPCAP_EXE_DL_STATUS 0 NPCAP_EXE_DL_CODE)
        if (NOT NPCAP_EXE_DL_CODE EQUAL 0)
            file(REMOVE "${NPCAP_INSTALLER_EXE}")
            message(WARNING
                "Could not download the Npcap installer automatically (it "
                "gets a new version number periodically - check "
                "https://npcap.com/dist/ and update NPCAP_INSTALLER_URL). "
                "The generated njamx installer will not bundle it, so Npcap "
                "must already be installed on the target machine.")
        endif()
    endif()

    # CPack's NSIS generator can't do a real "install for me / install for
    # everyone / pick my own folder" choice at install time (that needs
    # NSIS's MultiUser.nsh, which has no CPACK_NSIS_* hook), nor a custom
    # "add to PATH, and for whom" page, so packaging/windows/installer.nsi
    # is a hand-written NSIS script instead, built via this target rather
    # than through `cpack`.
    find_program(NJAMX_MAKENSIS makensis
        PATHS "$ENV{ProgramFiles}/NSIS" "$ENV{ProgramFiles\(x86\)}/NSIS")

    if (NJAMX_MAKENSIS AND EXISTS "${NPCAP_INSTALLER_EXE}")
        set(NJAMX_INSTALLER_OUT "${CMAKE_BINARY_DIR}/njamx-${PROJECT_VERSION}-setup.exe")
        # NSIS's File instruction doesn't recognize CMake's forward-slash
        # paths as absolute, so these need converting to backslash form
        # ($<TARGET_FILE:...> can only be resolved this late, at build
        # time, so that conversion has to be a generator expression too).
        add_custom_target(installer
            COMMAND "${NJAMX_MAKENSIS}"
                "/DNJAMX_VERSION=${PROJECT_VERSION}"
                "/DNJAMX_EXE=$<SHELL_PATH:$<TARGET_FILE:njamx>>"
                "/DNJAMX_EXE_NAME=$<TARGET_FILE_NAME:njamx>"
                "/DNJAMX_LICENSE=$<SHELL_PATH:${CMAKE_SOURCE_DIR}/LICENSE>"
                "/DNJAMX_NPCAP_EXE=$<SHELL_PATH:${NPCAP_INSTALLER_EXE}>"
                "/DNJAMX_OUT=$<SHELL_PATH:${NJAMX_INSTALLER_OUT}>"
                "${CMAKE_SOURCE_DIR}/packaging/windows/installer.nsi"
            DEPENDS njamx
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/packaging/windows"
            COMMENT "Building ${NJAMX_INSTALLER_OUT}"
            VERBATIM
        )
    else()
        add_custom_target(installer
            COMMAND "${CMAKE_COMMAND}" -E echo
                "Can't build the installer: makensis not found and/or the Npcap installer wasn't downloaded (see the warnings above)."
            COMMAND "${CMAKE_COMMAND}" -E false
        )
    endif()
else()
    install(TARGETS njamx RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    if (NJAMX_BUILD_GUI)
        # Lets njamx show up as a normal clickable app in desktop menus
        # (Exec passes -g so it opens straight into the GUI).
        install(FILES "${CMAKE_SOURCE_DIR}/packaging/linux/njamx.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
        install(FILES "${CMAKE_SOURCE_DIR}/packaging/icons/njamx.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps")
    endif()

    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcap2-bin")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
        "njamx - ARP-based LAN device scanner and jammer\nRequires the libcap2-bin 'setcap' tool, applied automatically by this package's postinst script so njamx can send/receive raw packets without running as root."
    )
    # Grants CAP_NET_RAW to the installed binary so it doesn't need sudo
    # every run; see tools/privileges.c, which checks for it at startup.
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/packaging/postinst")
endif()

install(FILES "${CMAKE_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_INSTALL_DOCDIR}")

# Windows packaging is the hand-written installer.nsi/`installer` target
# above, not CPack, so this is Linux-only from here down.
if (NOT WIN32)
    set(CPACK_PACKAGE_NAME "njamx")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_CONTACT "Julecko")
    set(CPACK_PACKAGE_VENDOR "Julecko")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

    include(CPack)
endif()
