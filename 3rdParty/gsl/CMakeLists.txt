project( use-gsl-lite LANGUAGES CXX )

# Provide #include access to gsl-lite as 'gsl/gsl-lite.hpp':

add_library( gsl-lite INTERFACE )
target_include_directories( gsl-lite INTERFACE ${PROJECT_SOURCE_DIR} )  # adapt as necessary
