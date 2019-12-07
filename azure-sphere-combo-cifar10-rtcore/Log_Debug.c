#include "mt3620-baremetal.h"
#include "Log_Debug.h"

static const uintptr_t IO_CM4_DEBUGUART = 0x21040000;

void DebugUARTInit(void)
{
	// Configure UART to use 115200-8-N-1.
	WriteReg32(IO_CM4_DEBUGUART, 0x0C, 0x80); // LCR (enable DLL, DLM)
	WriteReg32(IO_CM4_DEBUGUART, 0x24, 0x3);  // HIGHSPEED
	WriteReg32(IO_CM4_DEBUGUART, 0x04, 0);    // Divisor Latch (MS)
	WriteReg32(IO_CM4_DEBUGUART, 0x00, 1);    // Divisor Latch (LS)
	WriteReg32(IO_CM4_DEBUGUART, 0x28, 224);  // SAMPLE_COUNT
	WriteReg32(IO_CM4_DEBUGUART, 0x2C, 110);  // SAMPLE_POINT
	WriteReg32(IO_CM4_DEBUGUART, 0x58, 0);    // FRACDIV_M
	WriteReg32(IO_CM4_DEBUGUART, 0x54, 223);  // FRACDIV_L
	WriteReg32(IO_CM4_DEBUGUART, 0x0C, 0x03); // LCR (8-bit word length)
}

void _putchar(char character)
{
	while (!(ReadReg32(IO_CM4_DEBUGUART, 0x14) & (UINT32_C(1) << 5)));
	WriteReg32(IO_CM4_DEBUGUART, 0x0, character);
}
