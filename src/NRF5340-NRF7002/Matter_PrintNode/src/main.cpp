/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "3D_Print_UART_Data.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(app, CONFIG_CHIP_APP_LOG_LEVEL);

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(nordic_nrf_uarte)

char checkPhrase1[] = "T:";
char checkPhrase2[] = "SD printing byte";
char checkPhrase3[] = "Not SD printing";

char tx_buf[64];

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, 64, 10, 4);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* receive buffer used in UART ISR callback */
static char rx_buf[64];
static int rx_buf_pos;

void serial_cb(const struct device *dev, void *user_data);
void print_uart(char *buf);

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

 	if (!uart_irq_rx_ready(uart_dev)) {
		return;
 	}

 	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
 		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
 			/* terminate string */
 			rx_buf[rx_buf_pos] = '\0';

 			/* if queue is full, message is silently dropped */
 			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
 			rx_buf_pos = 0;
 		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
 			rx_buf[rx_buf_pos++] = c;
 		}
 		/* else: characters beyond buffer size are dropped */
 	}

	if (checkUARTDataInput(rx_buf, checkPhrase1) == 1) {
		extractTemperatureData(rx_buf, 4);
		print_uart("1\r\n");
	}

	if (checkUARTDataInput(rx_buf, checkPhrase2) == 1) {
		extractLongLongs(rx_buf);
		print_uart("2\r\n");
	}

	if (checkUARTDataInput(rx_buf, checkPhrase3) == 1) {
		print_uart("3\r\n");
	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
 	int msg_len = strlen(buf);

 	for (int i = 0; i < msg_len; i++) {
 		uart_poll_out(uart_dev, buf[i]);
 	}
 }

int main()
{
	if (!device_is_ready(uart_dev)) {
	 	printk("UART device not found!");
	 	return 0;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
	 	return 0;
	}
	uart_irq_rx_enable(uart_dev);

	print_uart("Hello! I'm your echo bot.\r\n");
	print_uart("Tell me something and press enter:\r\n");

	// while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
	// 	print_uart("Echo: ");
	// 	print_uart(tx_buf);
	// 	print_uart("\r\n");

	// 	if (checkUARTDataInput(tx_buf, checkPhrase1) == 1) {
	// 		extractTemperatureData(tx_buf, 4);

	// 		floatToChar(returnTempData(1), 2);
	// 		print_uart(returnCharBuffer());
	// 		print_uart("\r\n");

	// 		floatToChar(returnTempData(2), 2);
	// 		print_uart(returnCharBuffer());
	// 		print_uart("\r\n");

	// 		floatToChar(returnTempData(3), 2);
	// 		print_uart(returnCharBuffer());
	// 		print_uart("\r\n");

	// 		floatToChar(returnTempData(4), 2);
	// 		print_uart(returnCharBuffer());
	// 		print_uart("\r\n");
	// 	}

	// 	if (checkUARTDataInput(tx_buf, checkPhrase2) == 1) {
	// 		extractLongLongs(tx_buf);

	// 		longLongToChar(returnSDData(1), returnCharBuffer());
	// 		print_uart(returnCharBuffer());
	// 		print_uart("\r\n");

	// 		longLongToChar(returnSDData(2), returnCharBuffer());
	// 		print_uart(returnCharBuffer());
	// 		print_uart("\r\n");


	// 		//floatToChar((divideLongLong(returnSDData(1), returnSDData(2)) * 100), 2);
	// 		//write line that turns an int into a percentage (9900 -> 99.00%)
	// 		//print_uart(returnCharBuffer());
	// 		//print_uart("\r\n");
	// 	}

	// 	if (checkUARTDataInput(tx_buf, checkPhrase3) == 1) {
			
	// 	}
	// }

	CHIP_ERROR err = AppTask::Instance().StartApp();

	LOG_ERR("Exited with code %" CHIP_ERROR_FORMAT, err.Format());
	return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
