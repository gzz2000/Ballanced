cmake_minimum_required(VERSION 3.20)
project(EnergyItemCounter LANGUAGES CXX)
include("${BALLANCE_WEB_SDK}/BallanceWebMod.cmake")
ballance_web_mod(EnergyItemCounter ID EnergyItemCounter VERSION 0.1.1-web.1 SOURCES main.cpp)
