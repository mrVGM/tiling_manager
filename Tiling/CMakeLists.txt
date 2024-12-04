cmake_minimum_required (VERSION 3.8)

add_executable (Tiling
	"main.cpp"
)

target_link_libraries(Tiling 
	Advapi32.lib
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Tiling PROPERTY CXX_STANDARD 20)
endif()
