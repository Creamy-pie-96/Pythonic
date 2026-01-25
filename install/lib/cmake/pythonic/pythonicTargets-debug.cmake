#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "pythonic::pythonic_graph_viewer" for configuration "Debug"
set_property(TARGET pythonic::pythonic_graph_viewer APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(pythonic::pythonic_graph_viewer PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libpythonic_graph_viewer.a"
  )

list(APPEND _cmake_import_check_targets pythonic::pythonic_graph_viewer )
list(APPEND _cmake_import_check_files_for_pythonic::pythonic_graph_viewer "${_IMPORT_PREFIX}/lib/libpythonic_graph_viewer.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
