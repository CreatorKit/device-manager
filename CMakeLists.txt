# Project Properties
####################
CMAKE_MINIMUM_REQUIRED (VERSION 2.6.2)

PROJECT(device_manager)
# This will be extended to take the version from SCM
# Version config can be simplified from 3.0+
SET(device_manager_VERSION_MAJOR 0)
SET(device_manager_VERSION_MINOR 3)
SET(device_manager_VERSION_PATCH 0)
SET(device_manager_VERSION
    ${device_manager_VERSION_MAJOR}.${device_manager_VERSION_MINOR}.${device_manager_VERSION_PATCH})

# Options
###################
SET(CMAKE_VERBOSE_MAKEFILE 1)
SET(CMAKE_BUILD_TYPE DEBUG) # Options MINSIZEREL, RELEASE, DEBUG
SET(DOCS_INTERNAL 1 CACHE BOOL "enable internal docs generation")

# Includes
##########
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Paths
########
ADD_SUBDIRECTORY(src)
