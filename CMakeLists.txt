cmake_minimum_required(VERSION 2.8.11)
project(simClientApplication)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_MACOSX_RPATH 1)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR})

set(WITH_QT true CACHE BOOL "Enable Qt")
set(COPPELIASIM_LIBRARY NOTFOUND CACHE PATH "Path to the compiled CoppeliaSim dll/so/dylib file")

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
    add_definitions(-DWIN_SIM)
elseif(APPLE)
    add_definitions(-DMAC_SIM)
elseif(UNIX)
    add_definitions(-DLIN_SIM)
endif()

set(SOURCES main.cpp ../common/simLib.cpp)
add_executable(coppeliaSim ${SOURCES})
if(APPLE)
    set_target_properties(coppeliaSim PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif()
target_link_libraries(coppeliaSim ${LUA_LIBRARIES})
if(WITH_QT)
    target_link_libraries(coppeliaSim Qt5::Core)
endif()
if(WIN32)
    target_link_libraries(coppeliaSim -lwinmm)
endif()

if(NOT COPPELIASIM_LIBRARY)
    message(FATAL_ERROR "COPPELIASIM_LIBRARY is not set")
elseif(NOT EXISTS "${COPPELIASIM_LIBRARY}")
    message(FATAL_ERROR "COPPELIASIM_LIBRARY is not a valid file (${COPPELIASIM_LIBRARY})")
else()
    add_custom_command(TARGET coppeliaSim POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E
        copy "${COPPELIASIM_LIBRARY}" "$<TARGET_FILE_DIR:coppeliaSim>"
        COMMENT "Copying libcoppeliaSim in place..."
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
        add_custom_command(TARGET coppeliaSim POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E
                env PATH="${QT_BIN_DIR}" "${WINDEPLOYQT_EXECUTABLE}"
                "$<TARGET_FILE:coppeliaSim>"
            COMMENT "Running windeployqt..."
        )
    endif()
    if(APPLE)
        add_custom_command(TARGET coppeliaSim POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                "$<TARGET_FILE_DIR:coppeliaSim>/../.."
                -always-overwrite
            COMMENT "Running macdeployqt..."
        )
    endif()
endif()
