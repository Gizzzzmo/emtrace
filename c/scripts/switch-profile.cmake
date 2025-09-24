#!/usr/bin/env -S cmake -P

cmake_minimum_required(VERSION 3.25.0)

file(GLOB profiles 
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}/../profiles 
    "${CMAKE_CURRENT_LIST_DIR}/../profiles/*.*.json"
)

set(types "")
foreach(profile ${profiles})
    if (profile MATCHES "^([^.]*)\\.([^.]*)\\.json$")
        if(NOT CMAKE_MATCH_2 IN_LIST types)
            list(APPEND types ${CMAKE_MATCH_2})
        endif()
        list(APPEND ${CMAKE_MATCH_2}_profiles ${CMAKE_MATCH_1})
        if(${CMAKE_MATCH_1}_EXISTS)
            unset(${CMAKE_MATCH_1}_TYPE)
        else()
            set(${CMAKE_MATCH_1}_EXISTS TRUE)
            set(${CMAKE_MATCH_1}_TYPE ${CMAKE_MATCH_2})
        endif()
    endif()
endforeach()

if(NOT DEFINED CMAKE_ARGV3)
    foreach(type ${types})
        if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../profiles/.current_profile_${type})
            file(READ ${CMAKE_CURRENT_LIST_DIR}/../profiles/.current_profile_${type} current_profile)
            if(current_profile IN_LIST ${type}_profiles)
                message("${type} profiles:")
            else()
                message("${type} profiles (unknown profile currently selected):")
            endif()
        else()
            message("${type} profiles (none currently selected):")
        endif()
        foreach(profile ${${type}_profiles})
            if(profile STREQUAL current_profile)
                message("  ${profile} (current)")
            else()
                message("  ${profile}")
            endif()
        endforeach()
    endforeach()
    return()
endif()

set(PROFILE ${CMAKE_ARGV3})
if(NOT ${PROFILE}_EXISTS)
    message(FATAL_ERROR "Profile ${PROFILE} does not exist.")
endif()

if(NOT CMAKE_ARGV4)
    if(DEFINED ${PROFILE}_TYPE)
        set(TYPE ${${PROFILE}_TYPE})
    elseif(${PROFILE}_EXISTS)
        message(FATAL_ERROR "Profile ${PROFILE} is not unique. Please specify a type.")
    else()
        message(FATAL_ERROR "Profile ${PROFILE} does not exist.")
    endif()
else()
    set(TYPE ${CMAKE_ARGV4})
endif()

if(NOT TYPE IN_LIST types)
    message(FATAL_ERROR "Profile type ${TYPE} does not exist.")
endif()

if(NOT PROFILE IN_LIST ${TYPE}_profiles)
    message(FATAL_ERROR "Profile ${PROFILE} does not exist for type ${TYPE}.")
endif()

execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE BRANCH_NAME
    RESULT_VARIABLE result
)
if(result)
    message(FATAL_ERROR ${commit_hash})
endif()
string(REPLACE "\n" "" BRANCH_NAME ${BRANCH_NAME})

execute_process(
    COMMAND git rev-parse HEAD
    OUTPUT_VARIABLE COMMIT_HASH
    RESULT_VARIABLE result
)

if(result)
    message(FATAL_ERROR ${commit_hash})
endif()
string(REPLACE "\n" "" COMMIT_HASH ${COMMIT_HASH})

if(BRANCH_NAME STREQUAL HEAD)
    set(BUILD_DIR ${COMMIT_HASH})
else()
    set(BUILD_DIR ${BRANCH_NAME})
endif()

message("Switching ${TYPE} to profile ${PROFILE}")

configure_file(${CMAKE_CURRENT_LIST_DIR}/../profiles/${PROFILE}.${TYPE}.json ${CMAKE_CURRENT_LIST_DIR}/../profiles/${TYPE}.json @ONLY)
file(WRITE ${CMAKE_CURRENT_LIST_DIR}/../profiles/.current_profile_${TYPE} ${PROFILE})
