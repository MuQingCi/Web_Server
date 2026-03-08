cmake_minimum_required(VERSION 3.10...4.2)
project(TinyWebServer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_BUILD_TYPE Debug)

# 线程库
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 查找 MariaDB C++ Connector
find_package(PkgConfig REQUIRED)

# 首先尝试 mariadb-connector-cpp
pkg_check_modules(MARIADBPP mariadb-connector-cpp)

# 如果找到了，就使用 MARIADBPP 变量
if(MARIADBPP_FOUND)
    message(STATUS "Found MariaDB C++ connector: ${MARIADBPP_LIBRARIES}")
    set(MYSQL_LIBRARIES ${MARIADBPP_LIBRARIES})
    set(MYSQL_INCLUDE_DIRS ${MARIADBPP_INCLUDE_DIRS})
else()
    # 如果没找到，尝试找 mysqlcppconn
    pkg_check_modules(MYSQLCPPCONNECTOR mysqlcppconn)
    if(MYSQLCPPCONNECTOR_FOUND)
        message(STATUS "Found MySQL C++ connector")
        set(MYSQL_LIBRARIES ${MYSQLCPPCONNECTOR_LIBRARIES})
        set(MYSQL_INCLUDE_DIRS ${MYSQLCPPCONNECTOR_INCLUDE_DIRS})
    else()
        # 如果都没找到，尝试直接找库文件
        message(WARNING "No MySQL/MariaDB C++ connector found via pkg-config, trying direct search")
        find_path(MYSQL_INCLUDE_DIRS 
            NAMES mysql.h mysql/mysql.h mariadb/mysql.h
            PATHS /usr/include /usr/local/include /usr/include/mariadb
        )
        find_library(MYSQL_LIBRARIES
            NAMES mariadbcpp mysqlcppconn mysqlclient
            PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib64
        )
        if(MYSQL_INCLUDE_DIRS AND MYSQL_LIBRARIES)
            message(STATUS "Found MySQL/MariaDB libraries: ${MYSQL_LIBRARIES}")
        else()
            message(FATAL_ERROR "Could not find MySQL/MariaDB C++ connector. Please install mariadb-connector-cpp or mysql-connector-c++")
        endif()
    endif()
endif()

# 头文件路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hoop_timer
    ${CMAKE_CURRENT_SOURCE_DIR}/include/http
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tool
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lock
    ${CMAKE_CURRENT_SOURCE_DIR}/include/log
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sql
    ${CMAKE_CURRENT_SOURCE_DIR}/include/thread_pool
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mysql_connector  # 如果这个目录存在
    ${MYSQL_INCLUDE_DIRS}  # 使用找到的 MySQL include 路径
)

# 收集源文件 - 修正这里的变量名
file(GLOB_RECURSE SOURCES  # 改成 SOURCES，与后面的 add_executable 一致
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)

# 创建可执行文件 - 使用 SOURCES 变量
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} 
    Threads::Threads
    ${MYSQL_LIBRARIES}  # 使用 MYSQL_LIBRARIES 而不是 MYSQLCPPCONNECTOR_LIBRARIES
    rt
    dl  # 添加 dl 库，有时需要
)

# 如果有 pthread，显式添加（某些系统需要）
target_link_libraries(${PROJECT_NAME} pthread)

# 资源路径宏
target_compile_definitions(${PROJECT_NAME} PRIVATE
    RESOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/source/"
)

# 复制资源文件
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        ${CMAKE_CURRENT_BINARY_DIR}/bin/source
    COMMENT "Copying resource files"
)

# 打印找到的库信息（帮助调试）
message(STATUS "MySQL Include dirs: ${MYSQL_INCLUDE_DIRS}")
message(STATUS "MySQL Libraries: ${MYSQL_LIBRARIES}")