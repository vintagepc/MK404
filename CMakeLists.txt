cmake_minimum_required(VERSION 3.10)
set(CMAKE_DEBUG_POSTFIX d)

project(Einsy VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set (FIRMWARE
    MM-control-01.hex
    MK3S.afx
    stk500boot_v2_mega2560.hex
    )

set(EINSY_SOURCES
    hwDefs/button.c
    hwDefs/button.h
    hwDefs/Einsy_EEPROM.cpp
    hwDefs/Einsy_EEPROM.h
    hwDefs/Fan.cpp
    hwDefs/Fan.h
    hwDefs/hc595.c
    hwDefs/hc595.h
    hwDefs/hd44780.c
    hwDefs/hd44780_charROM.h
    hwDefs/hd44780_glut.c
    hwDefs/hd44780_glut.h
    hwDefs/hd44780.h
    hwDefs/heater.c
    hwDefs/heater.h
    hwDefs/IRSensor.cpp
    hwDefs/IRSensor.h
    hwDefs/led.c
    hwDefs/led.h
    hwDefs/mmu_buttons.c
    hwDefs/mmu_buttons.h
    hwDefs/PINDA.c
    hwDefs/PINDA.h
    hwDefs/rotenc.c
    hwDefs/rotenc.h
    hwDefs/sd_card.c
    hwDefs/sd_card.h
    hwDefs/stepper.c
    hwDefs/stepper.h
    hwDefs/thermistor.c
    hwDefs/thermistor.h
    hwDefs/thermistortables.h
    hwDefs/TMC2130.c
    hwDefs/TMC2130.h
    hwDefs/uart_logger.c
    hwDefs/uart_logger.h
    hwDefs/uart_pty.c
    hwDefs/uart_pty.h
    hwDefs/VoltageSrc.cpp
    hwDefs/VoltageSrc.h
    hwDefs/w25x20cl.c
    hwDefs/w25x20cl.h
    mmu.c
    mmu.h
    include/PinHelper_2560.h
    include/Util.h
    include/BasePeripheral.h
    )



add_executable(Einsy Einsy.cpp ${EINSY_SOURCES} ${FIRMWARE})

#//add_subdirectory(Hardware)

target_include_directories (Einsy PUBLIC 
                            "${PROJECT_BINARY_DIR}"
                            "${PROJECT_SOURCE_DIR}/../../simavr/sim"
                            "${PROJECT_SOURCE_DIR}/../../simavr/cores"
                            "${PROJECT_SOURCE_DIR}/hwDefs"
                            "${PROJECT_SOURCE_DIR}/include"
                         )

find_library(LIBSIMAVR
            simavr
            HINTS "${PROJECT_SOURCE_DIR}/../../simavr/*")

find_package(LibElf REQUIRED)
include_directories(${LIBELF_INCLUDE_DIRS})

target_link_libraries(Einsy pthread util glut GL GLU m ${LIBSIMAVR} ${LIBELF_LIBRARIES}) 

add_custom_command(TARGET Einsy POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/MM-control-01.hex"
                    "${PROJECT_BINARY_DIR}/MM-control-01.hex")
add_custom_command(TARGET Einsy POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/MK3S.afx"
                    "${PROJECT_BINARY_DIR}/MK3S.afx")
add_custom_command(TARGET Einsy POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/stk500boot_v2_mega2560.hex"
                    "${PROJECT_BINARY_DIR}/stk500boot_v2_mega2560.hex")


