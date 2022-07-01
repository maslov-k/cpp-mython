cmake_minimum_required(VERSION 3.5)

project(MythonInterpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MythonInterpreter lexer.h lexer.cpp lexer_test_open.cpp
                                 runtime.h runtime.cpp runtime_test.cpp
                                 statement.h statement.cpp statement_test.cpp
                                 parse.h parse.cpp parse_test.cpp
                                 main.cpp test_runner_p.h)
