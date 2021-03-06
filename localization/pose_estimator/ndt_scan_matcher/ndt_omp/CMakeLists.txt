cmake_minimum_required(VERSION 3.0.2)
project(ndt_omp)

# -mavx causes a lot of errors!!
add_definitions(-std=c++14 -msse -msse2 -msse3 -msse4 -msse4.1 -msse4.2)
set(CMAKE_CXX_FLAGS "-std=c++14 -msse -msse2 -msse3 -msse4 -msse4.1 -msse4.2")

# warn performance issue of ndt_omp which is NOT built in release mode
string(TOUPPER ${CMAKE_BUILD_TYPE} uppercase_CMAKE_BUILD_TYPE)
if (NOT (${uppercase_CMAKE_BUILD_TYPE} STREQUAL "RELEASE"))
  message(WARNING
    "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} may cause a serious performance degradation. Please configure with -DCMAKE_BUILD_TYPE=Release.")
endif()

find_package(catkin REQUIRED COMPONENTS
  roscpp
  pcl_ros
)

find_package(OpenMP)
if (OPENMP_FOUND)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

catkin_package(
  INCLUDE_DIRS include
  LIBRARIES ndt_omp
  CATKIN_DEPENDS
)

###########
## Build ##
###########

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

add_library(ndt_omp
  src/ndt_omp/voxel_grid_covariance_omp.cpp
  src/ndt_omp/ndt_omp.cpp
)

add_executable(align
  apps/align.cpp
)
add_dependencies(align
  ndt_omp
)
# pcl 1.7 causes a segfault without -O2 (or -O3) flag
target_compile_options(ndt_omp PUBLIC
  $<$<CONFIG:Debug>:-O2 -g>
)
target_link_libraries(align
  ${catkin_LIBRARIES}
  ndt_omp
)

install(TARGETS ndt_omp
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

## Install project namespaced headers
install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
)
