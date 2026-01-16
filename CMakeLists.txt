cmake_minimum_required(VERSION 3.10)
project(LuminaDB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Directorios de Inclusion
include_directories(include)

# Buscar archivos fuente
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Crear el ejecutable
add_executable(${PROJECT_NAME} main.cpp ${SOURCES})

# Configuracion de advertencia
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
