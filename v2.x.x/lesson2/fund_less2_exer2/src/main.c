/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

/* STEP 9 - Increase the sleep time from 100ms to 10 minutes  */
#define SLEEP_TIME_MS   100

/* SW0_NODE is the devicetree node identifier for the "sw0" alias */
#define SW0_NODE	DT_ALIAS(sw0) 
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

/* LED0_NODE is the devicetree node identifier for the "led0" alias. */
#define LED0_NODE	DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define STACKSIZE 1024
#define PRIORITY 7
static K_SEM_DEFINE(chatter_ok, 0, 1);


/* STEP 4 - Define the callback function */
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
#ifdef CONFIG_MYLOGGING_INTERRUPT
	printk("button_pressed: %d\r\n", pins);
#endif
	k_sem_give(&chatter_ok);
}

/* STEP 5 - Define a variable of type static struct gpio_callback */
static struct gpio_callback button_cb_data;

int main(void)
{
	int ret;

	printk("start\r\n");

	if (!device_is_ready(led.port)) {
		return -1;
	}

	if (!device_is_ready(button.port)) {
		return -1;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return -1;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return -1;
	}
	/* STEP 3 - Configure the interrupt on the button's pin */
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	/* STEP 6 - Initialize the static struct gpio_callback variable   */
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));

	/* STEP 7 - Add the callback function by calling gpio_add_callback()   */
	gpio_add_callback(button.port, &button_cb_data);

	printk("forever\r\n");
	k_sleep(K_FOREVER);
}

static void button_chatter_thread(void)
{
	while (1) {
		k_sem_take(&chatter_ok, K_FOREVER);
		printk("button_chatter_thread: after k_sem_take\r\n");

		while (1) {
			int count = 0;
			while (count < 5) {
				bool val = gpio_pin_get_dt(&button);
				if (val) {
					count++;
				} else {
					break;
				}
				k_msleep(10);
			}
			if (count != 5) {
				break;
			}
			gpio_pin_toggle_dt(&led);
			break;
		}

		k_sem_reset(&chatter_ok);
		printk("button_chatter_thread: after k_sem_reset\r\n");
	}
}

static K_THREAD_DEFINE(button_chatter_thread_id, STACKSIZE, button_chatter_thread, NULL, NULL,
		NULL, PRIORITY, 0, 0);
