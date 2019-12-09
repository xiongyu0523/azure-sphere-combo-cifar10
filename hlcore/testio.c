#if defined(AzureSphere_CA7)

#include <applibs/gpio.h>
#include <hw/sample_hardware.h>
#include <errno.h>
#include <string.h>
#include <applibs/log.h>

#elif defined(AzureSphere_CM4)

// removed

#endif

#include "testio.h"

#define MAX_TESTIO 4
static int testIoFd[MAX_TESTIO] = { 0xFF, 0xFF, 0xFF, 0xFF };

void testio_init(uint8_t bit_mask)
{
#if defined(AzureSphere_CA7)

	if (bit_mask & TESTIO_0_MASK) {
		testIoFd[0] = GPIO_OpenAsOutput(TESTIO_0, GPIO_OutputMode_PushPull, GPIO_Value_High);
		if (testIoFd[0] < 0) {
			Log_Debug("ERROR: GPIO_OpenAsOutput: errno=%d (%s)\n", errno, strerror(errno));
		}
	}

	if (bit_mask & TESTIO_1_MASK) {
		testIoFd[1] = GPIO_OpenAsOutput(TESTIO_1, GPIO_OutputMode_PushPull, GPIO_Value_High);
		if (testIoFd[1] < 0) {
			Log_Debug("ERROR: GPIO_OpenAsOutput: errno=%d (%s)\n", errno, strerror(errno));
		}
	}

	if (bit_mask & TESTIO_2_MASK) {
		testIoFd[2] = GPIO_OpenAsOutput(TESTIO_2, GPIO_OutputMode_PushPull, GPIO_Value_High);
		if (testIoFd[2] < 0) {
			Log_Debug("ERROR: GPIO_OpenAsOutput: errno=%d (%s)\n", errno, strerror(errno));
		}
	}

	if (bit_mask & TESTIO_3_MASK) {
		testIoFd[3] = GPIO_OpenAsOutput(TESTIO_3, GPIO_OutputMode_PushPull, GPIO_Value_High);
		if (testIoFd[3] < 0) {
			Log_Debug("ERROR: GPIO_OpenAsOutput: errno=%d (%s)\n", errno, strerror(errno));
		}
	}
#elif defined(AzureSphere_CM4)

	// removed

#endif
}

void testio_set_high(uint8_t index)
{
	if ((index < MAX_TESTIO) && (testIoFd[index] != 0xFF)) {
#if defined(AzureSphere_CA7)
		GPIO_SetValue(testIoFd[index], GPIO_Value_High);
#elif defined(AzureSphere_CM4)
		// removed
#endif
	}
}

void testio_set_low(uint8_t index)
{
	if ((index < MAX_TESTIO) && (testIoFd[index] != 0xFF)) {
#if defined(AzureSphere_CA7)
		GPIO_SetValue(testIoFd[index], GPIO_Value_Low);
#elif defined(AzureSphere_CM4)
		// removed
#endif
	}
}