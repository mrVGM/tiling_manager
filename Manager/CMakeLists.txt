cmake_minimum_required (VERSION 3.8)

add_executable (Manager
	"main.cpp"
)

target_link_libraries(Manager 
	Bridge
	Tasks
	Advapi32.lib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/lua54.lib
)

target_include_directories(Manager PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Manager PROPERTY CXX_STANDARD 20)
endif()
