cmake_minimum_required(VERSION 3.9)

set(spdlog_DIR /usr/local/spdlog/lib/spdlog/cmake)
set(Poco_DIR /usr/local/poco/lib/cmake/poco)
set(cpprestsdk_DIR /usr/local/cpprest-sdk/lib/cmake/cpprestsdk)
project(micro_server)
add_compile_options(-std=c++11 -fpermissive)
find_package(cpprestsdk REQUIRED)
find_package(spdlog REQUIRED)
find_package(Poco REQUIRED Foundation Util Net)

aux_source_directory(. SRCS)
add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} spdlog::spdlog_header_only cpprestsdk::cpprest boost_system ssl crypto ${Poco_LIBRARIES})
