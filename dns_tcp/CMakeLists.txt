cmake_minimum_required (VERSION 2.6)
if(POLICY CMP0026)
   cmake_policy(SET CMP0022 OLD)
endif()

IF(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
ENDIF(POLICY CMP0042)

project (sftap_dns)

find_package(Boost COMPONENTS iostreams REQUIRED)

set(LIBS ${Boost_IOSTREAMS_LIBRARY})

IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  set(LIBS ${LIBS} resolv)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(sftap_dns_tcp main.cpp fabs_bytes.cpp fabs_id.cpp)
target_link_libraries(sftap_dns_tcp ${LIBS})
