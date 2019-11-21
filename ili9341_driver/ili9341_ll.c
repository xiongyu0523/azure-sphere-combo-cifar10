#if defined(AzureSphere_CA7)

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "applibs_versions.h"

#include <applibs/log.h>
#include <applibs/i2c.h>
#include <applibs/spi.h>
#include <applibs/gpio.h>
#include <hw/sample_hardware.h>

static int RstGpioFd;
static int DcGpioFd;
static int BlGpioFd;
static int i2cFd;
static int spiFd;

#define MAX_SPI_TRANSFER_BYTES	4096

#elif defined(AzureSphere_CM4)

#include <stdlib.h>

#include "SPIMaster.h"
#include "GPIO.h"
#include "Log_Debug.h"

#define MT3620_RDB_HEADER3_ISU3_SPI		MT3620_UNIT_ISU3
#define MT3620_RDB_HEADER1_PIN4_GPIO	0
#define MT3620_RDB_HEADER1_PIN6_GPIO	1
#define MT3620_RDB_HEADER1_PIN8_GPIO	2

static SpiMaster* SpiHandler;
#define MAX_SPI_TRANSFER_BYTES	16

#endif

#include "ili9341_ll.h"

void ili9341_ll_reset_low(void)
{
#if defined(AzureSphere_CA7)

	GPIO_SetValue(RstGpioFd, GPIO_Value_Low);

#elif defined(AzureSphere_CM4)

	(void)GPIO_Write(MT3620_RDB_HEADER1_PIN4_GPIO, 0);

#endif
}
void ili9341_ll_reset_high(void)
{
#if defined(AzureSphere_CA7)

	GPIO_SetValue(RstGpioFd, GPIO_Value_High);

#elif defined(AzureSphere_CM4)

	(void)GPIO_Write(MT3620_RDB_HEADER1_PIN4_GPIO, 1);

#endif
}

void ili9341_ll_dc_low(void)
{
#if defined(AzureSphere_CA7)

	GPIO_SetValue(DcGpioFd, GPIO_Value_Low);

#elif defined(AzureSphere_CM4)

	(void)GPIO_Write(MT3620_RDB_HEADER1_PIN6_GPIO, 0);

#endif
}

void ili9341_ll_dc_high(void)
{
#if defined(AzureSphere_CA7)

	GPIO_SetValue(DcGpioFd, GPIO_Value_High);

#elif defined(AzureSphere_CM4)

	(void)GPIO_Write(MT3620_RDB_HEADER1_PIN6_GPIO, 1);

#endif
}

void ili9341_ll_backlight_off(void)
{
#if defined(AzureSphere_CA7)

	GPIO_SetValue(BlGpioFd, GPIO_Value_Low);

#elif defined(AzureSphere_CM4)

	(void)GPIO_Write(MT3620_RDB_HEADER1_PIN8_GPIO, 0);

#endif
}

void ili9341_ll_backlight_on(void)
{
#if defined(AzureSphere_CA7)

	GPIO_SetValue(BlGpioFd, GPIO_Value_High);

#elif defined(AzureSphere_CM4)

	(void)GPIO_Write(MT3620_RDB_HEADER1_PIN8_GPIO, 1);

#endif
}

int ili9341_ll_init(void)
{
#if defined(AzureSphere_CA7)

	RstGpioFd = GPIO_OpenAsOutput(ILI9341_RST, GPIO_OutputMode_PushPull, GPIO_Value_High);
	if (RstGpioFd < 0) {
		Log_Debug("ERROR: GPIO_OpenAsOutput: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	DcGpioFd = GPIO_OpenAsOutput(ILI9341_DC, GPIO_OutputMode_PushPull, GPIO_Value_High);
	if (DcGpioFd < 0) {
		Log_Debug("ERROR: GPIO_OpenAsOutput: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	BlGpioFd = GPIO_OpenAsOutput(ILI9341_BL, GPIO_OutputMode_PushPull, GPIO_Value_Low);
	if (BlGpioFd < 0) {
		Log_Debug("ERROR: GPIO_OpenAsOutput: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	SPIMaster_Config config;
	int ret = SPIMaster_InitConfig(&config);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_InitConfig: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
	spiFd = SPIMaster_Open(ILI9341_SPI, MT3620_SPI_CS_A, &config);
	if (spiFd < 0) {
		Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	int result = SPIMaster_SetBusSpeed(spiFd, 40000000);
	if (result < 0) {
		Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\r\n", errno, strerror(errno));
		close(spiFd);
		return -1;
	}

	result = SPIMaster_SetMode(spiFd, SPI_Mode_0);
	if (result < 0) {
		Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\r\n", errno, strerror(errno));
		close(spiFd);
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = GPIO_ConfigurePinForOutput(MT3620_RDB_HEADER1_PIN4_GPIO);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: GPIO_ConfigurePinForOutput: %d\r\n", ret);
		return -1;
	}
	GPIO_Write(MT3620_RDB_HEADER1_PIN4_GPIO, 1);

	ret = GPIO_ConfigurePinForOutput(MT3620_RDB_HEADER1_PIN6_GPIO);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: GPIO_ConfigurePinForOutput: %d\r\n", ret);
		return -1;
	}
	GPIO_Write(MT3620_RDB_HEADER1_PIN6_GPIO, 1);

	ret = GPIO_ConfigurePinForOutput(MT3620_RDB_HEADER1_PIN8_GPIO);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: GPIO_ConfigurePinForOutput: %d\r\n", ret);
		return -1;
	}
	GPIO_Write(MT3620_RDB_HEADER1_PIN8_GPIO, 0);

	SpiHandler = SpiMaster_Open(MT3620_RDB_HEADER3_ISU3_SPI);
	SpiMaster_ConfigDMA(SpiHandler, false);
	SpiMaster_Select(SpiHandler, CS_LINE0);
	SpiMaster_Configure(SpiHandler, false, false, 10000000);

	return 0;

#endif
}

int ili9341_ll_spi_tx_u8(uint8_t data)
{
#if defined(AzureSphere_CA7)

	SPIMaster_Transfer transfers;

	int ret = SPIMaster_InitTransfers(&transfers, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_InitTransfers: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	transfers.flags = SPI_TransferFlags_Write;
	transfers.writeData = &data;
	transfers.length = 1;

	ret = SPIMaster_TransferSequential(spiFd, &transfers, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_TransferSequential: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = SpiMaster_WriteSync(SpiHandler, &data, 1);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: SpiMaster_WriteSync: %d\r\n", ret);
		return -1;
	}

	return 0;

#endif
}

int ili9341_ll_spi_rx_u8(uint8_t *data)
{
#if defined(AzureSphere_CA7)

	SPIMaster_Transfer transfers;

	int ret = SPIMaster_InitTransfers(&transfers, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_InitTransfers: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	transfers.flags = SPI_TransferFlags_Read;
	transfers.readData = data;
	transfers.length = 1;

	ret = SPIMaster_TransferSequential(spiFd, &transfers, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_TransferSequential: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = SpiMaster_ReadSync(SpiHandler, &data, 1);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: SpiMaster_ReadSync: %d\r\n", ret);
		return -1;
	}

	return 0;

#endif
}

int ili9341_ll_spi_tx_u16(uint16_t data)
{
	uint8_t buf[2];
	buf[0] = data >> 8;
	buf[1] = data;

#if defined(AzureSphere_CA7)

	SPIMaster_Transfer transfers;

	int ret = SPIMaster_InitTransfers(&transfers, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_InitTransfers: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	transfers.flags = SPI_TransferFlags_Write;
	transfers.writeData = &buf[0];
	transfers.length = 2;

	ret = SPIMaster_TransferSequential(spiFd, &transfers, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_TransferSequential: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	return 0;

#elif defined(AzureSphere_CM4)

	int32_t ret = SpiMaster_WriteSync(SpiHandler, &buf, 2);
	if (ret != ERROR_NONE) {
		Log_Debug("ERROR: SpiMaster_WriteSync: %d\r\n", ret);
		return -1;
	}

	return 0;

#endif
}

int ili9341_ll_spi_tx(uint8_t *p_data, uint32_t tx_len)
{
#if defined(AzureSphere_CA7)

	uint32_t numOfXfer = tx_len / MAX_SPI_TRANSFER_BYTES;
	uint32_t lastXferSize = tx_len % MAX_SPI_TRANSFER_BYTES;

	SPIMaster_Transfer transfer;
	int ret = SPIMaster_InitTransfers(&transfer, 1);
	if (ret < 0) {
		Log_Debug("ERROR: SPIMaster_InitTransfers: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	transfer.flags = SPI_TransferFlags_Write;
	transfer.length = MAX_SPI_TRANSFER_BYTES;

	for (uint32_t i = 0; i < numOfXfer; i++) {

		transfer.writeData = p_data;

		ret = SPIMaster_TransferSequential(spiFd, &transfer, 1);
		if (ret < 0) {
			Log_Debug("ERROR: SPIMaster_TransferSequential: errno=%d (%s)\r\n", errno, strerror(errno));
			return -1;
		} else if (ret != transfer.length) {
			Log_Debug("ERROR: SPIMaster_TransferSequential transfer %d bytes, expect %d bytes\r\n", ret, transfer.length);
			return -1;
		}

		p_data += MAX_SPI_TRANSFER_BYTES;
	}

	if (lastXferSize > 0) {
		transfer.writeData = p_data;
		transfer.length = lastXferSize;

		ret = SPIMaster_TransferSequential(spiFd, &transfer, 1);
		if (ret < 0) {
			Log_Debug("ERROR: SPIMaster_TransferSequential: errno=%d (%s)\r\n", errno, strerror(errno));
			return -1;
		}
		else if (ret != transfer.length) {
			Log_Debug("ERROR: SPIMaster_TransferSequential transfer %d bytes, expect %d bytes\r\n", ret, transfer.length);
			return -1;
		}
	}

	return 0;

#elif defined(AzureSphere_CM4)

#endif
}

