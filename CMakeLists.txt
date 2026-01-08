cmake_minimum_required(VERSION 3.22)
project(PhysicsEngine)
set(CMAKE_CXX_STANDARD 17)

# Where CLion’s embedded vcpkg installs stuff:
set(VCPKG_INSTALLED_DIR "C:/Users/Jacob/.vcpkg-clion/vcpkg/installed/x64-mingw-dynamic")

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
find_package(glm CONFIG REQUIRED)

add_library(PhysicsCore STATIC
        # Physics
        src/physics/PhysicsSystem.h
        src/physics/PhysicsSystem.cpp
        src/physics/PhysicsBody.h
        src/physics/PhysicsBody.cpp
        src/physics/RigidBody.h
        src/physics/RigidBody.cpp
        src/physics/PointMass.h
        src/physics/PointMass.cpp

        # Bounding Box Logic
        src/physics/bounding/AABB.h
        src/physics/bounding/AABB.cpp
        src/physics/bounding/BoxCollider.h
        src/physics/bounding/BoxCollider.cpp
        src/physics/bounding/ICollider.h

        # Solvers
        src/physics/solver/InterceptSolver.h
        src/physics/solver/InterceptSolver.cpp
        src/physics/solver/ProblemRouter.h
        src/physics/solver/ProblemRouter.cpp
        src/physics/solver/VectorRootSolver.h
        src/physics/solver/VectorRootSolver.cpp
        src/physics/solver/ISolver.h

        # Dependencies
        src/graphics/utils/MathUtils.h
)

add_executable(PhysicsEngine src/main.cpp
        src/graphics/core/SceneObject.cpp
        src/graphics/core/SceneObject.h
        src/graphics/core/Camera.cpp
        src/graphics/core/Camera.h
        src/graphics/core/Scene.cpp
        src/graphics/core/Scene.h
        src/graphics/core/ResourceManager.cpp
        src/graphics/core/ResourceManager.h
        src/graphics/core/UniformBuffer.cpp
        src/graphics/core/UniformBuffer.h
        src/graphics/core/SceneManager.cpp
        src/graphics/core/SceneManager.h
        src/graphics/core/SceneSerializer.cpp
        src/graphics/core/SceneSerializer.h
        src/graphics/core/SceneObjectOptions.h
        src/graphics/components/Mesh.cpp
        src/graphics/components/Shader.cpp
        src/graphics/components/Shader.h
        src/graphics/components/ComputeShader.cpp
        src/graphics/components/ComputeShader.h
        src/graphics/components/Gizmo.cpp
        src/graphics/components/Gizmo.h
        src/graphics/components/TranslateHandle.cpp
        src/graphics/components/TranslateHandle.h
        src/graphics/components/RotateHandle.cpp
        src/graphics/components/RotateHandle.h
        src/graphics/components/ScaleHandle.cpp
        src/graphics/components/ScaleHandle.h
        src/graphics/core/IDrawable.h
        src/graphics/core/IPickable.h
        src/graphics/components/IHandle.h
        src/ui/OpenGLWindow.cpp
        src/ui/OpenGLWindow.h
        src/ui/MainWindow.cpp
        src/ui/MainWindow.h
        src/ui/RawInputFilter.cpp
        src/ui/RawInputFilter.h
        src/ui/inspector/InspectorWidget.cpp
        src/ui/inspector/InspectorWidget.h
        src/ui/HierarchyWidget.cpp
        src/ui/HierarchyWidget.h
        src/ui/inspector/InspectorRow.h
        src/ui/inspector/InspectorRow.cpp
        src/ui/inspector/IInspectorSection.h
        src/ui/inspector/TransformInspectorWidget.h
        src/ui/inspector/TransformInspectorWidget.cpp
        src/ui/inspector/InspectorRow.h
        src/ui/SnapshotTableModel.cpp
        src/ui/SnapshotTableModel.h
        src/ui/SolverDialog.cpp
        src/ui/SolverDialog.h
        src/ui/Vector3Widget.cpp
        src/ui/Vector3Widget.h
        src/ui/ScalarWidget.cpp
        src/ui/ScalarWidget.h
        src/ui/inspector/PhysicsInspectorWidget.cpp
        src/ui/inspector/PhysicsInspectorWidget.h
        src/ui/inspector/ForcesInspectorWidget.cpp
        src/ui/inspector/ForcesInspectorWidget.h
        src/ui/inspector/GlobalsInspectorWidget.cpp
        src/ui/inspector/GlobalsInspectorWidget.h
)

# Include directories
target_include_directories(PhysicsEngine PRIVATE
        ${CMAKE_SOURCE_DIR}/src)

# Link all the libraries
target_link_libraries(PhysicsEngine PRIVATE
        PhysicsCore
        OpenGL::GL
        glm::glm
        Qt6::Widgets
        Qt6::Gui
        Qt6::Core
        Qt6::OpenGLWidgets
        atomic
)

set(ASSETS_SOURCE "${CMAKE_SOURCE_DIR}/src/assets")
set(ASSETS_DEST "$<TARGET_FILE_DIR:PhysicsEngine>/assets")

add_custom_command(TARGET PhysicsEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSETS_SOURCE}
        ${ASSETS_DEST}
        COMMENT "Copying Assets..."
)

enable_testing()
add_subdirectory(tests)

target_include_directories(PhysicsCore PUBLIC src)
target_link_libraries(PhysicsCore PUBLIC
        glm::glm
        atomic
)