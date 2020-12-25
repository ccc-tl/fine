cmake_minimum_required(VERSION 3.12)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)

project(fine)

file(GLOB XDELTA_SRC vendor/xdelta/xdelta3/xdelta*.c)
set_source_files_properties(
  ${XDELTA_SRC}
  PROPERTIES
  COMPILE_FLAGS "-w"
)

file(GLOB XXHASH_SRC vendor/xxHash/xxhash.c)
set_source_files_properties(
  ${XXHASH_SRC}
  PROPERTIES
  COMPILE_FLAGS "-w"
)

file(GLOB LIBKIRK_SRC src/module/libkirk/*.c)
set_source_files_properties(
  ${LIBKIRK_SRC}
  PROPERTIES
  COMPILE_FLAGS "-w"
)

include_directories(fine PRIVATE vendor/xdelta/xdelta3 vendor/xxHash vendor/picojson)
file(GLOB FINE_SRC CONFIGURE_DEPENDS src/*.cpp src/module/*.cpp)

add_executable(fine ${XDELTA_SRC} ${XXHASH_SRC} ${LIBKIRK_SRC} ${FINE_SRC})
add_compile_definitions(SIZEOF_SIZE_T=8 PICOJSON_USE_INT64)
add_subdirectory(vendor/spdlog)
target_compile_options(fine PRIVATE -Wall -Wextra -Wstrict-aliasing=0 -fno-rtti -O3)
if (WIN32)
  set(OS_LINK_FLAGS "-municode")
else()
  set(OS_LINK_FLAGS)
endif()
target_link_libraries(fine ${OS_LINK_FLAGS} -static spdlog::spdlog -s)
set_target_properties(fine
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target"
)
