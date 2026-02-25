#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

#In Zephyr, the find_package(Zephyr ...) call initializes 
# Zephyr’s build and creates the app target for you. 
# You then add your sources with target_sources(app PRIVATE src/main.c).
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(blinky LANGUAGES C)

target_sources(app PRIVATE src/main.c)
