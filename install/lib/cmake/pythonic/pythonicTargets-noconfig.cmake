#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "pythonic::pythonic_graph_viewer" for configuration ""
set_property(TARGET pythonic::pythonic_graph_viewer APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(pythonic::pythonic_graph_viewer PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libpythonic_graph_viewer.a"
  )

list(APPEND _cmake_import_check_targets pythonic::pythonic_graph_viewer )
list(APPEND _cmake_import_check_files_for_pythonic::pythonic_graph_viewer "${_IMPORT_PREFIX}/lib/libpythonic_graph_viewer.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
