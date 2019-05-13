cmake_minimum_required(VERSION 2.8.11)
project(v_repClientApplication)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_MACOSX_RPATH 1)

set(WITH_QT true CACHE BOOL "Enable Qt")
set(VREP_LIBRARY NOTFOUND CACHE STRING "Path to the compiled V-REP dll/so/dylib file")
set(QSCINTILLA_LIBRARY NOTFOUND CACHE STRING "Path to the QScintilla dll/so/dylib file")

if(WITH_QT)
    add_definitions(-DQT_FRAMEWORK)
    find_package(Qt5Core REQUIRED)
else()
    add_definitions(-DSIM_WITHOUT_QT_AT_ALL)
endif()
find_package(Lua REQUIRED)

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

include_directories("../include")
include_directories(${LUA_INCLUDE_DIR})

if(WIN32)
    add_definitions(-DWIN_VREP)
elseif(APPLE)
    add_definitions(-DMAC_VREP)
elseif(UNIX)
    add_definitions(-DLIN_VREP)
endif()

set(SOURCES main.cpp ../common/v_repLib.cpp)
if(APPLE)
    add_executable(vrep MACOSX_BUNDLE ${SOURCES})
else()
    add_executable(vrep ${SOURCES})
endif()
target_link_libraries(vrep ${LUA_LIBRARIES})
if(WITH_QT)
    target_link_libraries(vrep Qt5::Core)
endif()
if(WIN32)
    target_link_libraries(vrep -lwinmm)
endif()

if(NOT VREP_LIBRARY)
    message(FATAL_ERROR "VREP_LIBRARY is not set")
elseif(NOT EXISTS "${VREP_LIBRARY}")
    message(FATAL_ERROR "VREP_LIBRARY is not a valid file (${VREP_LIBRARY})")
else()
    add_custom_command(TARGET vrep POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E
        copy "${VREP_LIBRARY}" "$<TARGET_FILE_DIR:vrep>"
        COMMENT "Copying libv_rep in place..."
    )
endif()

if(WITH_QT)
    if(NOT QT_BIN_DIR)
        if(NOT QMAKE_EXECUTABLE)
            get_target_property(QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
        endif()
        get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
    endif()
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BIN_DIR}")
    if(WIN32)
        add_custom_command(TARGET vrep POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E
                env PATH="${QT_BIN_DIR}" "${WINDEPLOYQT_EXECUTABLE}"
                "$<TARGET_FILE:vrep>"
            COMMENT "Running windeployqt..."
        )
    endif()
    if(APPLE)
        add_custom_command(TARGET vrep POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                "$<TARGET_FILE_DIR:vrep>/../.."
                -always-overwrite
            COMMENT "Running macdeployqt..."
        )
    endif()
endif()
