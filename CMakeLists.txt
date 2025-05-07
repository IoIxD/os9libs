cmake_minimum_required(VERSION 3.0)

project(aglMacOS LANGUAGES C CXX)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${RETRO68_TOOLCHAIN}universal/CIncludes/)
include_directories(./include/)
link_directories(./lib)

foreach(prg IN ITEMS triangle)
    message(STATUS Building ${prg}...)
    add_application(
        ${prg} 
        examples/${prg}.c
        CONSOLE
    )

    set_target_properties(${prg} PROPERTIES COMPILE_FLAGS "-ffunction-sections -mcpu=601 -O3 -Wall -Wextra -Wno-unused-parameter")
    set_target_properties(${prg} PROPERTIES LINK_FLAGS "-Wl,-gc-sections")

    target_link_directories(${prg} PUBLIC lib)
    target_link_libraries(${prg} ThreadsLib gl)
endforeach()


