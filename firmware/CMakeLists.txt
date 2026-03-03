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
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../dependencies/ch32sdk ch32sdk)
elseif(MCU_PLATFORM STREQUAL "K1921VK035")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../dependencies/niietsdk niietsdk)
endif()

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
if(MCU_PLATFORM STREQUAL "K1921VK035")

    add_executable(esc-firmware-niiet)

    target_sources(esc-firmware-niiet
        PRIVATE
            Src/main.c
            Src/dshot.c
            Src/firmwareversion.c
            Src/functions.c
            Src/signal.c
            Src/sounds.c
            Src/kiss_telemetry.c
            Mcu/K19XXVK035/Src/ADC.c
            Mcu/K19XXVK035/Src/comparator.c
            Mcu/K19XXVK035/Src/eeprom.c
            Mcu/K19XXVK035/Src/IO.c
            Mcu/K19XXVK035/Src/phaseouts.c
            Mcu/K19XXVK035/Src/serial_telemetry.c
            Mcu/K19XXVK035/Src/K19xxVK035_it.c
            Mcu/K19XXVK035/Src/peripherals.c
    )

    target_include_directories(esc-firmware-niiet
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/Inc
            ${CMAKE_CURRENT_SOURCE_DIR}/Mcu/K19XXVK035/Inc
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

    gcc_add_linker_script(esc-firmware-niiet PRIVATE ${K1921VK035_ESC_APP_LD_SCRIPT})
    gcc_generate_hex_file(esc-firmware-niiet)
    gcc_generate_bin_file(esc-firmware-niiet)

elseif(MCU_PLATFORM STREQUAL "CH32V203")

    add_executable(esc-firmware-wch)

    target_sources(esc-firmware-wch
        PRIVATE
            Src/main.c
            Src/dshot.c
            Src/firmwareversion.c
            Src/functions.c
            Src/signal.c
            Src/sounds.c
            Src/kiss_telemetry.c
            Mcu/CH32V203/Src/ADC.c
            Mcu/CH32V203/Src/comparator.c
            Mcu/CH32V203/Src/eeprom.c
            Mcu/CH32V203/Src/IO.c
            Mcu/CH32V203/Src/phaseouts.c
            Mcu/CH32V203/Src/serial_telemetry.c
            Mcu/CH32V203/Src/ch32v20x_it.c
            Mcu/CH32V203/Src/peripherals.c
    )

    target_include_directories(esc-firmware-wch
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/Inc
            ${CMAKE_CURRENT_SOURCE_DIR}/Mcu/CH32V203/Inc
    )

    target_link_libraries(esc-firmware-wch
        PRIVATE
            ch32sdk
            CH32V
            NANO
            NOSYS
    )

    target_compile_definitions(esc-firmware-wch PRIVATE
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

    set(ESC_WCH_LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/ch32sdk/ld/esc_ch32v20x_D6.ld)
    gcc_add_linker_script(esc-firmware-wch PRIVATE ${ESC_WCH_LD_SCRIPT})
    gcc_generate_hex_file(esc-firmware-wch)
    gcc_generate_bin_file(esc-firmware-wch)

endif()