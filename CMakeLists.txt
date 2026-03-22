cmake_minimum_required(VERSION 3.15)

project(learning-opengl-again VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/glad.cmake)
include(cmake/imgui.cmake)

set(SOURCE_FILES
    src/main.cpp
    src/application.hpp
    src/application.cpp
    src/imgui_impl.hpp
    src/imgui_impl.cpp
    src/applog.hpp
    src/applog.cpp
    src/logger.hpp
    src/logger.cpp
    src/callback_sink.hpp
    src/scene.hpp
    src/scenes.hpp
    src/shader_util.hpp
)

file(GLOB SCENE_SOURCE_FILES "src/scenes/*")

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${SCENE_SOURCE_FILES})

add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/quill)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glm::glm glad quill::quill imgui::imgui)

