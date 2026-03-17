#-------------------------------------------------------------------------------
# Zephyr Example Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE
    src/main.c
    src/display.c
    src/game.c
    src/input.c
    src/util.c
)

target_sources_ifdef(CONFIG_INPUT_BACKEND_TILT app PRIVATE src/input_tilt.c)
