cmake_minimum_required(VERSION 3.0)

project(OpenGL LANGUAGES C CXX)

# compile flags
add_compile_options(${COMPILE_FLAGS})
add_definitions(-w)

###############################################################################
## file globbing ##############################################################
###############################################################################

# these instructions search the directory tree when CMake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`
file(GLOB_RECURSE sources   
    src/**/**/**/*.c src/**/**/**/*.h src/**/**/**/*.cpp src/**/**/**/*.hpp 
    src/**/**/*.c src/**/**/*.h src/**/**/*.cpp src/**/**/*.hpp 
    src/**/*.c src/**/*.h src/**/*.cpp src/**/*.hpp 
    src/*.c src/*.h src/*.cpp src/*.hpp
)
file(GLOB_RECURSE data resources/*)
# you can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically

###############################################################################
## target definitions #########################################################
###############################################################################

# add the data to the target, so it becomes visible in some IDE

add_library(OpenGL ${sources})

target_link_directories(OpenGL PUBLIC ${CMAKE_SOURCE_DIR}/lib )

set_property(TARGET OpenGL PROPERTY C_STANDARD 99)

# just for OpenGL add some compiler flags
target_compile_options(OpenGL PUBLIC -std=c99 -Wall -Wfloat-conversion)

set_target_properties(OpenGL PROPERTIES COMPILE_OPTIONS -ffunction-sections)
#target_link_libraries(OpenGL "${RETRO68_ROOT}/InterfaceAndLibraries/Libraries/StubLibraries/ThreadsLib")

#target_link_libraries( OpenGL ThreadsLib)

if(PLATFORM MATCHES retro68)
    add_compile_definitions(FOR_68K)
endif()

if(PLATFORM MATCHES retroppc)
    add_compile_definitions(FOR_PPC)    
    target_link_libraries( OpenGL libGLUT)
endif()


message(STATUS "Making for ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(OpenGL PROPERTIES LINK_FLAGS "-Wl,--mac-strip-macsbug")
endif()

# this copies all resource files in the build directory
# we need this, because we want to work with paths relative to the executable
file(COPY ${data} DESTINATION resources)