cmake_minimum_required (VERSION 2.6)
cmake_policy(SET CMP0022 OLD)
cmake_policy(SET CMP0042 NEW)
project (stap_dns)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(LIBS resolv)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(stap_dns main.cpp)
target_link_libraries(stap_dns ${LIBS})
