cmake_minimum_required(VERSION 3.10)
set(CMAKE_DEBUG_POSTFIX d)
 
include(ExternalProject)

project(Einsy VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set (FIRMWARE
    MM-control-01.hex
    MK3S.afx
    stk500boot_v2_mega2560.hex
    )

set(H_FILES
    hwDefs/ADC_Buttons.h
    hwDefs/Button.h
    hwDefs/Einsy_EEPROM.h
    hwDefs/Fan.h
    hwDefs/HC595.h
    hwDefs/hd44780_charROM.h
    hwDefs/HD44780GL.h
    hwDefs/HD44780.h
    hwDefs/Heater.h
    hwDefs/IRSensor.h
    hwDefs/LED.h
    hwDefs/PINDA.h
    hwDefs/RotaryEncoder.h
    hwDefs/sd_card.h
    hwDefs/Thermistor.h
    hwDefs/TMC2130.h
    hwDefs/UART_Logger.h
    hwDefs/uart_pty.h
    hwDefs/VoltageSrc.h
    hwDefs/w25x20cl.h
    MMU2.h
)

set(INCLUDE_FILES
    include/SerialPipe.h
    include/ADCPeripheral.h
    include/BasePeripheral.h
    include/PinHelper_2560.h
    include/Util.h
    include/thermistortables.h
)


set(EINSY_SOURCES
    hwDefs/ADC_Buttons.cpp
    hwDefs/Button.cpp
    hwDefs/Einsy_EEPROM.cpp
    hwDefs/Fan.cpp
    hwDefs/HC595.cpp
    hwDefs/HD44780.cpp
    hwDefs/HD44780GL.cpp
    hwDefs/Heater.cpp
    hwDefs/IRSensor.cpp
    hwDefs/LED.cpp
    hwDefs/PINDA.cpp
    hwDefs/RotaryEncoder.cpp
    hwDefs/sd_card.c
    hwDefs/Thermistor.cpp
    hwDefs/TMC2130.cpp
    hwDefs/UART_Logger.cpp
    hwDefs/uart_pty.cpp
    hwDefs/VoltageSrc.cpp
    hwDefs/w25x20cl.cpp
    MMU2.cpp
    include/SerialPipe.cpp  
)

add_executable(Einsy Einsy.cpp ${EINSY_SOURCES} ${H_FILES} ${INCLUDE_FILES} ${FIRMWARE})

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE SIMAVR_BIN_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

message("Simavr binary dir detected as ${SIMAVR_BIN_DIR}")

ExternalProject_Add(simavr
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rdParty/simavr
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make build-simavr RELEASE=0
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1

)

set (LIBSIMAVR ${PROJECT_SOURCE_DIR}/3rdParty/simavr/simavr/obj-${SIMAVR_BIN_DIR}/libsimavr.a)

message("Defining simavr location as ${LIBSIMAVR}")

target_include_directories (Einsy PUBLIC 
                            "${PROJECT_BINARY_DIR}"
                            "${PROJECT_SOURCE_DIR}/3rdParty/simavr/simavr/sim/"
                            "${PROJECT_SOURCE_DIR}/hwDefs"
                            "${PROJECT_SOURCE_DIR}/include"
                         )

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

add_custom_target(Build_Run COMMAND cd ${PROJECT_BINARY_DIR} && ./Einsy &)
add_dependencies(Build_Run Einsy)

add_custom_target(MK3S.afx  COMMAND cd ${PROJECT_SOURCE_DIR} 
                            && ./build-fw.sh 
                            && cp ../Prusa-Firmware-build/Firmware.ino.elf MK3S.afx
                            && avr-objdump -d MK3S.afx > MK3S.txt)

add_custom_target(FAT32.img COMMAND unzip -o ${PROJECT_SOURCE_DIR}/assets/FAT32.zip)
add_custom_target(Einsy_atmega2560_SDcard.bin COMMAND mcopy -vsi FAT32.img ${PROJECT_SOURCE_DIR}/SDCard/* :: 
                                                COMMAND mv FAT32.img Einsy_atmega2560_SDcard.bin)
add_dependencies(Einsy_atmega2560_SDcard.bin FAT32.img)
