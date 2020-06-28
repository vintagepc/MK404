cmake_minimum_required(VERSION 3.10)
set(CMAKE_DEBUG_POSTFIX d)

include(ExternalProject)

project(MK404 VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Yells at you if you have extra link args.
set(CMAKE_LINK_WHAT_YOU_USE TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set (FIRMWARE
    MM-control-01.hex
    MK3S.afx
    stk500boot_v2_mega2560.hex
    )

set(H_FILES
	parts/ADCPeripheral.h
	parts/PinNames.h
	parts/BasePeripheral.h
	parts/boards/EinsyRambo.h
	parts/boards/MM_Control_01.h
	parts/PrinterFactory.h
	parts/wiring/Einsy_1_1a.h
	parts/wiring/MM_Control_01.h
	parts/wiring/Einsy_1_0a.h
	parts/ScriptHost.h
	parts/pinspecs/PinSpec_2560.h
	parts/pinspecs/PinSpec_32u4.h
	parts/PinSpec.h
	parts/SoftPWMable.h
	parts/Printer.h
	parts/SPIPeripheral.h
	parts/Scriptable.h
	parts/Board.h
	parts/components/LED.h
	parts/components/PINDA.h
	parts/components/HD44780GL.h
	parts/components/SDCard.h
	parts/components/uart_pty.h
	parts/components/hd44780_charROM.h
	parts/components/UART_Logger.h
	parts/components/RotaryEncoder.h
	parts/components/EEPROM.h
	parts/components/Thermistor.h
	parts/components/Fan.h
	parts/components/ADC_Buttons.h
	parts/components/VoltageSrc.h
	parts/components/w25x20cl.h
	parts/components/Beeper.h
	parts/components/HD44780.h
	parts/components/SerialLineMonitor.h
	parts/components/HC595.h
	parts/components/Button.h
	parts/components/PAT9125.h
	parts/components/GCodeSniffer.h
	parts/components/TMC2130.h
	parts/components/IRSensor.h
	parts/components/Heater.h
	parts/IScriptable.h
	parts/PinSpec_Helper.h
	parts/Wiring.h
#	parts/IRQHelper.h  Can't be checked/used standalone.
	parts/printers/Prusa_MK3S.h
	parts/printers/Prusa_MK3SMMU2.h
	utility/Color.h
	utility/GLPrint.h
	utility/FatImage.h
	utility/MK3SGL.h
	utility/GLObj.h
	include/SerialPipe.h
	include/Macros.h
	include/thermistortables.h
	include/MK3/Configuration_prusa.h
	include/MK3/eeprom.h
	include/Util.h
	MMU2.h
	3rdParty/arcball/Camera.hpp
	3rdParty/arcball/Matrix.hpp
	3rdParty/arcball/Vectors.hpp
	3rdParty/arcball/ArcBall.hpp
	3rdParty/arcball/Quaternion.hpp
)


set(MK404_SOURCES
	parts/Board.cpp
	parts/boards/MM_Control_01.cpp
	parts/boards/EinsyRambo.cpp
	parts/ScriptHost.cpp
	parts/components/GCodeSniffer.cpp
	parts/components/Beeper.cpp
	parts/components/HD44780GL.cpp
	parts/components/VoltageSrc.cpp
	parts/components/ADC_Buttons.cpp
	parts/components/Heater.cpp
	parts/components/EEPROM.cpp
	parts/components/IRSensor.cpp
	parts/components/w25x20cl.cpp
	parts/components/HD44780.cpp
	parts/components/UART_Logger.cpp
	parts/components/SDCard.cpp
	parts/components/PINDA.cpp
	parts/components/TMC2130.cpp
	parts/components/SerialLineMonitor.cpp
	parts/components/LED.cpp
	parts/components/HC595.cpp
	parts/components/RotaryEncoder.cpp
	parts/components/uart_pty.cpp
	parts/components/Thermistor.cpp
	parts/components/Fan.cpp
	parts/components/Button.cpp
	parts/printers/Prusa_MK3S.cpp
	parts/printers/Prusa_MK3SMMU2.cpp
	MMU2.cpp
	utility/MK3SGL.cpp
	utility/GLObj.cpp
	utility/FatImage.cpp
	utility/GLPrint.cpp
	utility/Color.cpp
	include/SerialPipe.cpp
	3rdParty/arcball/Camera.cpp
)

add_executable(MK404 MK404.cpp ${MK404_SOURCES} ${H_FILES} ${FIRMWARE})

target_compile_features(MK404 PRIVATE cxx_range_for)
target_compile_options(MK404 PRIVATE -Wall -Werror)

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

add_dependencies(MK404 simavr)

set (LIBSIMAVR ${PROJECT_SOURCE_DIR}/3rdParty/simavr/simavr/obj-${SIMAVR_BIN_DIR}/libsimavr.a)

message("Defining simavr location as ${LIBSIMAVR}")

target_include_directories (MK404 PUBLIC
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

target_link_libraries(MK404 pthread util glut GL GLU GLEW m SDL tinyobjloader ${LIBSIMAVR} ${LIBELF_LIBRARIES})

add_custom_command(TARGET MK404 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/MM-control-01.hex"
                    "${PROJECT_BINARY_DIR}/MM-control-01.hex")
add_custom_command(TARGET MK404 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/MK3S.afx"
                    "${PROJECT_BINARY_DIR}/MK3S.afx")
add_custom_command(TARGET MK404 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/stk500boot_v2_mega2560.hex"
                    "${PROJECT_BINARY_DIR}/stk500boot_v2_mega2560.hex")

# Copies the models/materials:
add_custom_command(TARGET MK404 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/assets"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/assets/*.obj"
                    "${PROJECT_BINARY_DIR}/assets/"
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PROJECT_SOURCE_DIR}/assets/*.mtl"
                    "${PROJECT_BINARY_DIR}/assets/")


add_custom_target(CPPCheck COMMAND cppcheck --error-exitcode=2 --std=c++11 --language=c++ ${MK404_SOURCES} ${H_FILES}
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_custom_target(Build_Run COMMAND cd ${PROJECT_BINARY_DIR} && ./MK404 &)
add_dependencies(Build_Run MK404)

add_custom_target(MK3S.afx  COMMAND cd ${PROJECT_SOURCE_DIR}
                            && ./build-fw.sh
                            && cp ../Prusa-Firmware-build/Firmware.ino.elf MK3S.afx
                            && avr-objdump -d MK3S.afx > MK3S.txt)


add_custom_target(FAT32.img COMMAND MK404 --sdimage FAT32.img --image-size 256M)
add_dependencies(FAT32.img MK404)
add_custom_target(Einsy_atmega2560_SDcard.bin COMMAND mcopy -vsi FAT32.img ${PROJECT_SOURCE_DIR}/SDCard/* ::
                                                COMMAND mv FAT32.img Einsy_atmega2560_SDcard.bin)
add_dependencies(Einsy_atmega2560_SDcard.bin FAT32.img)
