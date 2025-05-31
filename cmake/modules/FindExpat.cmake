# - Find expat 
# Find the native EXPAT headers and libraries. 
# 
#  EXPAT_INCLUDE_DIRS - where to find expat.h, etc. 
#  EXPAT_LIBRARIES    - List of libraries when using expat. 
#  EXPAT_LIBRARY_DEBUG - Debug version of Library 
#  EXPAT_LIBRARY_RELEASE - Release Version of Library 
#  EXPAT_FOUND        - True if expat found. 


############################################ 
# 
# Check the existence of the libraries. 
# 
############################################ 
# This macro was taken directly from the FindQt4.cmake file that is included 
# with the CMake distribution. This is NOT my work. All work was done by the 
# original authors of the FindQt4.cmake file. Only minor modifications were 
# made to remove references to Qt and make this file more generally applicable 
######################################################################### 

MACRO (ADJUST_LIB_VARS basename) 
  IF (${basename}_INCLUDE_DIR) 

  # if only the release version was found, set the debug variable also to the release version 
  IF (${basename}_LIBRARY_RELEASE AND NOT ${basename}_LIBRARY_DEBUG) 
    SET(${basename}_LIBRARY_DEBUG ${${basename}_LIBRARY_RELEASE}) 
    SET(${basename}_LIBRARY       ${${basename}_LIBRARY_RELEASE}) 
    SET(${basename}_LIBRARIES     ${${basename}_LIBRARY_RELEASE}) 
  ENDIF (${basename}_LIBRARY_RELEASE AND NOT ${basename}_LIBRARY_DEBUG) 

  # if only the debug version was found, set the release variable also to the debug version 
  IF (${basename}_LIBRARY_DEBUG AND NOT ${basename}_LIBRARY_RELEASE) 
    SET(${basename}_LIBRARY_RELEASE ${${basename}_LIBRARY_DEBUG}) 
    SET(${basename}_LIBRARY         ${${basename}_LIBRARY_DEBUG}) 
    SET(${basename}_LIBRARIES       ${${basename}_LIBRARY_DEBUG}) 
  ENDIF (${basename}_LIBRARY_DEBUG AND NOT ${basename}_LIBRARY_RELEASE) 
  IF (${basename}_LIBRARY_DEBUG AND ${basename}_LIBRARY_RELEASE) 
    # if the generator supports configuration types then set 
    # optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value 
    IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE) 
      SET(${basename}_LIBRARY       optimized 
${${basename}_LIBRARY_RELEASE} debug ${${basename}_LIBRARY_DEBUG}) 
    ELSE(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE) 
      # if there are no configuration types and CMAKE_BUILD_TYPE has no value 
      # then just use the release libraries 
      SET(${basename}_LIBRARY       ${${basename}_LIBRARY_RELEASE} ) 
    ENDIF(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE) 
    SET(${basename}_LIBRARIES       optimized 
${${basename}_LIBRARY_RELEASE} debug ${${basename}_LIBRARY_DEBUG}) 
  ENDIF (${basename}_LIBRARY_DEBUG AND ${basename}_LIBRARY_RELEASE) 

  SET(${basename}_LIBRARY ${${basename}_LIBRARY} CACHE FILEPATH "The 
${basename} library") 

  IF (${basename}_LIBRARY) 
    SET(${basename}_FOUND 1) 
  ENDIF (${basename}_LIBRARY) 

ENDIF (${basename}_INCLUDE_DIR ) 

  # Make variables changeble to the advanced user 
  MARK_AS_ADVANCED(${basename}_LIBRARY ${basename}_LIBRARY_RELEASE 
${basename}_LIBRARY_DEBUG ${basename}_INCLUDE_DIR ) 
ENDMACRO (ADJUST_LIB_VARS) 





# Look for the header file. 

# MESSAGE (STATUS "Finding expat library and headers..." ) 
SET (EXPAT_DEBUG 0) 

SET(EXPAT_INCLUDE_SEARCH_DIRS
    ${EXPAT_INCLUDE_SEARCH_DIRS}
    ${EXPAT_INSTALL}/include
  $ENV{EXPAT_INSTALL}/include
  /usr/local/opt/expat/include
) 

SET (EXPAT_LIB_SEARCH_DIRS 
    ${EXPAT_LIB_SEARCH_DIRS}
    ${EXPAT_INSTALL}/lib
  $ENV{EXPAT_INSTALL}/lib
  /usr/local/opt/expat/lib
  ) 

SET (EXPAT_BIN_SEARCH_DIRS
    ${EXPAT_INSTALL}/bin
  $ENV{EXPAT_INSTALL}/bin
  /usr/local/opt/expat/bin
) 

FIND_PATH(EXPAT_INCLUDE_DIR 
  NAMES expat.h 
  PATHS ${EXPAT_INCLUDE_SEARCH_DIRS} 
  NO_DEFAULT_PATH 
) 

IF (WIN32 AND NOT MINGW) 
    SET (EXPAT_SEARCH_DEBUG_NAMES "expatdll_D;libexpat_D;libexpatd") 
    SET (EXPAT_SEARCH_RELEASE_NAMES "expatdll;libexpat") 
ELSE (WIN32 AND NOT MINGW) 
    SET (EXPAT_SEARCH_DEBUG_NAMES "expat_debug") 
    SET (EXPAT_SEARCH_RELEASE_NAMES "expat") 
ENDIF(WIN32 AND NOT MINGW) 


#IF (EXPAT_DEBUG) 
message (STATUS "EXPAT_INCLUDE_SEARCH_DIRS: ${EXPAT_INCLUDE_SEARCH_DIRS}") 
message (STATUS "EXPAT_LIB_SEARCH_DIRS: ${EXPAT_LIB_SEARCH_DIRS}") 
message (STATUS "EXPAT_BIN_SEARCH_DIRS: ${EXPAT_BIN_SEARCH_DIRS}") 
message (STATUS "EXPAT_SEARCH_RELEASE_NAMES: ${EXPAT_SEARCH_RELEASE_NAMES}") 
message (STATUS "EXPAT_SEARCH_DEBUG_NAMES: ${EXPAT_SEARCH_DEBUG_NAMES}") 
#ENDIF (EXPAT_DEBUG) 

# Look for the library. 
FIND_LIBRARY(EXPAT_LIBRARY_DEBUG 
  NAMES ${EXPAT_SEARCH_DEBUG_NAMES} 
  PATH_SUFFIXES Debug/dynamic/utf8
  PATHS ${EXPAT_LIB_SEARCH_DIRS} 
  NO_DEFAULT_PATH 
  ) 

FIND_LIBRARY(EXPAT_LIBRARY_RELEASE 
  NAMES ${EXPAT_SEARCH_RELEASE_NAMES} 
  PATH_SUFFIXES Release/dynamic/utf8
  PATHS ${EXPAT_LIB_SEARCH_DIRS} 
  NO_DEFAULT_PATH 
  ) 

SET (EXPAT_XMLWF_PROG_NAME "xmlwf") 
IF (WIN32) 
    SET (EXPAT_XMLWF_PROG_NAME "xmlwf.exe") 
ENDIF(WIN32) 

FIND_PROGRAM(EXPAT_XMLWF_PROG 
    NAMES ${EXPAT_XMLWF_PROG_NAME} 
    PATHS ${EXPAT_BIN_SEARCH_DIRS} 
    NO_DEFAULT_PATH 
) 
MARK_AS_ADVANCED(EXPAT_XMLWF_PROG) 

#IF (EXPAT_DEBUG) 
MESSAGE(STATUS "EXPAT_INCLUDE_DIR: ${EXPAT_INCLUDE_DIR}") 
MESSAGE(STATUS "EXPAT_LIBRARY_DEBUG: ${EXPAT_LIBRARY_DEBUG}") 
MESSAGE(STATUS "EXPAT_LIBRARY_RELEASE: ${EXPAT_LIBRARY_RELEASE}") 
MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}") 
#endif(EXPAT_DEBUG) 

ADJUST_LIB_VARS(EXPAT) 

# MESSAGE( STATUS "EXPAT_LIBRARY: ${EXPAT_LIBRARY}") 

# Copy the results to the output variables. 
IF(EXPAT_INCLUDE_DIR AND EXPAT_LIBRARY) 
  SET(EXPAT_FOUND 1) 
  SET(EXPAT_LIBRARIES ${EXPAT_LIBRARY}) 
  SET(EXPAT_INCLUDE_DIRS ${EXPAT_INCLUDE_DIR}) 
  IF (EXPAT_LIBRARY_DEBUG) 
    GET_FILENAME_COMPONENT(EXPAT_LIBRARY_PATH ${EXPAT_LIBRARY_DEBUG} PATH) 
    SET(EXPAT_LIB_DIR  ${EXPAT_LIBRARY_PATH}) 
  ELSEIF(EXPAT_LIBRARY_RELEASE) 
    GET_FILENAME_COMPONENT(EXPAT_LIBRARY_PATH ${EXPAT_LIBRARY_RELEASE} PATH) 
    SET(EXPAT_LIB_DIR  ${EXPAT_LIBRARY_PATH}) 
  ENDIF(EXPAT_LIBRARY_DEBUG) 

  IF (EXPAT_XMLWF_PROG) 
    GET_FILENAME_COMPONENT(EXPAT_BIN_PATH ${EXPAT_XMLWF_PROG} PATH) 
    SET(EXPAT_BIN_DIR  ${EXPAT_BIN_PATH}) 
  ENDIF (EXPAT_XMLWF_PROG) 

ELSE(EXPAT_INCLUDE_DIR AND EXPAT_LIBRARY) 
  SET(EXPAT_FOUND 0) 
  SET(EXPAT_LIBRARIES) 
  SET(EXPAT_INCLUDE_DIRS) 
ENDIF(EXPAT_INCLUDE_DIR AND EXPAT_LIBRARY) 

# Report the results. 
IF(NOT EXPAT_FOUND) 
  SET(EXPAT_DIR_MESSAGE 
    "EXPAT was not found. Make sure EXPAT_LIBRARY and 
EXPAT_INCLUDE_DIR are set or set the EXPAT_INSTALL environment 
variable.") 
  IF(NOT EXPAT_FIND_QUIETLY) 
    MESSAGE(STATUS "${EXPAT_DIR_MESSAGE}") 
  ELSE(NOT EXPAT_FIND_QUIETLY) 
    IF(EXPAT_FIND_REQUIRED) 
      # MESSAGE(FATAL_ERROR "${EXPAT_DIR_MESSAGE}") 
      MESSAGE(FATAL_ERROR "Expat was NOT found and is Required by this project") 
    ENDIF(EXPAT_FIND_REQUIRED) 
  ENDIF(NOT EXPAT_FIND_QUIETLY) 
ENDIF(NOT EXPAT_FOUND) 


IF (EXPAT_FOUND) 
  INCLUDE(CheckSymbolExists) 
  ############################################# 
  # Find out if EXPAT was build using dll's 
  ############################################# 
  # Save required variable 
  SET(CMAKE_REQUIRED_INCLUDES_SAVE ${CMAKE_REQUIRED_INCLUDES}) 
  SET(CMAKE_REQUIRED_FLAGS_SAVE    ${CMAKE_REQUIRED_FLAGS}) 
  # Add EXPAT_INCLUDE_DIR to CMAKE_REQUIRED_INCLUDES 
  SET(CMAKE_REQUIRED_INCLUDES 
"${CMAKE_REQUIRED_INCLUDES};${EXPAT_INCLUDE_DIRS}") 

  CHECK_SYMBOL_EXISTS(EXPAT_BUILT_AS_DYNAMIC_LIB "expat_config.h" 
HAVE_EXPAT_DLL) 

    IF (HAVE_EXPAT_DLL STREQUAL "TRUE") 
        SET (HAVE_EXPAT_DLL "1") 
    ENDIF (HAVE_EXPAT_DLL STREQUAL "TRUE") 

  # Restore CMAKE_REQUIRED_INCLUDES and CMAKE_REQUIRED_FLAGS variables 
  SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVE}) 
  SET(CMAKE_REQUIRED_FLAGS    ${CMAKE_REQUIRED_FLAGS_SAVE}) 
  # 
  ############################################# 

ENDIF (EXPAT_FOUND) 