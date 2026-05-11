add_library(libs "")

target_link_libraries(libs PUBLIC OpenMP::OpenMP_CXX)

target_sources( libs 
    PRIVATE 
        InputParser.cpp
        InputData.cpp 
        InitialConditions.cpp
    PRIVATE 
        InputParser.h
        Particles.h
        Types.h
        CSVParticleWriter.h
        InputData.h
        InitialConditions.h )


target_link_libraries(libs PRIVATE warning_flags)
target_link_libraries(libs PUBLIC compiler_flags)

# Main solver executable
add_executable(galaxy main.cpp)
target_link_libraries(galaxy PUBLIC libs)
