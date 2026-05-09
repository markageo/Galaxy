add_library(libs "")

target_sources( libs 
    PRIVATE 
        InputParser.cpp 
    PRIVATE 
        InputParser.h )


target_link_libraries(libs PRIVATE warning_flags)
target_link_libraries(libs PUBLIC compiler_flags)

# Main solver executable
add_executable(galaxy main.cpp)
target_link_libraries(galaxy PUBLIC libs)
