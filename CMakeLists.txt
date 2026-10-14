cmake_minimum_required(VERSION 3.20)

project(CortexActuate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Select target: HOST for SIL harness, or STM32F407 for firmware
if(NOT DEFINED CORTEXACTUATE_TARGET)
    set(CORTEXACTUATE_TARGET HOST)
endif()

# Common include paths
include_directories(${CMAKE_SOURCE_DIR}/include)

if(CORTEXACTUATE_TARGET STREQUAL "STM32F407")
    # ===========================================================================
    # STM32F407 ARM CORTEX-M4 FIRMWARE BUILD
    # ===========================================================================

    # Startup and system init
    add_library(startup OBJECT
        src/startup/startup_stm32f4.c
        src/startup/system_init.c
        src/startup/main_stub.cpp
    )
    target_compile_options(startup PRIVATE -Wall -Wextra -Werror -Os)

    # Firmware executable
    add_executable(cortexactuate.elf)

    target_sources(cortexactuate.elf PRIVATE
        $<TARGET_OBJECTS:startup>
    )

    target_compile_options(cortexactuate.elf PRIVATE
        -Wall -Wextra -Werror -Os
        -ffunction-sections -fdata-sections
    )

    target_link_options(cortexactuate.elf PRIVATE
        -Wl,--gc-sections
        -Wl,-Map=cortexactuate.map
        -T${CMAKE_SOURCE_DIR}/src/startup/stm32f407.ld
    )

    # Post-build: generate .bin and .hex, print size
    add_custom_command(TARGET cortexactuate.elf POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary cortexactuate.elf cortexactuate.bin
        COMMAND arm-none-eabi-objcopy -O ihex cortexactuate.elf cortexactuate.hex
        COMMAND arm-none-eabi-size cortexactuate.elf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

else()
    # ===========================================================================
    # HOST BUILD: SIL HARNESS, TESTS, TOOLS
    # ===========================================================================

    enable_testing()

    # GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    FetchContent_MakeAvailable(googletest)

    # Placeholder for FreeRTOS — later steps will add real integration
    add_library(freertos_stub INTERFACE)
    target_include_directories(freertos_stub INTERFACE ${CMAKE_SOURCE_DIR}/include)

    # =========== TEST EXECUTABLE ===========
    add_executable(cortexactuate_test)

    target_sources(cortexactuate_test PRIVATE
        test/test_main.cpp
        test/hal_contract_test.cpp
    )

    target_link_libraries(cortexactuate_test
        gtest gtest_main
        freertos_stub
    )

    target_compile_options(cortexactuate_test PRIVATE -Wall -Wextra -Werror)

    add_test(NAME CortexActuateTests COMMAND cortexactuate_test)

    # =========== SIL EXECUTABLE ===========
    add_executable(cortexactuate_sil)

    target_sources(cortexactuate_sil PRIVATE
        sil/main.cpp
    )

    target_link_libraries(cortexactuate_sil freertos_stub)
    target_compile_options(cortexactuate_sil PRIVATE -Wall -Wextra -Werror)

endif()
