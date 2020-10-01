cmake_minimum_required(VERSION 3.10)
#project (MK404_Tests VERSION 1.0)

file(GLOB PRINTER_CLASSES ${PROJECT_SOURCE_DIR}/parts/printers/*.h)

file(GLOB GCODE_FILES *.gcode)

foreach(PRINTER_FN IN LISTS PRINTER_CLASSES)
	get_filename_component(PRINTER ${PRINTER_FN} NAME_WE)
	add_custom_target(${PRINTER}_SDcard.bin
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
		DEPENDS MK404
		COMMAND MK404 --sdimage ${PROJECT_BINARY_DIR}/${PRINTER}_SDcard.bin --image-size 128M
		COMMAND mcopy -vsi ${PROJECT_BINARY_DIR}/${PRINTER}_SDcard.bin ${GCODE_FILES} ::
		SOURCES ${GCODE_FILES}
	)
endforeach()
