cmake_minimum_required(VERSION 3.10)
#project (MK404_Tests VERSION 1.0)

file(GLOB TESTS_SOURCES test_*.c)

set(F_CPU 16000000UL)
set(MCU atmega2560)

set(CMAKE_EXE_LINKER_FLAGS -mmcu=${MCU})

# Use AVR GCC toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_ASM_COMPILER avr-gcc)

message(STATUS "Test source directory is ${PROJECT_SOURCE_DIR}")

add_definitions(
	-DF_CPU=${F_CPU}
)

add_custom_target( Test_firmwares

)

foreach(TEST_SRC IN LISTS TESTS_SOURCES)

	get_filename_component(TEST_BASE ${TEST_SRC} NAME_WE)

	add_custom_target(${TEST_BASE}.afx
		SOURCES ${PROJECT_SOURCE_DIR}/scripts/tests/${TEST_BASE}.txt ${PROJECT_SOURCE_DIR}/scripts/tests/${TEST_BASE}.c
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/tests"
		COMMAND  avr-gcc -Wall -gdwarf-2 -Os -std=gnu99 -mmcu=atmega2560 -DF_CPU=16000000 -fno-inline-small-functions -ffunction-sections -fdata-sections
			-Wl,--relax,--gc-sections -Wl,--undefined=_mmcu,--section-start=.mmcu=0x910000 -I${PROJECT_SOURCE_DIR}/3rdParty/simavr/simavr/sim/avr
			-o ${PROJECT_BINARY_DIR}/tests/${TEST_BASE}.afx ${TEST_SRC}
	)

	add_dependencies(Test_firmwares ${TEST_BASE}.afx)

	add_custom_command(TARGET ${TEST_BASE}.afx POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/scripts/tests/${TEST_BASE}.txt ${PROJECT_BINARY_DIR}/tests/
	)

	add_custom_target(Run_${TEST_BASE}
		COMMAND cd ${PROJECT_BINARY_DIR} && ./MK404 Test_Printer -f ${PROJECT_BINARY_DIR}/tests/${TEST_BASE}.afx --script ${PROJECT_BINARY_DIR}/tests/${TEST_BASE}.txt
		DEPENDS ${TEST_BASE}.afx
	)

endforeach()
