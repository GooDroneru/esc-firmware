cmake_minimum_required(VERSION 3.17)

set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../dependencies/cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_DIR}")

# ---------------------------------------------------------------------------
# Select target MCU platform (must be set before project())
#   K1921VK035  — NIIET ARM Cortex-M4F
#   K1921VG5T   — NIIET RISC-V
#   CH32V203    — WCH RISC-V  (reserved, not yet implemented)

# ---------------------------------------------------------------------------
set(MCU_PLATFORM "K1921VG5T" CACHE STRING "Target MCU platform: K1921VK035, K1921VG5T, or CH32V203")
set_property(CACHE MCU_PLATFORM PROPERTY STRINGS "K1921VK035" "K1921VG5T" "CH32V203")

if(MCU_PLATFORM STREQUAL "CH32V203")
    set(CH32_DEVICE "D6" CACHE STRING "CH32V20x device variant: D6, D8, D8W")
    set(CH32_SYSCLK "SYSCLK_FREQ_96MHz_HSI" CACHE STRING "System clock frequency define")
    include(${CMAKE_DIR}/ch32/CH32V2.cmake)
elseif(MCU_PLATFORM STREQUAL "K1921VK035")
    include(${CMAKE_DIR}/arm/K1921VK035.cmake)
elseif(MCU_PLATFORM STREQUAL "K1921VG5T")
    include(${CMAKE_DIR}/riscv/K1921VG5T.cmake)
else()
    message(FATAL_ERROR "Unknown MCU_PLATFORM '${MCU_PLATFORM}'. Supported: K1921VK035, K1921VG5T, CH32V203")
endif()

include(${CMAKE_DIR}/common.cmake)

project(esc-firmware VERSION 2.2 LANGUAGES C ASM)

# ---------------------------------------------------------------------------
# Platform SDK
# ---------------------------------------------------------------------------
if(MCU_PLATFORM STREQUAL "CH32V203")
    set(CH32_OVERRIDE_SYSTEM ON CACHE BOOL "Use project-local system_ch32v20x.c instead of SDK's" FORCE)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../dependencies/ch32sdk ch32sdk)
elseif(MCU_PLATFORM STREQUAL "K1921VK035")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../dependencies/niietsdk/vk035 niietsdk)
elseif(MCU_PLATFORM STREQUAL "K1921VG5T")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../dependencies/niietsdk/vg5t niietsdk)
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

    add_executable(esc-firmware-vk035)

    target_sources(esc-firmware-vk035
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

    target_include_directories(esc-firmware-vk035
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${ESC_COMMON_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/mcu/k19xxvk035/Inc
    )

    target_link_libraries(esc-firmware-vk035
        PRIVATE
            vk035sdk
            K1921VK035
            NANO
            NOSYS
            m
    )

    target_compile_definitions(esc-firmware-vk035 PRIVATE
        MCU_K19XXVK035
        __NO_SYSTEM_INIT
        SYSCLK_PLL
        FIRMWARE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FIRMWARE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    )

    target_compile_options(esc-firmware-vk035 PRIVATE
        $<$<CONFIG:Release>:-Os>
    )

    target_link_options(esc-firmware-vk035 PRIVATE
        -Wl,-Map=${CMAKE_BINARY_DIR}/esc-firmware-vk035.map
        -Wl,--print-memory-usage
        -Wl,--no-warn-rwx-segments
    )

    set(K1921VK035_ESC_APP_LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcu/k19xxvk035/esc_app.ld)
    gcc_add_linker_script(esc-firmware-vk035 PRIVATE ${K1921VK035_ESC_APP_LD_SCRIPT})
    gcc_generate_hex_file(esc-firmware-vk035)
    gcc_generate_bin_file(esc-firmware-vk035)

elseif(MCU_PLATFORM STREQUAL "K1921VG5T")

    add_executable(esc-firmware-vg5t)

    target_sources(esc-firmware-vg5t
        PRIVATE
            src/main.c
            src/dshot.c
            src/firmwareversion.c
            src/functions.c
            src/signal.c
            src/sounds.c
            src/kiss_telemetry.c
            mcu/k19xxvg5t/Src/ADC.c
            mcu/k19xxvg5t/Src/comparator.c
            mcu/k19xxvg5t/Src/eeprom.c
            mcu/k19xxvg5t/Src/IO.c
            mcu/k19xxvg5t/Src/phaseouts.c
            mcu/k19xxvg5t/Src/serial_telemetry.c
            mcu/k19xxvg5t/Src/K19xxVG5T_it.c
            mcu/k19xxvg5t/Src/peripherals.c
    )

    target_include_directories(esc-firmware-vg5t
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${ESC_COMMON_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/mcu/k19xxvg5t/Inc
    )

    target_link_libraries(esc-firmware-vg5t
        PRIVATE
            vg5tsdk
            K1921VG5T
            NANO
            NOSYS
            m
    )

    target_compile_definitions(esc-firmware-vg5t PRIVATE
        MCU_K19XXVG5T
        __NO_SYSTEM_INIT
        # Clocks are set up by the bootloader: 16 MHz HSE -> PLL 96 MHz.
        # HSECLK_VAL is needed by SystemCoreClockUpdate() to compute the clock.
        HSECLK_VAL=16000000
        SYSCLK_PLL
        CKO_NONE
        # machine timer runs from the core clock after reset (UM 10.1)
        MTIME_FREQ_HZ=96000000ULL
        FIRMWARE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FIRMWARE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    )

    target_compile_options(esc-firmware-vg5t PRIVATE
        $<$<CONFIG:Release>:-Os>
    )

    target_link_options(esc-firmware-vg5t PRIVATE
        -Wl,-Map=${CMAKE_BINARY_DIR}/esc-firmware-vg5t.map
        -Wl,--print-memory-usage
        -Wl,--no-warn-rwx-segments
    )

    set(K1921VG5T_ESC_APP_LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/mcu/k19xxvg5t/esc_app.ld)
    gcc_add_linker_script(esc-firmware-vg5t PRIVATE ${K1921VG5T_ESC_APP_LD_SCRIPT})
    gcc_generate_hex_file(esc-firmware-vg5t)
    gcc_generate_bin_file(esc-firmware-vg5t)

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
        NEED_INPUT_READY
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