cmake_minimum_required(VERSION 2.8.11)
project(simClientApplication)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_MACOSX_RPATH 1)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)
FetchContent_Declare(simVariousFiles
    GIT_REPOSITORY https://github.com/CoppeliaRobotics/simVariousFiles
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/simVariousFiles
)
if(NOT simVariousFiles_POPULATED)
    FetchContent_Populate(simVariousFiles)
endif()

set(COPPELIASIM_FLAVOR "edu" CACHE STRING "Choose the CoppeliaSim flavor." FORCE)
set_property(CACHE COPPELIASIM_FLAVOR PROPERTY STRINGS "edu" "player" "pro")

set(QT_PATH "/usr/local/opt/qt" CACHE STRING "The path of Qt libraries." FORCE)

set(WITH_QT true CACHE BOOL "Enable Qt")
set(COPPELIASIM_LIBRARY NOTFOUND CACHE PATH "Path to the compiled CoppeliaSim dll/so/dylib file")
set(INSTALL_DIR "" CACHE PATH "If specified, it will used as install destination")
if(INSTALL_DIR)
    if(NOT EXISTS "${INSTALL_DIR}")
        message(FATAL_ERROR "'${INSTALL_DIR}' does not exist")
    elseif(NOT IS_DIRECTORY "${INSTALL_DIR}")
        message(FATAL_ERROR "'${INSTALL_DIR}' is not a directory")
    endif()
    message(STATUS "INSTALL_DIR: ${INSTALL_DIR}")
endif()

if(WIN32)
    set(RUN_WINDEPLOYQT OFF CACHE BOOL "Run windeployqt after build")
endif()
if(APPLE)
    set(RUN_MACDEPLOYQT OFF CACHE BOOL "Run macdeployqt after build")
endif()

if(WITH_QT)
    add_definitions(-DQT_FRAMEWORK)
    find_package(Qt5 COMPONENTS Core REQUIRED)
else()
    add_definitions(-DSIM_WITHOUT_QT_AT_ALL)
endif()

if(UNIX AND NOT APPLE)
    # from main.cpp:28 "required to have Lua extension libraries work under Linux. Strange..."
    set(NEED_LUA TRUE)
endif()

if(NEED_LUA)
    find_package(Lua 5.3 EXACT REQUIRED)
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

include_directories("../include")

if(WIN32)
    add_definitions(-DWIN_SIM)
elseif(APPLE)
    add_definitions(-DMAC_SIM)
elseif(UNIX)
    add_definitions(-DLIN_SIM)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

set(SOURCES main.cpp ../common/simLib.cpp)
add_executable(coppeliaSim ${SOURCES})
if(APPLE)
    set_target_properties(coppeliaSim PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/simVariousFiles/mac/Info.plist)
endif()
if(NEED_LUA)
    target_link_libraries(coppeliaSim ${LUA_LIBRARIES})
endif()
if(WITH_QT)
    target_link_libraries(coppeliaSim Qt5::Core)
endif()
if(WIN32)
    target_link_libraries(coppeliaSim winmm)
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

if(WITH_QT)
    if(NOT QT_BIN_DIR)
        if(NOT QMAKE_EXECUTABLE)
            get_target_property(QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
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
