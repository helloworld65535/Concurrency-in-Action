#为项目设置cmake的最低要求版本。
cmake_minimum_required(VERSION 3.20)
#设置项目的名称。
project(Chapter2)
#使用指定的源文件将可执行文件添加到项目中。
add_executable(Chapter2
    main.cpp
    run_time.hpp
    code_lists.hpp
)