cmake_minimum_required(VERSION 3.17)

set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../dependencies/cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_DIR}")

# ---------------------------------------------------------------------------
# Select target MCU platform (must be set before project())
#   K1921VK035  — NIIET ARM Cortex-M4F
#   CH32V203    — WCH RISC-V  (reserved, not yet implemented)
# ---------------------------------------------------------------------------
set(MCU_PLATFORM "K1921VK035" CACHE STRING "Target MCU platform: K1921VK035 or CH32V203")
set_property(CACHE MCU_PLATFORM PROPERTY STRINGS "K1921VK035" "CH32V203")

if(MCU_PLATFORM STREQUAL "CH32V203")
    set(CH32_DEVICE "D6" CACHE STRING "CH32V20x device variant: D6, D8, D8W")
    set(CH32_SYSCLK "SYSCLK_FREQ_96MHz_HSI" CACHE STRING "System clock frequency define")
    include(${CMAKE_DIR}/ch32/CH32V2.cmake)
elseif(MCU_PLATFORM STREQUAL "K1921VK035")
    include(${CMAKE_DIR}/arm/K1921VK035.cmake)
else()
    message(FATAL_ERROR "Unknown MCU_PLATFORM '${MCU_PLATFORM}'. Supported: K1921VK035, CH32V203")
endif()

include(${CMAKE_DIR}/common.cmake)

project(esc-firmware VERSION 2.0 LANGUAGES C ASM)

# ---------------------------------------------------------------------------
# Platform SDK
# ---------------------------------------------------------------------------
if(MCU_PLATFORM STREQUAL "CH32V203")
    set(CH32_OVERRIDE_SYSTEM ON CACHE BOOL "Use project-local system_ch32v20x.c instead of SDK's" FORCE)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../dependencies/ch32sdk ch32sdk)
elseif(MCU_PLATFORM STREQUAL "K1921VK035")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../dependencies/niietsdk niietsdk)
endif()

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------

# Common include directories shared by all MCU targets
set(ESC_COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/common
)

if(MCU_PLATFORM STREQUAL "K1921VK035")

    add_executable(esc-firmware-niiet)

    target_sources(esc-firmware-niiet
        PRIVATE
            src/main.c
            src/dshot.c
            src/firmwareversion.c
            src/functions.c
            src/signal.c
            src/sounds.c
            src/kiss_telemetry.c
            mcu/k19xxvk035/Src/ADC.c
            mcu/k19xxvk035/Src/comparator.c
            mcu/k19xxvk035/Src/eeprom.c
            mcu/k19xxvk035/Src/IO.c
            mcu/k19xxvk035/Src/phaseouts.c
            mcu/k19xxvk035/Src/serial_telemetry.c
            mcu/k19xxvk035/Src/K19xxVK035_it.c
            mcu/k19xxvk035/Src/peripherals.c
    )

    target_include_directories(esc-firmware-niiet
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${ESC_COMMON_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/mcu/k19xxvk035/Inc
    )

    target_link_libraries(esc-firmware-niiet
        PRIVATE
            niietsdk
            K1921VK035
            NANO
            NOSYS
            m
    )

    target_compile_definitions(esc-firmware-niiet PRIVATE
        MCU_K19XXVK035
        __NO_SYSTEM_INIT
        SYSCLK_PLL
        FIRMWARE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FIRMWARE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    )

    target_compile_options(esc-firmware-niiet PRIVATE
        $<$<CONFIG:Release>:-Os>
    )

    target_link_options(esc-firmware-niiet PRIVATE
        -Wl,-Map=${CMAKE_BINARY_DIR}/esc-firmware-niiet.map
        -Wl,--print-memory-usage
    )

    set(K1921VK035_ESC_APP_LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcu/k19xxvk035/esc_app.ld)
    gcc_add_linker_script(esc-firmware-niiet PRIVATE ${K1921VK035_ESC_APP_LD_SCRIPT})
    gcc_generate_hex_file(esc-firmware-niiet)
    gcc_generate_bin_file(esc-firmware-niiet)

elseif(MCU_PLATFORM STREQUAL "CH32V203")

    add_executable(esc-firmware-wch)

    target_sources(esc-firmware-wch
        PRIVATE
            src/main.c
            src/dshot.c
            src/firmwareversion.c
            src/functions.c
            src/signal.c
            src/sounds.c
            src/kiss_telemetry.c
            mcu/v203/Src/ADC.c
            mcu/v203/Src/comparator.c
            mcu/v203/Src/eeprom.c
            mcu/v203/Src/IO.c
            mcu/v203/Src/phaseouts.c
            mcu/v203/Src/serial_telemetry.c
            mcu/v203/Src/ch32v20x_it.c
            mcu/v203/Src/peripherals.c
            mcu/v203/Src/system_ch32v20x.c
    )

    target_include_directories(esc-firmware-wch
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${ESC_COMMON_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/mcu/v203/Inc
    )

    target_link_libraries(esc-firmware-wch
        PRIVATE
            ch32sdk
            CH32V
            NANO
            NOSYS
    )

    target_compile_definitions(esc-firmware-wch PRIVATE
        MCU_CH32V203
        ${CH32_SYSCLK}
        FIRMWARE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FIRMWARE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    )

    target_compile_options(esc-firmware-wch PRIVATE
        $<$<CONFIG:Release>:-Os>
    )

    target_link_options(esc-firmware-wch PRIVATE
        -Wl,-Map=${CMAKE_BINARY_DIR}/esc-firmware-wch.map
        -Wl,--print-memory-usage
    )

    set(ESC_WCH_LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcu/v203/esc_app.ld)
    gcc_add_linker_script(esc-firmware-wch PRIVATE ${ESC_WCH_LD_SCRIPT})
    gcc_generate_hex_file(esc-firmware-wch)
    gcc_generate_bin_file(esc-firmware-wch)

endif()