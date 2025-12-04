cmake_minimum_required(VERSION 3.14)
project(sqlparser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boostライブラリの検索
# Boostライブラリの検索
# cmake .. -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBOOST_INCLUDEDIR="d:/vcpkg/installed/x64-windows/include" -DBOOST_LIBRARYDIR="d:/vcpkg/installed/x64-windows/lib"
# 手動インストールの場合、CMake実行時に -DBOOST_ROOT="D:/path/to/boost" を指定してください
find_package(Boost 1.82 REQUIRED)

# ヘッダーオンリーライブラリとして定義
add_library(sqlparser INTERFACE)
target_include_directories(sqlparser INTERFACE include)
target_link_libraries(sqlparser INTERFACE Boost::boost)

# サブディレクトリの追加
add_subdirectory(examples)
enable_testing()
add_subdirectory(tests)
