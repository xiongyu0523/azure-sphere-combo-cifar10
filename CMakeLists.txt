#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.8)
project (azure-sphere-combo-cifar10-hlcore C)

# include
include_directories(${CMAKE_SOURCE_DIR} 
					${CMAKE_SOURCE_DIR}/arducam_driver 
					${CMAKE_SOURCE_DIR}/ili9341_driver)

# macro
add_compile_definitions(AzureSphere_CA7)
# Create executable
add_executable (${PROJECT_NAME} main.c delay.c epoll_timerfd_utilities.c
								arducam_driver/ArduCAM.c arducam_driver/ll.c arducam_driver/ov2640_regs.c
								ili9341_driver/ili9341.c ili9341_driver/ili9341_ll.c ili9341_driver/font.c ili9341_driver/text.c)
target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c)

# Add MakeImage post-build command
include (${AZURE_SPHERE_MAKE_IMAGE_FILE})
