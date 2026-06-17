cmake_minimum_required(VERSION 3.25)

project(SpaceGame
		LANGUAGES CXX
		VERSION 0.1.0
)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS source/*.cpp)

add_executable(space-game ${SOURCES})

target_include_directories(space-game PRIVATE include)

target_link_libraries(space-game PRIVATE
		Vulkan::Vulkan
		glfw
)
