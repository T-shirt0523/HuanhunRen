# ============================================================
# 还魂人 - CMake + Ninja 构建（MinGW-w64 G++ / raylib 5.5）
# 用法：
#   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
#         -DCMAKE_C_COMPILER=D:/codetool/mingw64/bin/gcc.exe ^
#         -DCMAKE_CXX_COMPILER=D:/codetool/mingw64/bin/g++.exe ^
#         -DCMAKE_MAKE_PROGRAM=D:/codetool/ninja/ninja.exe
#   cmake --build build
# 产物：build/game.exe
# ============================================================
cmake_minimum_required(VERSION 3.16)
project(HuanhunRen CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(RAYLIB_DIR "D:/codetool/raylib-5.5_win64_mingw-w64" CACHE PATH "raylib 5.5 根目录（含 include/ 与 lib/）")

add_executable(game
    main.cpp
    world.cpp
    creature.cpp
    assets.cpp
    audio.cpp
    water.cpp
    progress.cpp
    net.cpp
    l10n.cpp
)

target_include_directories(game PRIVATE "${RAYLIB_DIR}/include")
target_link_directories(game PRIVATE "${RAYLIB_DIR}/lib")
# 使用动态导入库（官方 w64 静态库存在 __imp__wassert 符号问题）
target_link_libraries(game raylibdll opengl32 gdi32 winmm ws2_32)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(game PRIVATE -O2 -Wall)
endif()

# 构建后复制 raylib.dll 到输出目录
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RAYLIB_DIR}/lib/raylib.dll" "$<TARGET_FILE_DIR:game>"
)

# 输出到项目根目录
set_target_properties(game PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
