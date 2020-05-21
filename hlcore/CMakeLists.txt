#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)
project (azure-sphere-combo-cifar10-hlcore C)

azsphere_configure_tools(TOOLS_REVISION "20.04")
azsphere_configure_api(TARGET_API_SET "5+Beta2004")

# include
include_directories(${CMAKE_SOURCE_DIR} 
					${CMAKE_SOURCE_DIR}/arducam_driver 
					${CMAKE_SOURCE_DIR}/ili9341_driver)

# macro
add_compile_definitions(AzureSphere_CA7)
# Create executable
add_executable(${PROJECT_NAME} main.c delay.c epoll_timerfd_utilities.c testio.c tjpgd.c
								arducam_driver/ArduCAM.c arducam_driver/ll.c arducam_driver/ov2640_regs.c
								ili9341_driver/ili9341.c ili9341_driver/ili9341_ll.c ili9341_driver/font.c ili9341_driver/text.c)
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "Hardware/mt3620_rdb" TARGET_DEFINITION "sample_hardware.json")

azsphere_target_add_image_package(${PROJECT_NAME})
