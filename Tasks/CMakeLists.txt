cmake_minimum_required (VERSION 3.8)

add_library (Tasks
	"public/tasks.h"
	"private/tasks.cpp"
)

target_include_directories(Tasks INTERFACE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
target_include_directories(Tasks PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/private")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Tasks PROPERTY CXX_STANDARD 20)
endif()
