cmake_minimum_required(VERSION 3.15)

project(learning-opengl-again VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/glad.cmake)

set(SOURCE_FILES
    src/main.cpp
    src/application.hpp
    src/application.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_subdirectory(external/glfw)
add_subdirectory(external/glm)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glm::glm glad)

