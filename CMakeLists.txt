cmake_minimum_required(VERSION 3.11)
project(fltk-schnapsen LANGUAGES CXX)

# prefer dynamic libraries over static
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_STATIC_LIBRARY_SUFFIX})

find_package(FLTK CONFIG REQUIRED components images)

add_executable(${PROJECT_NAME} fltk-schnapsen.cxx src)
target_include_directories(fltk-schnapsen PRIVATE include/ src/)

# to enable sound via miniaudio
if(EXISTS "${CMAKE_SOURCE_DIR}/include/miniaudio.h")
set(cxxflags "-DUSE_MINIAUDIO")
else()
endif()

# to enable shadow text
#set(cxxflags ${cxxflags} -DUSE_IMAGE_TEXT)
# to enable fancy gamebook score digits
set(cxxflags ${cxxflags} -DUSE_SVG_DIGITS)

# to enable custom font
option(USE_CUSTOM_FONT "Use Custom Font" ON)
if(USE_CUSTOM_FONT)
	set(cxxflags ${cxxflags} -DCUSTOM_FONT=\"MysteryQuest-Regular.ttf\")
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
target_link_libraries(${PROJECT_NAME} PRIVATE ${FLTK_LIBRARIES}
)

if(USE_CUSTOM_FONT)
if(UNIX)
target_link_libraries(${PROJECT_NAME} PRIVATE fontconfig)
endif()
endif()
