#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <applibs/log.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <applibs/application.h>


#include "epoll_timerfd_utilities.h"
#include "ArduCAM.h"
#include "ili9341.h"
#include "text.h"
#include "delay.h"

static void SocketEventHandler(EventData* eventData);

static const char rtAppComponentId[] = "6583cf17-d321-4d72-8283-0b7c5b56442b";

#define DISPLAY_WIDTH	160
#define DISPLAY_HEIGHT	160
#define DISPLAY_DEPTH	2

#define CFIAR10_WIDTH	32
#define CFIAR10_HEIGHT  32
#define CFIAR10_DEPTH   3

static uint8_t s_CameraBuffer[320 * 240 * 2 + 8]; // QVGA from Camera 
static uint8_t s_DisplayBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_DEPTH];       // 128x128 Centralized display on LCD
static uint8_t s_Cifar10ResizeBuffer[CFIAR10_WIDTH * CFIAR10_HEIGHT * CFIAR10_DEPTH]; // stanard cifar-10 format

static int epollFd = 0;
static int rtSocketFd = 0;
static EventData socketEventData = { .eventHandler = &SocketEventHandler };

static void SocketEventHandler(EventData* eventData)
{
	const char* cifar10_label[] = { "Plane", "Car  ", "Bird  ", "Cat  ", "Deer  ", "Dog  ", "Frog  ", "Horse", "Ship ", "Truck" };

	uint8_t index;
	ssize_t bytesReceived = recv(rtSocketFd, &index, sizeof(index), 0);
	if (bytesReceived < 0) {
		Log_Debug("ERROR: Unable to receive message: %d (%s)\r\n", errno, strerror(errno));
		return;
	}

	lcd_set_text_cursor(5, 30);
	lcd_display_string(cifar10_label[index]);
}

void resize(uint8_t *p_in, uint8_t *p_out)
{
	int resize_ratio = 5 * 2; // 128/32 = 4 x 2 bytes per pixel

	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 32; x++) {

			int orig_img_loc = y * 160 * resize_ratio + x * resize_ratio;
			int out_img_loc = (y * 32 + x) * 3;

			uint8_t pix_hi = p_in[orig_img_loc];
			uint8_t pix_lo = p_in[orig_img_loc + 1];

			p_out[out_img_loc] = (0xF8 & pix_hi);
			p_out[out_img_loc + 1] = ((0x07 & pix_hi) << 5) | ((0xE0 & pix_lo) >> 3);
			p_out[out_img_loc + 2] = (0x1F & pix_lo) << 3;
		}
	}
}

static void* epoll_thread(void* ptr)
{
	Log_Debug("epoll_thread start\r\n");

	epollFd = CreateEpollFd();
	if (epollFd < 0) {
		return -1;
	}

	struct timeval to = { .tv_sec = 1, .tv_usec = 0 };
	int ret = setsockopt(rtSocketFd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));
	if (ret == -1) {
		Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
		return -1;
	}

	if (RegisterEventHandlerToEpoll(epollFd, rtSocketFd, &socketEventData, EPOLLIN) != 0) {
		return -1;
	}

	while (1) {
		(void)WaitForEventAndCallHandler(epollFd);
	}
}


int main(int argc, char* argv[])
{
	pthread_t thread_id;

	Log_Debug("Exmaple to capture a JPEG image from ArduCAM mini 2MP Plus and send to Azure Blob\r\n");

	rtSocketFd = Application_Socket(rtAppComponentId);
	if (rtSocketFd == -1) {
		Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
		return -1;
	}

	if (pthread_create(&thread_id, NULL, epoll_thread, NULL)) {
		Log_Debug("ERROR: creating thread fail\r\n");
		return -1;
	}

	ili9341_init();

	lcd_set_text_size(2);
	lcd_set_text_cursor(5, 10);
	lcd_display_string("Cifar-10 Demo");

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
		resize(&s_DisplayBuffer[0], &s_Cifar10ResizeBuffer[0]);

		// max allowed size is 1KB for a single transfer
		const uint32_t maxInterCoreBufSize = 1024;
		for (uint32_t i = 0; i < sizeof(s_Cifar10ResizeBuffer) / maxInterCoreBufSize; i++) {
			ssize_t bytesSent = send(rtSocketFd, &s_Cifar10ResizeBuffer[i * maxInterCoreBufSize], maxInterCoreBufSize, 0);
			if (bytesSent < 0) {
				Log_Debug("ERROR: Unable to send message: %d (%s)\r\n", errno, strerror(errno));
			} else if (bytesSent != maxInterCoreBufSize) {
				Log_Debug("ERROR: Write %d bytes, expect %d bytes\r\n", bytesSent, maxInterCoreBufSize);
			}
		}
	}

	Log_Debug("App Exit\r\n");

	return 0;
}