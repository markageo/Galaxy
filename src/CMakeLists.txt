add_library(libs "")

target_link_libraries(libs PUBLIC OpenMP::OpenMP_CXX)

target_sources( libs 
    PRIVATE 
        InputParser.cpp
        InputData.cpp 
        InitialConditions.cpp
        EngineCPU.cpp
        EngineCUDA.cu
    PRIVATE 
        InputParser.h
        Particles.h
        Types.h
        CSVParticleStateWriter.h
        InputData.h
        InitialConditions.h
        Engine.h 
        Tree.h
)

if (USE_CUDA)
    target_sources( libs PRIVATE EngineCUDA.cu )
endif()


if (HAS_HDF5)
    target_compile_definitions(libs PUBLIC HAS_HDF5)
    target_include_directories(libs PUBLIC ${HDF5_INCLUDE_DIRS})
    target_link_libraries(libs PUBLIC ${HDF5_LIBRARIES})
    target_sources( libs PRIVATE HDF5ParticleStateWriter.h )
endif()

target_compile_options(libs PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wshadow;-Wunused;-Wconversion;-Wpedantic>
)

# Main solver executable
add_executable(galaxy main.cpp)
target_link_libraries(galaxy PUBLIC libs)
