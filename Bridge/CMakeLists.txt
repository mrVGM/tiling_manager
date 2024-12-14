cmake_minimum_required (VERSION 3.8)

add_library (Bridge SHARED
	"main.cpp"
	"public/bridge.h"
)

target_include_directories(Bridge INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(Bridge PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

target_link_libraries(Bridge User32)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Bridge PROPERTY CXX_STANDARD 20)
endif()
