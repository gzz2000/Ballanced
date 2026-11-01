cmake_minimum_required(VERSION 3.20)
project(WebHello LANGUAGES CXX)
include("${BALLANCE_WEB_SDK}/BallanceWebMod.cmake")
ballance_web_mod(WebHello ID WebHello VERSION 1.0.0 SOURCES WebHello.cpp
    RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/message.txt=message.txt")
