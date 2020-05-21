#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

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
#include "tjpgd.h"

//#define CFG_MODE_JPEG
#define CFG_MODE_BITMAP

#if (defined(CFG_MODE_JPEG) && defined(CFG_MODE_BITMAP)) || (!defined(CFG_MODE_JPEG) && !defined(CFG_MODE_BITMAP))
#error "define CFG_MODE_JPEG or CFG_MODE_BITMAP"
#endif

static void SocketEventHandler(EventData* eventData);

static const char rtAppComponentId[] = "6583cf17-d321-4d72-8283-0b7c5b56442b";

#define DISPLAY_WIDTH	160
#if defined(CFG_MODE_JPEG)
#define DISPLAY_HEIGHT	120
#elif defined(CFG_MODE_BITMAP)
#define DISPLAY_HEIGHT  160
#endif
#define DISPLAY_DEPTH	2

#define CFIAR10_WIDTH	32
#define CFIAR10_HEIGHT  32
#define CFIAR10_DEPTH   3

#if defined(CFG_MODE_JPEG)
static uint8_t s_CameraBuffer[4096]; // QQVGA JPG from Camera (usually 2.x KB)

#define TJPD_BUF_SIZE 3100
static uint8_t tJpgdBuffer[TJPD_BUF_SIZE];
static uint32_t readIndexOfJpg = 0;

#elif defined (CFG_MODE_BITMAP)
static uint8_t s_CameraBuffer[320 * 240 * 2 + 8]; // QVGA from Camera
#endif
static uint8_t s_DisplayBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_DEPTH];       // 128x128 Centralized display on LCD
static uint8_t s_Cifar10ResizeBuffer[CFIAR10_WIDTH * CFIAR10_HEIGHT * CFIAR10_DEPTH]; // stanard cifar-10 format

static int epollFd = 0;
static int rtSocketFd = 0;
static EventData socketEventData = { .eventHandler = &SocketEventHandler };
static sem_t rtCoreReadySem;

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

	if (sem_post(&rtCoreReadySem) < 0) {
		Log_Debug("ERROR: sem_post failed: %d (%s)\r\n", errno, strerror(errno));
	}
}

static void resize(uint8_t* p_in, uint8_t* p_out)
{
	// offset so that only the center part of rectangular image is selected for resizing
	int width_offset = ((DISPLAY_WIDTH - DISPLAY_HEIGHT) / 2) * DISPLAY_DEPTH;

	int yresize_ratio = (DISPLAY_WIDTH / CFIAR10_WIDTH) * DISPLAY_DEPTH;
	int xresize_ratio = (DISPLAY_HEIGHT / CFIAR10_HEIGHT) * DISPLAY_DEPTH;
	int resize_ratio = (xresize_ratio < yresize_ratio) ? xresize_ratio : yresize_ratio;

	for (int y = 0; y < CFIAR10_WIDTH; y++) {
		for (int x = 0; x < CFIAR10_HEIGHT; x++) {

			int orig_img_loc = (y * DISPLAY_WIDTH * resize_ratio + x * resize_ratio + width_offset);
			int out_img_loc = (y * CFIAR10_WIDTH + x) * CFIAR10_DEPTH;

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
#if defined(CFG_MODE_JPEG)
static uint16_t input_func(JDEC* jd, uint8_t* buff, uint16_t ndata)
{
	if (buff != NULL) {

		memcpy(buff, &s_CameraBuffer[readIndexOfJpg], ndata);
		readIndexOfJpg += ndata;

	} else {
		readIndexOfJpg += ndata;
	}

	return ndata;
}

static uint16_t out_func(JDEC* jd, void* bitmap, JRECT* rect)
{
	uint8_t *p_src, *p_dst;
	uint16_t y, bws, bwd;

	/* Copy the decompressed RGB rectanglar to the frame buffer (assuming RGB888 cfg) */
	p_src = (uint8_t*)bitmap;
	p_dst = s_DisplayBuffer + DISPLAY_DEPTH * (rect->top * DISPLAY_WIDTH + rect->left);		/* Left-top of destination rectangular */
	bws = DISPLAY_DEPTH * (rect->right - rect->left + 1);									/* Width of source rectangular [byte] */
	bwd = DISPLAY_DEPTH * DISPLAY_WIDTH;													/* Width of frame buffer [byte] */
	for (y = rect->top; y <= rect->bottom; y++) {
		memcpy(p_dst, p_src, bws);   /* Copy a line */
		p_src += bws; p_dst += bwd;  /* Next line */
	}

	return 1;    /* Continue to decompress */
}
#endif
int main(int argc, char* argv[])
{
	pthread_t thread_id;

	Log_Debug("Example to demo image recognition with ArduCAM 2MP camera and ILI9341 TFT\r\n");

	rtSocketFd = Application_Connect(rtAppComponentId);
	if (rtSocketFd == -1) {
		Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
		return -1;
	}

	sem_init(&rtCoreReadySem, 0, 1);

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
#if defined(CFG_MODE_JPEG)
	arducam_set_format(JPEG);
#elif defined (CFG_MODE_BITMAP)
	arducam_set_format(BMP);
#endif
	arducam_InitCAM();
#if defined(CFG_MODE_JPEG)
	arducam_OV2640_set_JPEG_size(OV2640_160x120);
#endif

	delay_ms(100);
	arducam_clear_fifo_flag();
	arducam_flush_fifo();

	// draw a red rect of display area
	uint16_t lc_x = (ILI9341_LCD_PIXEL_WIDTH - DISPLAY_WIDTH) / 2;
	uint16_t lc_y = (ILI9341_LCD_PIXEL_HEIGHT - DISPLAY_HEIGHT) / 2;
	if ((lc_x > 0) && (lc_y > 0)) {
		ili9341_draw_rect(lc_x - 1, lc_y - 1, DISPLAY_WIDTH + 2, DISPLAY_HEIGHT + 2, RED);
	}

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

#if defined(CFG_MODE_JPEG)
		JRESULT ret;
		JDEC jd;

		readIndexOfJpg = 0;
		ret = jd_prepare(&jd, input_func, &tJpgdBuffer[0], TJPD_BUF_SIZE, NULL);
		if (ret != JDR_OK) {
			Log_Debug("ERROR: jd_prepare failed, reason = %d\r\n", ret);
			return -1;
		}

		ret = jd_decomp(&jd, out_func, 0);
		if (ret != JDR_OK) {
			Log_Debug("ERROR: jd_decomp failed, reason = %d\r\n", ret);
			return -1;
		}
#endif

#if defined(CFG_MODE_BITMAP)
		uint32_t r_pos = 320 * 2 * lc_y + lc_x * 2;
		uint32_t w_pos = 0;

		for (uint8_t row = 0; row < DISPLAY_HEIGHT; row++) {
			memcpy(&s_DisplayBuffer[w_pos], &s_CameraBuffer[r_pos], DISPLAY_WIDTH * 2);

			w_pos += DISPLAY_WIDTH * 2;
			r_pos += 640;
		}
#elif defined(CFG_MODE_JPEG)
		uint8_t tmpt;
		for (uint32_t i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_DEPTH; i += 2) {
			tmpt = s_DisplayBuffer[i];
			s_DisplayBuffer[i] = s_DisplayBuffer[i + 1];
			s_DisplayBuffer[i + 1] = tmpt;
		}
#endif

		ili9341_draw_bitmap(lc_x, lc_y, DISPLAY_WIDTH, DISPLAY_HEIGHT, &s_DisplayBuffer[0]);
		resize(&s_DisplayBuffer[0], &s_Cifar10ResizeBuffer[0]);

		while (sem_wait(&rtCoreReadySem) == -1 && errno == EINTR);

		// max allowed size is 1KB for a single transfer, split 3072 into 3 message
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