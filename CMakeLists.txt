cmake_minimum_required(VERSION 3.1.0)

project(TinyWebServer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQLCPPCONNECTOR REQUIRED mysqlcppconn)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hoop_timer
    ${CMAKE_CURRENT_SOURCE_DIR}/include/http
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tool
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mysql_connector
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lock
    ${CMAKE_CURRENT_SOURCE_DIR}/include/log
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sql
    ${CMAKE_CURRENT_SOURCE_DIR}/include/thread_pool
    ${MYSQLCPPCONNECTOR_INCLUDE_DIRS}
)


file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
    )

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    Threads::Threads
    ${MYSQLCPPCONNECTOR_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME}
    RESOURCE_PATH = "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        ${CMAKE_CURRENT_BINARY_DIR}/bin/source
    COMMENT "Copying resource files"
)