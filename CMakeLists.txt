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
    parts/components/ADC_Buttons.h
    parts/components/Button.h
    parts/components/Einsy_EEPROM.h
    parts/components/Fan.h
    parts/components/HC595.h
    parts/components/hd44780_charROM.h
    parts/components/HD44780GL.h
    parts/components/HD44780.h
    parts/components/Heater.h
    parts/components/IRSensor.h
    parts/components/LED.h
    parts/components/PINDA.h
    parts/components/RotaryEncoder.h
    parts/components/Thermistor.h
    parts/components/TMC2130.h
    parts/components/UART_Logger.h
    parts/components/uart_pty.h
    parts/components/VoltageSrc.h
    parts/components/w25x20cl.h
    MMU2.h
)

set(INCLUDE_FILES
    include/MK3/Configuration_prusa.h
    include/SerialPipe.h
    include/Util.h
    include/thermistortables.h
    utility/Color.h
	utility/FatImage.h
    utility/MK3SGL.h
    utility/GLObj.h
    utility/GLPrint.h
    parts/PinNames.h
    parts/ADCPeripheral.h
    parts/BasePeripheral.h
	parts/SPIPeripheral.h
    parts/PinSpec.h
    parts/pinspecs/PinSpec_32u4.h
	parts/pinspecs/PinSpec_2560.h
    parts/Board.h
    parts/boards/MM_Control_01.h
	parts/boards/EinsyRambo.h
	parts/Printer.h
	parts/PrinterFactory.h
    parts/Wiring.h
    parts/wiring/MM_Control_01.h
	parts/wiring/Einsy_1_0a.h
	parts/wiring/Einsy_1_1a.h
)


set(EINSY_SOURCES
    parts/components/ADC_Buttons.cpp
    parts/components/Button.cpp
    parts/components/Einsy_EEPROM.cpp
    parts/components/Fan.cpp
	parts/components/GCodeSniffer.cpp
    parts/components/HC595.cpp
    parts/components/HD44780.cpp
    parts/components/HD44780GL.cpp
    parts/components/Heater.cpp
    parts/components/IRSensor.cpp
    parts/components/LED.cpp
    parts/components/PINDA.cpp
    parts/components/RotaryEncoder.cpp
    parts/components/SDCard.cpp
    parts/components/Thermistor.cpp
    parts/components/TMC2130.cpp
    parts/components/UART_Logger.cpp
    parts/components/uart_pty.cpp
    parts/components/VoltageSrc.cpp
    parts/components/w25x20cl.cpp
    MMU2.cpp
    include/SerialPipe.cpp
    utility/Color.cpp
	utility/FatImage.cpp
    parts/Board.cpp
    parts/boards/MM_Control_01.cpp
	parts/boards/EinsyRambo.cpp
	parts/printers/Prusa_MK3S.cpp
	parts/printers/Prusa_MK3SMMU2.cpp
    utility/MK3SGL.cpp
    utility/GLObj.cpp
    utility/GLPrint.cpp
    3rdParty/arcball/Camera.cpp
)

add_executable(Einsy Einsy.cpp ${EINSY_SOURCES} ${H_FILES} ${INCLUDE_FILES} ${FIRMWARE})

target_compile_features(Einsy PRIVATE cxx_range_for)

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE SIMAVR_BIN_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
message("Simavr binary dir detected as ${SIMAVR_BIN_DIR}")

add_subdirectory(3rdParty/tinyobjloader)


ExternalProject_Add(simavr
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/3rdParty/simavr
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make build-simavr RELEASE=0
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_ALWAYS 1

)

add_dependencies(Einsy simavr)

set (LIBSIMAVR ${PROJECT_SOURCE_DIR}/3rdParty/simavr/simavr/obj-${SIMAVR_BIN_DIR}/libsimavr.a)

message("Defining simavr location as ${LIBSIMAVR}")

target_include_directories (Einsy PUBLIC
                            "${PROJECT_BINARY_DIR}"
                            "${PROJECT_SOURCE_DIR}/3rdParty/simavr/simavr/sim/"
							"${PROJECT_SOURCE_DIR}/3rdParty/TCLAP/include/"
                            "${PROJECT_SOURCE_DIR}/include"
							"${PROJECT_SOURCE_DIR}/"
                            "${PROJECT_SOURCE_DIR}/parts"
                            "${PROJECT_SOURCE_DIR}/parts/boards"
                            "${PROJECT_SOURCE_DIR}/parts/components"
                            "${PROJECT_SOURCE_DIR}/parts/pinspecs"
                            "${PROJECT_SOURCE_DIR}/parts/wiring"
                            "${PROJECT_SOURCE_DIR}/utility"
                            "${PROJECT_SOURCE_DIR}/3rdParty/arcball"
                         )


include_directories("${PROJECT_SOURCE_DIR}/3rdParty/tinyobjloader/")


find_package(LibElf REQUIRED)
include_directories(${LIBELF_INCLUDE_DIRS})

target_link_libraries(Einsy pthread util glut GL GLU GLEW m tinyobjloader ${LIBSIMAVR} ${LIBELF_LIBRARIES})

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

# Copies the models/materials:
add_custom_command(TARGET Einsy POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/assets"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/assets/*.obj"
                    "${PROJECT_BINARY_DIR}/assets/"
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/assets/*.mtl"
                    "${PROJECT_BINARY_DIR}/assets/")


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
