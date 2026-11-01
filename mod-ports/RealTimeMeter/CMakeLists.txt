cmake_minimum_required(VERSION 3.20)
project(RealTimeMeterWeb LANGUAGES CXX)
include("${BALLANCE_WEB_SDK}/BallanceWebMod.cmake")
ballance_web_mod(RealTimeMeter ID RealTimeMeter VERSION 0.1.2-web.1 SOURCES main.cpp)
