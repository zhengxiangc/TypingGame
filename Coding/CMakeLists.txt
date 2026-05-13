cmake_minimum_required(VERSION 3.19)
project(TypingGame LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Widgets Multimedia LinguistTools Test)

qt_standard_project_setup()

set(TG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Core rules (Qt Test links this only — avoids pulling Multimedia into ctest)
qt_add_library(typinggame_logic STATIC
    "${TG_SRC}/controller/gamecontroller.cpp"
    "${TG_SRC}/controller/gamecontroller.h"
)

target_include_directories(typinggame_logic PUBLIC "${TG_SRC}")

target_link_libraries(typinggame_logic PUBLIC Qt::Core)

target_compile_features(typinggame_logic PUBLIC cxx_std_20)

# Config + background services (GUI app only)
qt_add_library(typinggame_infra STATIC
    "${TG_SRC}/config/resourceconfig.cpp"
    "${TG_SRC}/config/resourceconfig.h"
    "${TG_SRC}/services/asynclogger.cpp"
    "${TG_SRC}/services/asynclogger.h"
    "${TG_SRC}/services/eventtracker.cpp"
    "${TG_SRC}/services/eventtracker.h"
    "${TG_SRC}/services/sfxid.h"
    "${TG_SRC}/services/sfxplayer.cpp"
    "${TG_SRC}/services/sfxplayer.h"
    "${TG_SRC}/services/bgmplayer.cpp"
    "${TG_SRC}/services/bgmplayer.h"
    "${TG_SRC}/services/gameaudioservice.cpp"
    "${TG_SRC}/services/gameaudioservice.h"
    "${TG_SRC}/services/deepseekwordclient.cpp"
    "${TG_SRC}/services/deepseekwordclient.h"
    "${TG_SRC}/services/threadsafequeue.h"
)

target_include_directories(typinggame_infra PUBLIC "${TG_SRC}")

target_link_libraries(typinggame_infra
    PUBLIC
        Qt::Core
        Qt::Network
        Qt::Multimedia
)

target_compile_features(typinggame_infra PUBLIC cxx_std_20)

# Space War (all sources under src/spacewar/)
set(SW_SRC "${TG_SRC}/spacewar")
qt_add_library(spacewar_embed STATIC
    "${SW_SRC}/controller/spacewarcontroller.cpp"
    "${SW_SRC}/controller/spacewarcontroller.h"
    "${SW_SRC}/spacewargame.cpp"
    "${SW_SRC}/spacewargame.h"
)

target_include_directories(spacewar_embed PUBLIC "${SW_SRC}")
target_link_libraries(spacewar_embed
    PUBLIC
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
        typinggame_infra
)
target_compile_features(spacewar_embed PUBLIC cxx_std_20)

qt_add_executable(TypingGame
    WIN32 MACOSX_BUNDLE
    "${TG_SRC}/main.cpp"
    "${TG_SRC}/mainwindow.cpp"
    "${TG_SRC}/mainwindow.h"
    "${TG_SRC}/mainwindow.ui"
    "${TG_SRC}/applegame.h"
    "${TG_SRC}/applegame.cpp"
)

target_link_libraries(TypingGame
    PRIVATE
        typinggame_logic
        spacewar_embed
        typinggame_infra
        Qt::Widgets
        Qt::Multimedia
)

target_include_directories(TypingGame PRIVATE "${TG_SRC}")

target_compile_features(TypingGame PRIVATE cxx_std_20)

qt6_add_translations(TypingGame
    TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/mui/TypingGame_zh_CN.ts"
)

if (WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}" REQUIRED)
    add_custom_command(TARGET TypingGame POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                $<$<CONFIG:Debug>:--debug>
                $<$<NOT:$<CONFIG:Debug>>:--release>
                --no-compiler-runtime
                "$<TARGET_FILE:TypingGame>"
        COMMENT "Running windeployqt for TypingGame"
    )
endif()

add_custom_command(TARGET TypingGame POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/res"
            "$<TARGET_FILE_DIR:TypingGame>/assets"
    COMMENT "Copy bundled res to assets next to TypingGame executable"
)

install(TARGETS TypingGame
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET TypingGame
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# ---- Qt Test (logic / controller) ----
enable_testing()

qt_add_executable(TypingGameLogicTests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_gamecontroller.cpp"
)

target_link_libraries(TypingGameLogicTests
    PRIVATE
        typinggame_logic
        Qt::Core
        Qt::Test
)

target_include_directories(TypingGameLogicTests PRIVATE "${TG_SRC}")

target_compile_features(TypingGameLogicTests PRIVATE cxx_std_20)

# Visual Studio 等多配置生成器：主程序在 <build>/Release|Debug/；单测在同级 tests/ 子目录，避免与同目录下 TypingGame.exe 混淆。
set_target_properties(TypingGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)
set_target_properties(TypingGameLogicTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/tests"
)

if (WIN32)
    add_custom_command(TARGET TypingGameLogicTests POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                $<$<CONFIG:Debug>:--debug>
                $<$<NOT:$<CONFIG:Debug>>:--release>
                --no-compiler-runtime
                "$<TARGET_FILE:TypingGameLogicTests>"
        COMMENT "Running windeployqt for TypingGameLogicTests"
    )
endif()

add_test(NAME TypingGameLogicTests COMMAND TypingGameLogicTests)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    message(STATUS "构建完成后，主程序路径示例: ${CMAKE_BINARY_DIR}/Release/TypingGame.exe 或 ${CMAKE_BINARY_DIR}/Debug/TypingGame.exe")
elseif(CMAKE_GENERATOR MATCHES "Ninja")
    message(STATUS "Ninja 单配置: 可执行文件通常在 ${CMAKE_BINARY_DIR}/ （由 CMAKE_BUILD_TYPE 决定，无 Release/Debug 子目录）")
endif()
