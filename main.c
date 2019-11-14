#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <time.h>
#include <signal.h>
#include <applibs/networking.h>

#include "ArduCAM.h"
#include "ili9341.h"
#include "delay.h"

#define DISPLAY_WIDTH	128
#define DISPLAY_HEIGHT	128

static uint8_t s_CameraBuffer[320 * 240 * 2 + 8]; // QVGA from Camera 
static uint8_t s_DisplayBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT * 2];    // 128x128 Centralized display on LCD
static uint8_t s_Cifar10ResizeBuffer[32 * 32 * 3];// stanard cifar-10 format

void resize_for_cifar10(uint8_t *p_in, uint8_t *p_out)
{
	int resize_ratio = 4 * 2; // 128/32 = 4, 2 bytes per pixel

	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 32; x++) {

			int orig_img_loc = y * 128 * resize_ratio + x * resize_ratio;
			int out_img_loc = ((32 - 1 - y) * 32 + (32 - 1 - x)) * 3;
			uint8_t pix_hi = p_in[orig_img_loc];
			uint8_t pix_lo = p_in[orig_img_loc + 1];

			p_out[out_img_loc] = (0xF8 & pix_hi);
			p_out[out_img_loc + 1] = ((0x07 & pix_hi) << 5) | ((0xE0 & pix_lo) >> 3);
			p_out[out_img_loc + 2] = (0x1F & pix_lo) << 3;
#if 0
			int orig_img_loc = y * 128 * resize_ratio + x * resize_ratio;
			int out_img_loc = ((32 - 1 - y) * 32 + (32 - 1 - x)) * 2;
			uint8_t pix_hi = p_in[orig_img_loc];
			uint8_t pix_lo = p_in[orig_img_loc + 1];

			p_out[out_img_loc] = p_in[orig_img_loc];
			p_out[out_img_loc + 1] = p_in[orig_img_loc + 1];
#endif
		}
	}
}


int main(int argc, char* argv[])
{
	Log_Debug("Exmaple to capture a JPEG image from ArduCAM mini 2MP Plus and send to Azure Blob\r\n");

	ili9341_init();

	// init hardware and probe camera
	arducam_ll_init();
	arducam_reset();
	if (arducam_test() == 0) {
		Log_Debug("ArduCAM mini 2MP Plus is found\r\n");
	}
	else {
		Log_Debug("ArduCAM mini 2MP Plus is not found\r\n");
		return -1;
	}

	// config Camera

	arducam_set_format(BMP);
	arducam_InitCAM();
	delay_ms(1000);
	arducam_clear_fifo_flag();
	arducam_flush_fifo();

	uint16_t leftcorner_x = (320 - DISPLAY_WIDTH) / 2;
	uint16_t leftcorner_y = (240 - DISPLAY_HEIGHT) / 2;

	ili9341_draw_rect(leftcorner_x - 1, leftcorner_y - 1, DISPLAY_WIDTH + 2, DISPLAY_HEIGHT + 2, RED);

	while (1) {

		// Trigger a capture and wait for data ready in DRAM
		arducam_start_capture();
		while (!arducam_check_fifo_done());

		uint32_t img_len = arducam_read_fifo_length();
		if (img_len > MAX_FIFO_SIZE) {
			Log_Debug("ERROR: FIFO overflow\r\n");
			return -1;
		}

		arducam_CS_LOW();
		arducam_set_fifo_burst();
		arducam_read_fifo_burst(&s_CameraBuffer[0], img_len);
		arducam_CS_HIGH();

		arducam_clear_fifo_flag();

		uint32_t r_pos = 320 * 2 * leftcorner_y + leftcorner_x * 2;
		uint32_t w_pos = 0;
		
		for (uint8_t row = 0; row < DISPLAY_HEIGHT; row++) {
			memcpy(&s_DisplayBuffer[w_pos], &s_CameraBuffer[r_pos], DISPLAY_WIDTH * 2);

			w_pos += DISPLAY_WIDTH * 2;
			r_pos += 640;
		}

		ili9341_draw_bitmap(leftcorner_x, leftcorner_y, DISPLAY_WIDTH, DISPLAY_HEIGHT, &s_DisplayBuffer[0]);

		resize_for_cifar10(&s_DisplayBuffer[0], &s_Cifar10ResizeBuffer[0]);

		// pass to M4 core to do NN inference
	}

	Log_Debug("App Exit\r\n");

	return 0;
}