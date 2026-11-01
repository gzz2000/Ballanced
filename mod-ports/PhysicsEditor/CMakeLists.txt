cmake_minimum_required(VERSION 3.20)
project(PhysicsEditorWasm LANGUAGES CXX)
include("${BALLANCE_WEB_SDK}/BallanceWebMod.cmake")
ballance_web_mod(PhysicsEditor ID PhysicsEditor VERSION 0.0.4-web.1 SOURCES main.cpp)
