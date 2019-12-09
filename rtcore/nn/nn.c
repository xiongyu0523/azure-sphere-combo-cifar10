#include "nn.h"

static uint8_t mean[DATA_OUT_CH*DATA_OUT_DIM*DATA_OUT_DIM] = MEAN_DATA;

static q7_t conv1_wt[CONV1_IN_CH*CONV1_KER_DIM*CONV1_KER_DIM*CONV1_OUT_CH] = CONV1_WT;
static q7_t conv1_bias[CONV1_OUT_CH] = CONV1_BIAS;

static q7_t conv2_wt[CONV2_IN_CH*CONV2_KER_DIM*CONV2_KER_DIM*CONV2_OUT_CH] = CONV2_WT;
static q7_t conv2_bias[CONV2_OUT_CH] = CONV2_BIAS;

static q7_t conv3_wt[CONV3_IN_CH*CONV3_KER_DIM*CONV3_KER_DIM*CONV3_OUT_CH] = CONV3_WT;
static q7_t conv3_bias[CONV3_OUT_CH] = CONV3_BIAS;

static q7_t ip1_wt[IP1_IN_DIM*IP1_OUT_DIM] = IP1_WT;
static q7_t ip1_bias[IP1_OUT_DIM] = IP1_BIAS;

//Add input_data and output_data in top main.cpp file
//uint8_t input_data[DATA_OUT_CH*DATA_OUT_DIM*DATA_OUT_DIM];
//q7_t output_data[IP1_OUT_DIM];

q7_t col_buffer[3200];
q7_t scratch_buffer[40960];

void mean_subtract(q7_t* image_data) {
  for(int i=0; i<DATA_OUT_CH*DATA_OUT_DIM*DATA_OUT_DIM; i++) {
    image_data[i] = (q7_t)__SSAT( ((int)((uint8_t)image_data[i] - mean[i]) >> DATA_RSHIFT), 8);
  }
}

void run_nn(q7_t* input_data, q7_t* output_data) {
  q7_t* buffer1 = scratch_buffer;
  q7_t* buffer2 = buffer1 + 32768;
  mean_subtract(input_data);
  arm_convolve_HWC_q7_RGB(input_data, CONV1_IN_DIM, CONV1_IN_CH, conv1_wt, CONV1_OUT_CH, CONV1_KER_DIM, CONV1_PAD, CONV1_STRIDE, conv1_bias, CONV1_BIAS_LSHIFT, CONV1_OUT_RSHIFT, buffer1, CONV1_OUT_DIM, (q15_t*)col_buffer, NULL);
  arm_maxpool_q7_HWC(buffer1, POOL1_IN_DIM, POOL1_IN_CH, POOL1_KER_DIM, POOL1_PAD, POOL1_STRIDE, POOL1_OUT_DIM, col_buffer, buffer2);
  arm_relu_q7(buffer2, RELU1_OUT_DIM*RELU1_OUT_DIM*RELU1_OUT_CH);
  arm_convolve_HWC_q7_fast(buffer2, CONV2_IN_DIM, CONV2_IN_CH, conv2_wt, CONV2_OUT_CH, CONV2_KER_DIM, CONV2_PAD, CONV2_STRIDE, conv2_bias, CONV2_BIAS_LSHIFT, CONV2_OUT_RSHIFT, buffer1, CONV2_OUT_DIM, (q15_t*)col_buffer, NULL);
  arm_relu_q7(buffer1, RELU2_OUT_DIM*RELU2_OUT_DIM*RELU2_OUT_CH);
  arm_avepool_q7_HWC(buffer1, POOL2_IN_DIM, POOL2_IN_CH, POOL2_KER_DIM, POOL2_PAD, POOL2_STRIDE, POOL2_OUT_DIM, col_buffer, buffer2);
  arm_convolve_HWC_q7_fast(buffer2, CONV3_IN_DIM, CONV3_IN_CH, conv3_wt, CONV3_OUT_CH, CONV3_KER_DIM, CONV3_PAD, CONV3_STRIDE, conv3_bias, CONV3_BIAS_LSHIFT, CONV3_OUT_RSHIFT, buffer1, CONV3_OUT_DIM, (q15_t*)col_buffer, NULL);
  arm_relu_q7(buffer1, RELU3_OUT_DIM*RELU3_OUT_DIM*RELU3_OUT_CH);
  arm_avepool_q7_HWC(buffer1, POOL3_IN_DIM, POOL3_IN_CH, POOL3_KER_DIM, POOL3_PAD, POOL3_STRIDE, POOL3_OUT_DIM, col_buffer, buffer2);
  arm_fully_connected_q7_opt(buffer2, ip1_wt, IP1_IN_DIM, IP1_OUT_DIM, IP1_BIAS_LSHIFT, IP1_OUT_RSHIFT, ip1_bias, output_data, (q15_t*)col_buffer);
  arm_softmax_q7(output_data, IP1_OUT_DIM, output_data);
}
