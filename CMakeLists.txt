cmake_minimum_required(VERSION 2.8)
project(nss_custom)
set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 1)
set(${PROJECT_NAME}_VERSION
    "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}")
option(USE_SYSLOG "Enable syslog support" ON)
option(USE_LOCAL "Build to work local" OFF)
option(USE_TEST "Build test application" OFF)
if(USE_SYSLOG)
    message(STATUS "Syslog enabled")
endif()
if(USE_LOCAL)
    message(STATUS "Work local enabled")
    SET(CONF_FILE ${CMAKE_CURRENT_BINARY_DIR}/nss_custom.conf)
endif()
if(USE_TEST)
    message(STATUS "Test application enabled")
endif()


#find_file(ERROR_H error.h)
find_file(NSS_H ndss.h)
#if(NSS_H_NOTFOUND)
#    message(FATAL_ERROR "nss.h not found")
#endif()

find_file(SYSLOG_H syslog.h)
if(SYSLOG_H_NOTFOUND)
    message(WARNING "syslog.h not found")
    SET(USE_SYSLOG 0)
endif()

if(NOT DEFINED CONF_FILE)
    SET(CONF_FILE /etc/nss_custom.conf)
endif()

configure_file(include/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})



add_library(${PROJECT_NAME} SHARED
    nss_custom.c)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR}
    VERSION ${${PROJECT_NAME}_VERSION})

if(USE_TEST)
    add_executable(nss_custom_test
        nss_custom_test.c)
    target_link_libraries(nss_custom_test ${PROJECT_NAME})
endif()

#add_custom_target(
