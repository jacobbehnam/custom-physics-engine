cmake_minimum_required(VERSION 3.22)
project(OpenGLApp)
set(CMAKE_CXX_STANDARD 17)

# Where CLion’s embedded vcpkg installs stuff:
set(VCPKG_INSTALLED_DIR "C:/Users/Jacob/.vcpkg-clion/vcpkg/installed/x64-mingw-dynamic")

# Manually point to the headers and lib for GLFW
set(GLFW3_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/include")
set(GLFW3_LIBRARY "${VCPKG_INSTALLED_DIR}/lib/libglfw3dll.a")

# Also pull in OpenGL itself
find_package(OpenGL REQUIRED)

# === GLM ===
find_package(glm CONFIG REQUIRED)  # << Added this line

# === GLAD ===
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/glad")
include_directories("${GLAD_DIR}/include")

# add glad.c to your target
add_executable(OpenGLApp main.cpp "${GLAD_DIR}/src/glad.c"
        graphics/Mesh.cpp
        graphics/SceneObject.cpp
        graphics/SceneObject.h
        graphics/Camera.cpp
        graphics/Camera.h)

# Include directories
target_include_directories(OpenGLApp PRIVATE ${GLFW3_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR})

# Link all the libraries
target_link_libraries(OpenGLApp PRIVATE
        ${GLFW3_LIBRARY}
        OpenGL::GL
        glm::glm                 # << Added this link
)
