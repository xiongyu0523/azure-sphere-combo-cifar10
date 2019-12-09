#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
PROJECT(azure-sphere-combo-cifar10-rtcore C)

# include
include_directories(${CMAKE_SOURCE_DIR} 
					${CMAKE_SOURCE_DIR}/printf 
					${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/lib/mt3620
					${CMAKE_SOURCE_DIR}/nn
					${CMAKE_SOURCE_DIR}/CMSIS/NN/Include ${CMAKE_SOURCE_DIR}/CMSIS/DSP/Include ${CMAKE_SOURCE_DIR}/CMSIS/Core/Include)

# macro
add_compile_definitions(AzureSphere_CM4)
add_compile_definitions(ARM_MATH_DSP)
#add_compile_definitions(ARM_MATH_LOOPUNROLL)
add_compile_definitions(__FPU_PRESENT=1U)

# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c delay.c mt3620-intercore.c Log_Debug.c printf/printf.c
							   nn/nn.c
							   CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q7.c CMSIS/NN/Source/ActivationFunctions/arm_nn_activations_q15.c CMSIS/NN/Source/ActivationFunctions/arm_relu_q7.c CMSIS/NN/Source/ActivationFunctions/arm_relu_q15.c CMSIS/NN/Source/ActivationFunctions/arm_relu6_s8.c
							   CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_add_s8.c CMSIS/NN/Source/BasicMathFunctions/arm_elementwise_mul_s8.c
							   CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_1x1_HWC_q7_fast_nonsquare.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_1x1_s8_fast.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_basic.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_basic_nonsquare.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_fast.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_fast_nonsquare.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q7_RGB.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q15_basic.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q15_fast.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_HWC_q15_fast_nonsquare.c CMSIS/NN/Source/ConvolutionFunctions/arm_convolve_s8.c CMSIS/NN/Source/ConvolutionFunctions/arm_depthwise_conv_s8.c CMSIS/NN/Source/ConvolutionFunctions/arm_depthwise_conv_s8_opt.c CMSIS/NN/Source/ConvolutionFunctions/arm_depthwise_conv_u8_basic_ver1.c CMSIS/NN/Source/ConvolutionFunctions/arm_depthwise_separable_conv_HWC_q7.c CMSIS/NN/Source/ConvolutionFunctions/arm_depthwise_separable_conv_HWC_q7_nonsquare.c CMSIS/NN/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_q7_q15.c CMSIS/NN/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_q7_q15_reordered.c CMSIS/NN/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_s8_s16.c CMSIS/NN/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_s8_s16_reordered.c
							   CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15.c CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_mat_q7_vec_q15_opt.c CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7.c CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q7_opt.c CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15.c CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_q15_opt.c CMSIS/NN/Source/FullyConnectedFunctions/arm_fully_connected_s8.c
							   CMSIS/NN/Source/NNSupportFunctions/arm_nn_accumulate_q7_to_q15.c CMSIS/NN/Source/NNSupportFunctions/arm_nn_add_q7.c CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q7.c CMSIS/NN/Source/NNSupportFunctions/arm_nn_mult_q15.c CMSIS/NN/Source/NNSupportFunctions/arm_nntables.c CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_no_shift.c CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_reordered_no_shift.c CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_reordered_with_offset.c CMSIS/NN/Source/NNSupportFunctions/arm_q7_to_q15_with_offset.c
							   CMSIS/NN/Source/PoolingFunctions/arm_avgpool_s8.c CMSIS/NN/Source/PoolingFunctions/arm_max_pool_s8.c CMSIS/NN/Source/PoolingFunctions/arm_max_pool_s8_opt.c CMSIS/NN/Source/PoolingFunctions/arm_pool_q7_HWC.c
							   CMSIS/NN/Source/SoftmaxFunctions/arm_softmax_q7.c CMSIS/NN/Source/SoftmaxFunctions/arm_softmax_q15.c CMSIS/NN/Source/SoftmaxFunctions/arm_softmax_with_batch_q7.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/CMSIS/DSP/Lib/GCC/libarm_cortexM4lf_math.a)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

# Add MakeImage post-build command
INCLUDE("${AZURE_SPHERE_MAKE_IMAGE_FILE}")
