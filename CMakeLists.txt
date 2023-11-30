cmake_minimum_required(VERSION 3.16.3)
project(simClientApplication)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 17)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT COPPELIASIM_INCLUDE_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(COPPELIASIM_INCLUDE_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/include)
    else()
        set(COPPELIASIM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    endif()
endif()

set(COPPELIASIM_VERSION_CHECK_SRC "${CMAKE_BINARY_DIR}/sim_version_check.cpp")
set(COPPELIASIM_VERSION_CHECK_BIN "${CMAKE_BINARY_DIR}/sim_version_check")
file(WRITE ${COPPELIASIM_VERSION_CHECK_SRC} "
#include <iostream>
#include <simLib/simConst.h>
int main() {
    char sep = ';';
    std::cout
        << SIM_PROGRAM_VERSION_NB/10000 << sep
        << SIM_PROGRAM_VERSION_NB/100%100 << sep
        << SIM_PROGRAM_VERSION_NB%100 << sep
        << SIM_PROGRAM_REVISION_NB << sep
        << 0 << std::endl;
}
")
try_run(COPPELIASIM_VERSION_RUN_RESULT COPPELIASIM_VERSION_COMPILE_RESULT ${COPPELIASIM_VERSION_CHECK_BIN} ${COPPELIASIM_VERSION_CHECK_SRC} CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${COPPELIASIM_INCLUDE_DIR} RUN_OUTPUT_VARIABLE COPPELIASIM_VERSION_CHECK_OUTPUT)
if(${COPPELIASIM_VERSION_COMPILE_RESULT})
    if(${COPPELIASIM_VERSION_RUN_RESULT} EQUAL 0)
        list(GET COPPELIASIM_VERSION_CHECK_OUTPUT 0 COPPELIASIM_VERSION_MAJOR)
        list(GET COPPELIASIM_VERSION_CHECK_OUTPUT 1 COPPELIASIM_VERSION_MINOR)
        list(GET COPPELIASIM_VERSION_CHECK_OUTPUT 2 COPPELIASIM_VERSION_PATCH)
        list(GET COPPELIASIM_VERSION_CHECK_OUTPUT 3 COPPELIASIM_REVISION)
        set(COPPELIASIM_VERSION "${COPPELIASIM_VERSION_MAJOR}.${COPPELIASIM_VERSION_MINOR}.${COPPELIASIM_VERSION_PATCH}.${COPPELIASIM_REVISION}")
        set(COPPELIASIM_VERSION_STR "${COPPELIASIM_VERSION_MAJOR}.${COPPELIASIM_VERSION_MINOR}.${COPPELIASIM_VERSION_PATCH} rev${COPPELIASIM_REVISION}")
    else()
        message(FATAL_ERROR "Failed to run CoppeliaSim version check program")
        return()
    endif()
else()
    message(FATAL_ERROR "Failed to compile CoppeliaSim version check program")
    return()
endif()

set(SIM_VARIOUS_FILES OFF CACHE BOOL "Take some extra files from the various files repo.")
if(SIM_VARIOUS_FILES)
    include(FetchContent)
    FetchContent_Declare(simVariousFiles
        GIT_REPOSITORY https://github.com/CoppeliaRobotics/simVariousFiles
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/simVariousFiles
    )
    if(NOT simVariousFiles_POPULATED)
        FetchContent_Populate(simVariousFiles)
    endif()
    if(APPLE)
        set(CFBundleVersion "${COPPELIASIM_VERSION}")
        set(CFBundleShortVersionString "${COPPELIASIM_VERSION_STR}")
        configure_file(
            ${CMAKE_CURRENT_BINARY_DIR}/simVariousFiles/mac/Info.plist.in
            ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
        )
    endif()
endif()

set(COPPELIASIM_FLAVOR "edu" CACHE STRING "Choose the CoppeliaSim flavor.")
set_property(CACHE COPPELIASIM_FLAVOR PROPERTY STRINGS "edu" "player" "pro")

if(APPLE)
    if(DEFINED ENV{HOMEBREW_PREFIX})
        set(HOMEBREW_PREFIX "$ENV{HOMEBREW_PREFIX}")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()
endif()

set(QT_PATH "${HOMEBREW_PREFIX}/opt/qt" CACHE STRING "The path of Qt libraries.")

set(COPPELIASIM_LIBRARY NOTFOUND CACHE PATH "Path to the compiled CoppeliaSim dll/so/dylib file")

if(WIN32)
    set(RUN_WINDEPLOYQT OFF CACHE BOOL "Run windeployqt after build")
endif()
if(APPLE)
    set(RUN_MACDEPLOYQT OFF CACHE BOOL "Run macdeployqt after build")
endif()

add_definitions(-DQT_FRAMEWORK)

set(Qt Qt5 CACHE STRING "Qt version to use (e.g. Qt5)")
set_property(CACHE Qt PROPERTY STRINGS Qt5 Qt6)

find_package(Boost COMPONENTS program_options REQUIRED)
find_package(${Qt} COMPONENTS Core REQUIRED)

if(UNIX AND NOT APPLE)
    # from main.cpp:28 "required to have Lua extension libraries work under Linux. Strange..."
    set(NEED_LUA TRUE)
endif()

if(NEED_LUA)
    if(NOT LUA_INCLUDE_DIR OR NOT LUA_LIBRARIES)
        find_package(Lua 5.3 EXACT REQUIRED)
    endif()
    include_directories(${LUA_INCLUDE_DIR})
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

include_directories("${COPPELIASIM_INCLUDE_DIR}")

if(WIN32)
    add_definitions(-DWIN_SIM)
elseif(APPLE)
    add_definitions(-DMAC_SIM)
elseif(UNIX)
    add_definitions(-DLIN_SIM)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

set(SOURCES sourceCode/main.cpp "${COPPELIASIM_INCLUDE_DIR}/simLib/simLib.cpp")
add_executable(coppeliaSim ${SOURCES})
target_link_libraries(coppeliaSim PRIVATE Boost::program_options)
if(APPLE AND SIM_VARIOUS_FILES)
    set_target_properties(coppeliaSim PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
endif()
if(NEED_LUA)
    target_link_libraries(coppeliaSim PRIVATE ${LUA_LIBRARIES})
endif()
target_link_libraries(coppeliaSim PRIVATE Qt::Core)
if(WIN32)
    target_link_libraries(coppeliaSim PRIVATE winmm)
endif()
if(UNIX)
    target_compile_options(coppeliaSim PRIVATE -fvisibility=hidden)
endif()

if(COPPELIASIM_LIBRARY)
    if(NOT EXISTS "${COPPELIASIM_LIBRARY}")
        message(FATAL_ERROR "COPPELIASIM_LIBRARY is not a valid file (${COPPELIASIM_LIBRARY})")
    endif()
    add_custom_command(TARGET coppeliaSim POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E
        copy "${COPPELIASIM_LIBRARY}" "$<TARGET_FILE_DIR:coppeliaSim>"
        COMMENT "Copying libcoppeliaSim in place..."
    )
endif()

if(NOT QT_BIN_DIR)
    if(NOT QMAKE_EXECUTABLE)
        get_target_property(QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
    endif()
    get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
endif()
if(RUN_WINDEPLOYQT)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt REQUIRED HINTS "${QT_BIN_DIR}")
    add_custom_command(TARGET coppeliaSim POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E
            env PATH="${QT_BIN_DIR}" "${WINDEPLOYQT_EXECUTABLE}"
            "$<TARGET_FILE:coppeliaSim>"
        COMMENT "Running windeployqt..."
    )
endif()
if(APPLE)
    add_custom_command(TARGET coppeliaSim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:coppeliaSim>/../Resources"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:coppeliaSim>/../MacOS"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:coppeliaSim>/../Frameworks"
        COMMENT "Create all bundle subdirs..."
    )
endif()
if(APPLE AND SIM_VARIOUS_FILES)
    add_custom_command(TARGET coppeliaSim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/simVariousFiles/mac/icon/macIcon_${COPPELIASIM_FLAVOR}.icns "$<TARGET_FILE_DIR:coppeliaSim>/../Resources/icon.icns"
        COMMENT "Copy icon file..."
    )
endif()
if(RUN_MACDEPLOYQT)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt REQUIRED HINTS "${QT_BIN_DIR}")
    add_custom_command(TARGET coppeliaSim POST_BUILD
        COMMAND "${MACDEPLOYQT_EXECUTABLE}"
            "$<TARGET_FILE_DIR:coppeliaSim>/../.."
            -always-overwrite
        COMMENT "Running macdeployqt..."
    )
    find_package(Python3 REQUIRED)
    if(SIM_VARIOUS_FILES)
        add_custom_command(TARGET coppeliaSim POST_BUILD
            COMMAND "${Python3_EXECUTABLE}"
                ${CMAKE_CURRENT_BINARY_DIR}/simVariousFiles/mac/installation/macdeployqtfix/macdeployqtfix.py
                "$<TARGET_FILE:coppeliaSim>"
                "${QT_PATH}"
            COMMENT "Running macdeployqtfix..."
        )
    endif()
endif()

if(INSTALL_DIR)
    install(TARGETS coppeliaSim DESTINATION "${INSTALL_DIR}")
endif()
