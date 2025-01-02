cmake_minimum_required(VERSION 3.16.0)
project(Reproduce_8K_Tearing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(APPEND CMAKE_PREFIX_PATH F:/Packages/GLEW/glew-2.2.0)

#-----------------------------------------------------------------------------
if(WIN32)
    add_definitions(-DNOMINMAX)
endif(WIN32)
if(UNIX)
    add_definitions(-DOS_LINUX_XCB_GLX)
endif(UNIX)

#-----------------------------------------------------------------------------
# Fetch and build packages

include(FetchContent)
if(WIN32)
FetchContent_Declare(
  glfw3
  GIT_REPOSITORY https://github.com/arizonaCameraLab/glfw.git
  GIT_TAG        b35641f4a3c62aa86a0b3c983d163bc0fe36026d # Master as of 4/26/2024
)
FetchContent_MakeAvailable(glfw3)
else(WIN32)
find_package(glfw3 REQUIRED)
endif(WIN32)

#-----------------------------------------------------------------------------
# Checking for optional and required packages

find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(GLEW REQUIRED)

#-----------------------------------------------------------------------------
# Build the application.

add_executable(Reproduce_8K_Tearing main.cpp)

target_include_directories(Reproduce_8K_Tearing PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  "${CMAKE_CURRENT_BINARY_DIR}/Install/include"
)
target_link_libraries(Reproduce_8K_Tearing PUBLIC
  glfw GLEW::glew OpenGL::GL
)

install(TARGETS Reproduce_8K_Tearing EXPORT ${PROJECT_NAME}
  RUNTIME DESTINATION bin COMPONENT bin
  LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT lib
  ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT lib
  INCLUDES DESTINATION include
  PUBLIC_HEADER DESTINATION include
)

if(WIN32)
  # Function to find an element in a list that contains a specified substring
  function(find_element_with_substring result list substring)
      set(found_element "")
      foreach(element IN LISTS list)
          string(FIND "${element}" "${substring}" pos)
          if(NOT pos EQUAL -1)
              set(found_element "${element}")
              break()
          endif()
      endforeach()
      set(${result} "${found_element}" PARENT_SCOPE)
  endfunction()

  # Assuming GLEW::glew is the imported target from find_package(GLEW REQUIRED)
  # Use that to find all of the glew32.dll files in neighboring directories and then pick the one
  # that has "x64" in the path.
  get_target_property(GLEW_INCLUDE_DIRS GLEW::glew INTERFACE_INCLUDE_DIRECTORIES)
  file(GLOB_RECURSE GLEW_DLL "${GLEW_INCLUDE_DIRS}/../*glew32.dll")
  find_element_with_substring(GLEW_DLL2 "${GLEW_DLL}" "x64")

  if(GLEW_DLL)
    # Copy the GLEW DLL to the build directory after building the executable
    add_custom_command(TARGET Reproduce_8K_Tearing POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${GLEW_DLL} $<TARGET_FILE_DIR:Reproduce_8K_Tearing>
      COMMENT "Copying ${GLEW_DLL} to build directory"
    )

    # Install the GLEW DLL to the install directory
    install(FILES ${GLEW_DLL} DESTINATION ${CMAKE_INSTALL_BINDIR})
  else()
    message(WARNING "GLEW DLL not found. Make sure glew32.dll is in the same path as your GLEW include directory.")
  endif()
endif()

