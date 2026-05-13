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
        CSVParticleWriter.h
        InputData.h
        InitialConditions.h
        Engine.h 
)

if (USE_CUDA)
    target_sources( libs PRIVATE EngineCUDA.cu )
endif()


target_compile_options(libs PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wshadow;-Wunused;-Wconversion;-Wpedantic>
)

# Main solver executable
add_executable(galaxy main.cpp)
target_link_libraries(galaxy PUBLIC libs)
