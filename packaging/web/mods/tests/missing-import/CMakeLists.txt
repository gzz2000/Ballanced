cmake_minimum_required(VERSION 3.20)
project(MissingImportProbe LANGUAGES CXX)
include("${BALLANCE_WEB_SDK}/BallanceWebMod.cmake")
ballance_web_mod(MissingImportProbe ID MissingImportProbe VERSION 0.1.0 SOURCES MissingImportProbe.cpp)
