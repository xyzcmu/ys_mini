cmake_minimum_required(VERSION 3.20)

project(TestCmake)

# === 开启 C++23 支持 ===
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)


set(OpenCV_DIR "C:/opencv/build/")
find_package(OpenCV REQUIRED)

# 打印找到的 OpenCV 版本和路径（方便调试）
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")


# 将所有生成的可执行文件（.exe）统一输出到项目根目录下的 bin 文件夹 ===
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_executable(mini main.cpp gameWindow.cpp gameInput.cpp)

# 添加头文件路径
target_include_directories(mini PRIVATE ${OpenCV_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)

target_link_directories(mini PRIVATE ${PROJECT_SOURCE_DIR}/lib)

# 链接 OpenCV 库文件
target_link_libraries(mini PRIVATE ${OpenCV_LIBS} myOcr)



# 程序是用 MinGW (GCC 15) 编译的，如果目标电脑没有安装 MinGW 运行环境
# 可能会报缺少 libstdc++-6.dll 的错误
# 把 GCC 标准库静态打包进 exe，做到真正的“绿色免安装”
# target_link_options(prog PRIVATE "-static-libgcc" "-static-libstdc++")

message(status: "win32=" ${WIN32})
message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Compiler Path: ${CMAKE_CXX_COMPILER}")




