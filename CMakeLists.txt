cmake_minimum_required(VERSION 3.11)
project(fltk-schnapsen LANGUAGES CXX)

find_package(FLTK CONFIG REQUIRED components images)

add_executable(${PROJECT_NAME} fltk-schnapsen.cxx src)
target_include_directories(fltk-schnapsen PRIVATE include/ src/)

if(EXISTS "${CMAKE_SOURCE_DIR}/miniaudio.h")
set(cxxflags "-DUSE_MINIAUDIO")
else()
endif()

# --- Add Compiler Switches Here ---

# 1. Add general C++ standard options (Recommended way)
# This ensures you use C++20 across all platforms
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 2. Add warning flags (Highly recommended for quality control)
# These flags work on GCC, Clang, and usually map correctly to MSVC equivalents.
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    # Add other common flags like -O2 for release builds if desired
    ${cxxflags}
)

# 3. Add a specific, non-portable flag (e.g., specific to GCC/Clang)
# You can guard platform-specific flags using generator expressions:
target_compile_options(${PROJECT_NAME} PRIVATE
    # Example for enabling an address sanitizer on supported compilers/platforms
    #$<$<CXX_COMPILER_ID:GNU>:-fsanitize=address>
    #$<$<CXX_COMPILER_ID:Clang>:-fsanitize=address>
)

# --- Link the libraries as before ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    fltk
    fltk_images
)
