cmake_minimum_required(VERSION 3.15)

# set the project name and version
project(galaxy VERSION 1.0 LANGUAGES CXX)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)


# Default executable location
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# specify the C++ standard
set (CMAKE_CXX_STANDARD 20)

# For clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Openmp, so that it works with clang
find_package(OpenMP REQUIRED)

# Check if CUDA is available on the system
include(CheckLanguage)
check_language(CUDA)

if (CMAKE_CUDA_COMPILER)
    set(CMAKE_CUDA_ARCHITECTURES native CACHE STRING "")
    set(USE_CUDA on)
    enable_language( CUDA )
    add_compile_definitions( USE_CUDA )
else()
    set(USE_CUDA off)
    message(STATUS "CUDA not found, only OpenMP backend will be available.")
endif()



# Add address sanitizer to the debug build
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
	message ( WARNING "THE BUILD TYPE IS DEBUG. ADDRESS SANITIZER WILL BE USED" )
endif()

# Make release the default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" )
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Type of build (Debug, Release etc." FORCE)
endif()


add_subdirectory(src)

# Link time optimisation
if( supported )
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET galaxy PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()


