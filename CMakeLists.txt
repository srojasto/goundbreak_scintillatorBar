cmake_minimum_required(VERSION 3.16)
project(ScintBar)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(scintbar scintbar.cc ${sources} ${headers})
target_include_directories(scintbar PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(scintbar ${Geant4_LIBRARIES})

foreach(macro vis.mac run.mac scan.mac scan_point.mac plotScan.C)
  configure_file(${PROJECT_SOURCE_DIR}/${macro} ${PROJECT_BINARY_DIR}/${macro} COPYONLY)
endforeach()
