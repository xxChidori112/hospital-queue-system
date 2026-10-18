cmake_minimum_required(VERSION 3.16)
project(HospitalQueueSystem LANGUAGES CXX)

find_package(SQLite3 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(hospital_queue
    src/main.cpp
    src/hospital_system.cpp
)

add_executable(hospital_web
    src/web_server.cpp
    src/hospital_system.cpp
)

target_include_directories(hospital_queue PRIVATE include)
target_include_directories(hospital_web PRIVATE include)
target_link_libraries(hospital_queue PRIVATE SQLite::SQLite3)
target_link_libraries(hospital_web PRIVATE SQLite::SQLite3)

if(MSVC)
    target_compile_options(hospital_queue PRIVATE /W4 /utf-8)
    target_compile_options(hospital_web PRIVATE /W4 /utf-8)
else()
    target_compile_options(hospital_queue PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(hospital_web PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_link_libraries(hospital_web PRIVATE ws2_32)
endif()
