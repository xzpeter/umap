#############################################################################
# Copyright 2017-2019 Lawrence Livermore National Security, LLC and other
# UMAP Project Developers. See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: LGPL-2.1-only
#############################################################################
project(update_rate)

FIND_PACKAGE( OpenMP REQUIRED )
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS 
    "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
  add_executable(update_rate update_rate.cpp)

  add_dependencies(update_rate umap)
  target_link_libraries(update_rate umap)

  include_directories( ${CMAKE_CURRENT_SOURCE_DIR} ${UMAPINCLUDEDIRS} )

  install(TARGETS update_rate
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
    RUNTIME DESTINATION bin )
else()
  message("Skipping update_rate, OpenMP required")
endif()

