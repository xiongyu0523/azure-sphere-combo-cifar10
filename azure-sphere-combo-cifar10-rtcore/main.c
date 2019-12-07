#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "Log_Debug.h"
#include "delay.h"

#include "mt3620-baremetal.h"
#include "mt3620-intercore.h"

#include "nn.h"

#define CFIAR10_WIDTH	32
#define CFIAR10_HEIGHT  32
#define CFIAR10_DEPTH   3

static uint8_t Cifar10ImgBuf[CFIAR10_WIDTH * CFIAR10_HEIGHT * CFIAR10_DEPTH];
static const char* cifar10_label[] = {"Plane", "Car", "Bird", "Cat", "Deer", "Dog", "Frog", "Horse", "Ship", "Truck" };

static const uintptr_t IO_CM4_RGU = 0x2101000C;

extern uint32_t StackTop; // &StackTop == end of TCM
static _Noreturn void DefaultExceptionHandler(void);
static _Noreturn void RTCoreMain(void);

// ARM DDI0403E.d SB1.5.2-3
// From SB1.5.3, "The Vector table must be naturally aligned to a power of two whose alignment
// value is greater than or equal to (Number of Exceptions supported x 4), with a minimum alignment
// of 128 bytes.". The array is aligned in linker.ld, using the dedicated section ".vector_table".

// The exception vector table contains a stack pointer, 15 exception handlers, and an entry for
// each interrupt.
#define INTERRUPT_COUNT 100 // from datasheet
#define EXCEPTION_COUNT (16 + INTERRUPT_COUNT)
#define INT_TO_EXC(i_) (16 + (i_))
const uintptr_t ExceptionVectorTable[EXCEPTION_COUNT] __attribute__((section(".vector_table")))
__attribute__((used)) = {
	[0] = (uintptr_t)&StackTop,                // Main Stack Pointer (MSP)
	[1] = (uintptr_t)RTCoreMain,               // Reset
	[2] = (uintptr_t)DefaultExceptionHandler,  // NMI
	[3] = (uintptr_t)DefaultExceptionHandler,  // HardFault
	[4] = (uintptr_t)DefaultExceptionHandler,  // MPU Fault
	[5] = (uintptr_t)DefaultExceptionHandler,  // Bus Fault
	[6] = (uintptr_t)DefaultExceptionHandler,  // Usage Fault
	[11] = (uintptr_t)DefaultExceptionHandler, // SVCall
	[12] = (uintptr_t)DefaultExceptionHandler, // Debug monitor
	[14] = (uintptr_t)DefaultExceptionHandler, // PendSV
	[15] = (uintptr_t)DefaultExceptionHandler, // SysTick

	[INT_TO_EXC(0)... INT_TO_EXC(INTERRUPT_COUNT - 1)] = (uintptr_t)DefaultExceptionHandler };

static _Noreturn void DefaultExceptionHandler(void)
{
	for (;;) {
		// empty.
	}
}

static uint8_t _get_top_prediction(q7_t* predictions, uint8_t max)
{
	uint8_t index = 0;
	int8_t  max_val = -128;
	for (uint8_t i = 0; i < max; i++) {
		if (max_val < predictions[i]) {
			max_val = predictions[i];
			index = i;
		}
	}
	return index;
}

_Noreturn void RTCoreMain(void)
{
	// Boost M4 core to 197.6MHz (@26MHz), refer to chapter 3.3 in MT3620 Datasheet
	uint32_t val = ReadReg32(IO_CM4_RGU, 0);
	val &= 0xFFFF00FF;
	val |= 0x00000200;
	WriteReg32(IO_CM4_RGU, 0, val);
	
	DebugUARTInit();
	Log_Debug("Exmaple to run NN inference on RTcore for cifar-10 dataset\r\n");

	BufferHeader *outbound, *inbound;
	uint32_t sharedBufSize = 0;

	if (GetIntercoreBuffers(&outbound, &inbound, &sharedBufSize) == -1) {
		Log_Debug("ERROR: GetIntercoreBuffers failed\r\n");
		while (1);
	}

	const size_t payloadStart = 20;
	const uint32_t maxInterCoreBufSize = 1024;
	uint32_t piece = 0;
	uint32_t recvSize = maxInterCoreBufSize + payloadStart;
	uint8_t recvBuf[maxInterCoreBufSize + payloadStart];
	q7_t output_data[10];
	uint8_t top_index;

	while (1) {

		// waiting for incoming data
		if (DequeueData(outbound, inbound, sharedBufSize, &recvBuf[0], &recvSize) == -1) {
			continue;
		}

		// 3072 = 1024 x 3, as the maximum allowed user payload is 1024, we need split into 3 buffer. (HL and RT core must be synced)
		if (piece < 3) {
			memcpy(&Cifar10ImgBuf[piece * maxInterCoreBufSize], &recvBuf[payloadStart], maxInterCoreBufSize);
			piece++;
		}

		if (piece == 3) {
			piece = 0;

			// input = 32x32x3 RGB data, output = Possibility of each class
			run_nn(&Cifar10ImgBuf[0], output_data);
			top_index = _get_top_prediction(output_data, 10);
			Log_Debug("%s\r\n", cifar10_label[top_index]);

			// Send the result back to HL core
			recvBuf[payloadStart] = top_index;
			EnqueueData(inbound, outbound, sharedBufSize, &recvBuf[0], payloadStart + 1);
		}
	}
}
