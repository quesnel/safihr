CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(safihr CXX)
SET (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
ENABLE_TESTING()

SET(MODEL_MAJOR 0)
SET(MODEL_MINOR 1)
SET(MODEL_PATCH 0)
SET(MODEL_NAME "${PROJECT_NAME}-${MODEL_MAJOR}.${MODEL_MINOR}.${MODEL_PATCH}")
SET(VLE_NAME "${PROJECT_NAME}-${MODEL_MAJOR}.${MODEL_MINOR}")

##
## Options for compilation of package
##

OPTION(WITH_TEST "will build the test [default: ON]" ON)
OPTION(WITH_DOC "will compile doc and install it [default: OFF]" OFF)
OPTION(WITH_WARNINGS "will compile with g++ warnings [default: ON]" ON)

if (CMAKE_BUILD_TYPE STREQUAL "")
  SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif ()

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
  if (NOT WITH_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -std=c++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++11")
  else (NOT WITH_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
  endif (NOT WITH_WARNINGS)
  if ("${CMAKE_BUILD_TYPE}" EQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" EQUAL "RelWithDebInfo")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")
  endif ()
endif ()


##
## Modules
##

INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckLibraryExists)
INCLUDE(CMakeDetermineCCompiler)

##
## Check libraries with pkgconfig
##

set(VLE_DEBUG 0)
find_package(VLE REQUIRED)

##
## Check VLE's packages
##

set (VleCheckPackage_DIR "${CMAKE_SOURCE_DIR}/cmake")
FIND_PACKAGE(VleCheckPackage REQUIRED)
set (VleCheckAndDeclareGeneratedModels_DIR
  "${CMAKE_SOURCE_DIR}/cmake")
FIND_PACKAGE(VleCheckAndDeclareGeneratedModels REQUIRED)
SET(VLE_ABI_VERSION 1.1)

VLE_CHECK_PACKAGE(DECISION2 vle.extension.decision2)

if (NOT DECISION2_FOUND)
     message(SEND_ERROR "Missing vle.extension.decision2")
endif (NOT DECISION2_FOUND)

##
## Find boost
##

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost COMPONENTS unit_test_framework date_time)

##
## Generate the doxygen
##

FIND_PACKAGE(Doxygen)
IF (DOXYGEN)
  SET(DOXYGEN_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
  SET(DOXYGEN_OUTPUT_MODELING_DIR "${PROJECT_BINARY_DIR}/doxygen/modeling")
  SET(DOXYGEN_OUTPUT_SOURCE_DIR "${PROJECT_BINARY_DIR}/doxygen/sources")
  CONFIGURE_FILE("cmake/doxygen-modeling.conf.in"
    "${PROJECT_BINARY_DIR}/doxygen-modeling.conf")
  CONFIGURE_FILE("cmake/doxygen-sources.conf.in"
    "${PROJECT_BINARY_DIR}/doxygen-sources.conf")
  FILE(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/doxygen")
  FILE(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/doxygen/modeling")
  FILE(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/doxygen/sources")

  ADD_CUSTOM_COMMAND(
    OUTPUT "${PROJECT_BINARY_DIR}/doxygen/modeling/index.html"
    DEPENDS "${PROJECT_BINARY_DIR}/doxygen-modeling.conf"
    COMMAND "${DOXYGEN}"
    ARGS "${PROJECT_BINARY_DIR}/doxygen-modeling.conf")

  ADD_CUSTOM_COMMAND(
    OUTPUT "${PROJECT_BINARY_DIR}/doxygen/sources/index.html"
    DEPENDS "${PROJECT_BINARY_DIR}/doxygen-sources.conf"
    COMMAND "${DOXYGEN}"
    ARGS "${PROJECT_BINARY_DIR}/doxygen-sources.conf")

  ADD_CUSTOM_TARGET(doc_modeling ALL DEPENDS
    "${PROJECT_BINARY_DIR}/doxygen-modeling.conf"
    "${PROJECT_BINARY_DIR}/doxygen/modeling/index.html" VERBATIM)
  ADD_CUSTOM_TARGET(doc_sources ALL DEPENDS
    "${PROJECT_BINARY_DIR}/doxygen-sources.conf"
    "${PROJECT_BINARY_DIR}/doxygen/sources/index.html" VERBATIM)

  INSTALL(DIRECTORY "${PROJECT_BINARY_DIR}/doxygen/modeling/html" DESTINATION
    "doc/html/modeling")
  INSTALL(DIRECTORY "${PROJECT_BINARY_DIR}/doxygen/sources/html" DESTINATION
    "doc/html/sources")
ENDIF (DOXYGEN)

##
## Define function to simplify the definition of simulations plugins and test
##

FUNCTION(DeclareDevsDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareDevsDynamics name sources)

FUNCTION(DeclareCellDevsDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${CELLDEVS_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${CELLDEVS_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareCellDevsDynamics name sources)

FUNCTION(DeclareCellQssDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${CELLQSS_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${CELLQSS_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareCellQssDynamics name sources)

FUNCTION(DeclareDecisionDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${DECISION_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${DECISION_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareDecisionDynamics name sources)

FUNCTION(DeclareDecisionDynamics2 name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${DECISION2_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${DECISION2_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareDecisionDynamics2 name sources)

FUNCTION(DeclareDifferenceEquationDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${DIFFERENCE_EQU_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${DIFFERENCE_EQU_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareDifferenceEquationDynamics name sources)

FUNCTION(DeclareFsaDifferenceEquationDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${DIFFERENCE_EQU_LIBRARIES} ${FSA_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareFsaDifferenceEquationDynamics name sources)

FUNCTION(DeclareDifferentialEquationDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${DIFFERENTIAL_EQU_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${DIFFERENTIAL_EQU_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareDifferentialEquationDynamics name sources)

FUNCTION(DeclareDsDevsDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${DSDEVS_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${DSDEVS_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareDsDevsDynamics name sources)

FUNCTION(DeclareFsaDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${FSA_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${FSA_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclareFsaDynamics name sources)

FUNCTION(DeclarePetrinetDynamics name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${PETRINET_INCLUDE_DIRS}
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_LIBRARY(${name} MODULE ${sources})
  TARGET_LINK_LIBRARIES(${name} ${PETRINET_LIBRARIES}
    ${VLE_LIBRARIES} ${Boost_LIBRARIES})
  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION plugins/simulator
    LIBRARY DESTINATION plugins/simulator)
ENDFUNCTION(DeclarePetrinetDynamics name sources)

FUNCTION(DeclareVleTest name sources)
  INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${VLE_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(
    ${VLE_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS})
  ADD_EXECUTABLE(${name} ${sources})
  TARGET_LINK_LIBRARIES(${name}
    ${VLE_LIBRARIES}
    ${Boost_LIBRARIES}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY})
  ADD_TEST(${name} ${name})
ENDFUNCTION(DeclareVleTest name sources)

##
## Subdirectory
##

add_subdirectory(data)
if (WITH_DOC)
  add_subdirectory(doc)
endif ()
add_subdirectory(exp)
add_subdirectory(src)

if (Boost_UNIT_TEST_FRAMEWORK_FOUND AND WITH_TEST)
  add_subdirectory(test)
endif (Boost_UNIT_TEST_FRAMEWORK_FOUND AND WITH_TEST)

##
## CPack configuration
##

INSTALL(FILES Authors.txt Description.txt License.txt News.txt Readme.txt
  DESTINATION .)

INCLUDE(CMakeCPack.cmake)
