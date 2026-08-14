cmake_minimum_required(VERSION 3.20)
project(TBFuntimeEngineGUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Libraries
find_package(fmt CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(TB_Funtime_Engine_GUI src/main.cpp src/app.rc)

# Link libraries
target_link_libraries(TB_Funtime_Engine_GUI PRIVATE
	fmt::fmt
	Eigen3::Eigen
	imgui::imgui
	implot::implot
	glfw
	opengl32
)

# RapidCSV weirdness
find_path(RAPIDCSV_INCLUDE_DIRS "rapidcsv.h")
if(RAPIDCSV_INCLUDE_DIRS)
	target_include_directories(TB_Funtime_Engine_GUI PRIVATE ${RAPIDCSV_INCLUDE_DIRS})
endif()

# 5. Set pwd/cwd to find data files
set_target_properties(TB_Funtime_Engine_GUI PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")