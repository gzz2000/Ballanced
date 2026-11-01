cmake_minimum_required(VERSION 3.20)
project(KeystrokeViewer LANGUAGES CXX)
include("${BALLANCE_WEB_SDK}/BallanceWebMod.cmake")
ballance_web_mod(KeystrokeViewer ID KeystrokeViewer VERSION 0.1.0-web.1 SOURCES main.cpp)
