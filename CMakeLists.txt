cmake_minimum_required(VERSION 3.30)
project(stadion C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

add_executable(kibic src/kibic.c src/resources.c)
add_executable(pracownik_techniczny src/pracownik_techniczny.c src/resources.c)
add_executable(kierownik_stadionu src/kierownik_stadionu.c)