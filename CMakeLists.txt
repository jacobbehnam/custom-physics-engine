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

# === Qt ===
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64")
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core OpenGLWidgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_WIN32_EXECUTABLE FALSE)

# === GLM ===
find_package(glm CONFIG REQUIRED)  # << Added this line

# add glad.c to your target
add_executable(OpenGLApp main.cpp
        graphics/components/Mesh.cpp
        graphics/core/SceneObject.cpp
        graphics/core/SceneObject.h
        graphics/core/Camera.cpp
        graphics/core/Camera.h
        graphics/interfaces/IDrawable.h
        graphics/components/TranslateHandle.cpp
        graphics/components/TranslateHandle.h
        graphics/components/Shader.cpp
        graphics/components/Shader.h
        graphics/core/Scene.cpp
        graphics/core/Scene.h
        graphics/components/Gizmo.cpp
        graphics/components/Gizmo.h
        graphics/utils/MathUtils.cpp
        graphics/utils/MathUtils.h
        graphics/interfaces/IPickable.h
        graphics/core/ResourceManager.cpp
        graphics/core/ResourceManager.h
        graphics/components/RotateHandle.cpp
        graphics/components/RotateHandle.h
        graphics/interfaces/IHandle.h
        graphics/components/ScaleHandle.cpp
        graphics/components/ScaleHandle.h
        graphics/core/UniformBuffer.cpp
        graphics/core/UniformBuffer.h
        physics/RigidBody.cpp
        physics/RigidBody.h
        physics/PhysicsSystem.cpp
        physics/PhysicsSystem.h
        physics/bounding/AABB.cpp
        physics/bounding/AABB.h
        physics/PointMass.cpp
        physics/PointMass.h
        ui/OpenGLWindow.cpp
        ui/OpenGLWindow.h
        ui/MainWindow.cpp
        ui/MainWindow.h
        ui/RawInputFilter.cpp
        ui/RawInputFilter.h
        graphics/core/SceneManager.cpp
        graphics/core/SceneManager.h
        ui/InspectorWidget.cpp
        ui/InspectorWidget.h
        ui/HierarchyWidget.cpp
        ui/HierarchyWidget.h
        ui/InspectorRow.cpp
        ui/InspectorRow.h
        graphics/interfaces/ICollider.h
        physics/bounding/BoxCollider.cpp
        physics/bounding/BoxCollider.h
        ui/SnapshotTableModel.cpp
        ui/SnapshotTableModel.h
        graphics/core/SceneObjectOptions.h
        physics/solver/OneUnknownSolver.cpp
        physics/solver/OneUnknownSolver.h
        physics/PhysicsBody.h
        physics/PhysicsBody.cpp
        graphics/components/ComputeShader.cpp
        graphics/components/ComputeShader.h
        physics/PhysicsBody.cpp
        graphics/core/SceneSerializer.cpp
        graphics/core/SceneSerializer.h
        physics/solver/ProblemRouter.cpp
        physics/solver/ProblemRouter.h
        physics/solver/VectorRootSolver.cpp
        physics/solver/VectorRootSolver.h
        ui/SolverDialog.cpp
        ui/SolverDialog.h)

# Include directories
target_include_directories(OpenGLApp PRIVATE
        ${GLFW3_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR})

# Link all the libraries
target_link_libraries(OpenGLApp PRIVATE
        ${GLFW3_LIBRARY}
        OpenGL::GL
        glm::glm                 # << Added this link
        Qt6::Widgets
        Qt6::Gui
        Qt6::Core
        Qt6::OpenGLWidgets
        atomic
)
