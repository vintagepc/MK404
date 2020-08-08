# see FindGLUT for API details

if (NOT APPLE)
  	include(FindGLUT)
	# Xi and Xmu are most definitely not needed here.
	set( GLUT_LIBRARIES
		${GLUT_glut_LIBRARY}
	)
else()
  find_path(GLUT_INCLUDE_DIR NAMES GL/glut.h
    PATHS
    /usr/local/include
    )

  find_library(GLUT_glut_LIBRARY NAMES glut
    NO_DEFAULT_PATH
    PATHS /usr/local/lib
    DOC "brew freeglut")
  mark_as_advanced(GLUT_glut_LIBRARY)
  if(GLUT_glut_LIBRARY)
    message("Found freeglut '${GLUT_glut_LIBRARY}'")
  else()
    message(FATAL_ERROR "Cannot find library freeglut. Try: # brew install freeglut")
  endif()
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLUT REQUIRED_VARS GLUT_glut_LIBRARY GLUT_INCLUDE_DIR)
  if (GLUT_FOUND)
    # Is -lXi and -lXmu required on all platforms that have it?
    # If not, we need some way to figure out what platform we are on.
    set( GLUT_LIBRARIES
      ${GLUT_glut_LIBRARY}
      )
    if(NOT TARGET GLUT::GLUT)
      add_library(GLUT::GLUT UNKNOWN IMPORTED)
      set_target_properties(GLUT::GLUT PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GLUT_INCLUDE_DIR}")
      set_property(TARGET GLUT::GLUT APPEND PROPERTY
        IMPORTED_LOCATION "${GLUT_glut_LIBRARY}")
    endif()
  endif()
  mark_as_advanced(GLUT_INCLUDE_DIR)
endif()
