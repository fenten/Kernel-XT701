/*
 * arch/arm/mach-omap2/board-mapphone-keypad.c
 *
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009-2010 Motorola, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_event.h>
#include <linux/keyreset.h>

#include <plat/mux.h>
#include <plat/gpio.h>
#include <linux/gpio_mapping.h>
#include <plat/keypad.h>
#include <plat/board-mapphone.h>

#ifdef CONFIG_ARM_OF
#include <mach/dt_path.h>
#include <asm/prom.h>
#endif

#ifdef CONFIG_KEYBOARD_ADP5588
#include <linux/adp5588_keypad.h>
#endif


static unsigned int mapphone_col_gpios[] = { 43, 53, 54, 55, 56, 57, 58, 63 };
static unsigned int mapphone_row_gpios[] = { 34, 35, 36, 37, 38, 39, 40, 41 };

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(mapphone_row_gpios) + (row))

static const unsigned short mapphone_p3_keymap[ARRAY_SIZE(mapphone_col_gpios) *
					     ARRAY_SIZE(mapphone_row_gpios)] = {
	[KEYMAP_INDEX(0, 0)] = KEY_9,
	[KEYMAP_INDEX(0, 1)] = KEY_R,
	[KEYMAP_INDEX(0, 2)] = KEY_SEND, /* n/c dummy for CALLSEND testing*/
	[KEYMAP_INDEX(0, 3)] = KEY_VOLUMEDOWN,
	[KEYMAP_INDEX(0, 4)] = KEY_F4,   /* n/c dummy for CALLEND testing */
	[KEYMAP_INDEX(0, 5)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(0, 6)] = KEY_SEARCH,
	[KEYMAP_INDEX(0, 7)] = KEY_D,

	[KEYMAP_INDEX(1, 0)] = KEY_7,
	[KEYMAP_INDEX(1, 1)] = KEY_M,
	[KEYMAP_INDEX(1, 2)] = KEY_L,
	[KEYMAP_INDEX(1, 3)] = KEY_K,
	[KEYMAP_INDEX(1, 4)] = KEY_N,
	[KEYMAP_INDEX(1, 5)] = KEY_C,
	[KEYMAP_INDEX(1, 6)] = KEY_Z,
	[KEYMAP_INDEX(1, 7)] = KEY_RIGHTSHIFT,

	[KEYMAP_INDEX(2, 0)] = KEY_1,
	[KEYMAP_INDEX(2, 1)] = KEY_Y,
	[KEYMAP_INDEX(2, 2)] = KEY_I,
	[KEYMAP_INDEX(2, 3)] = KEY_COMMA,
	[KEYMAP_INDEX(2, 4)] = KEY_LEFTALT,
	[KEYMAP_INDEX(2, 5)] = KEY_DOT,
	[KEYMAP_INDEX(2, 6)] = KEY_G,
	[KEYMAP_INDEX(2, 7)] = KEY_E,

/*	[KEYMAP_INDEX(3, 0)] = KEY_, */
	[KEYMAP_INDEX(3, 1)] = KEY_6,
	[KEYMAP_INDEX(3, 2)] = KEY_3,
	[KEYMAP_INDEX(3, 3)] = KEY_DOWN,
	[KEYMAP_INDEX(3, 4)] = KEY_UP,
	[KEYMAP_INDEX(3, 5)] = KEY_LEFT,
	[KEYMAP_INDEX(3, 6)] = KEY_RIGHT,
	[KEYMAP_INDEX(3, 7)] = KEY_REPLY,	/* d-pad center key */

	[KEYMAP_INDEX(4, 0)] = KEY_5,
	[KEYMAP_INDEX(4, 1)] = KEY_J,
	[KEYMAP_INDEX(4, 2)] = KEY_B,
	[KEYMAP_INDEX(4, 3)] = KEY_CAMERA-1,	/* camera 1 key, steal KEY_HP*/
	[KEYMAP_INDEX(4, 4)] = KEY_T,
	[KEYMAP_INDEX(4, 5)] = KEY_CAMERA,	/* "camera 2" key */
	[KEYMAP_INDEX(4, 6)] = KEY_MENU,
	[KEYMAP_INDEX(4, 7)] = KEY_X,

	[KEYMAP_INDEX(5, 0)] = KEY_8,
	[KEYMAP_INDEX(5, 1)] = KEY_SPACE,
	[KEYMAP_INDEX(5, 2)] = KEY_RIGHTALT,
/*	[KEYMAP_INDEX(5, 3)] = KEY_, */
	[KEYMAP_INDEX(5, 4)] = KEY_SLASH,
	[KEYMAP_INDEX(5, 5)] = KEY_EMAIL,	/* @ */
	[KEYMAP_INDEX(5, 6)] = KEY_BACKSPACE,
	[KEYMAP_INDEX(5, 7)] = KEY_A,

	[KEYMAP_INDEX(6, 0)] = KEY_2,
	[KEYMAP_INDEX(6, 1)] = KEY_0,
	[KEYMAP_INDEX(6, 2)] = KEY_F,
	[KEYMAP_INDEX(6, 3)] = KEY_LEFTSHIFT,
	[KEYMAP_INDEX(6, 4)] = KEY_ENTER,
	[KEYMAP_INDEX(6, 5)] = KEY_O,
	[KEYMAP_INDEX(6, 6)] = KEY_H,
	[KEYMAP_INDEX(6, 7)] = KEY_Q,

	[KEYMAP_INDEX(7, 0)] = KEY_4,
	[KEYMAP_INDEX(7, 1)] = KEY_V,
	[KEYMAP_INDEX(7, 2)] = KEY_S,
	[KEYMAP_INDEX(7, 3)] = KEY_P,
	[KEYMAP_INDEX(7, 4)] = KEY_QUESTION,
	[KEYMAP_INDEX(7, 5)] = KEY_MUTE,
	[KEYMAP_INDEX(7, 6)] = KEY_U,
	[KEYMAP_INDEX(7, 7)] = KEY_W,
};

#ifndef CONFIG_ARM_OF
static const unsigned short mapphone_keymap_closed[
	ARRAY_SIZE(mapphone_col_gpios) * ARRAY_SIZE(mapphone_row_gpios)] = {
	[KEYMAP_INDEX(0, 3)] = KEY_VOLUMEDOWN,
	[KEYMAP_INDEX(0, 5)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(4, 3)] = KEY_CAMERA-1,	/* camera 1 key, steal KEY_HP*/
	[KEYMAP_INDEX(4, 5)] = KEY_CAMERA,	/* "camera 2" key */
};
#else
static const unsigned short *mapphone_keymap_closed;
#endif

#ifdef CONFIG_KEYBOARD_ADP5588
static struct adp5588_leds_platform_data mapphone_adp5588_leds_pdata;

static struct platform_device mapphone_adp5588_leds_dev = {
	.name		= ADP5588_BACKLIGHT_NAME,
	.id		= -1,
	.dev		= {
		.platform_data  = &mapphone_adp5588_leds_pdata,
	},
};

struct adp5588_platform_data mapphone_adp5588_pdata = {
	.leds_device = &mapphone_adp5588_leds_dev,
};
#endif

static struct gpio_event_direct_entry mapphone_keypad_switch_map[] = {
	{GPIO_SLIDER,		SW_LID}
};

static int fixup(int index)
{
       int slide_open = gpio_get_value(mapphone_keypad_switch_map[0].gpio);
       if (!slide_open)
		return mapphone_keymap_closed[index];
       return 1;
}

static struct gpio_event_matrix_info mapphone_keypad_matrix_info = {
	.info.func = gpio_event_matrix_func,
	.keymap = mapphone_p3_keymap,
	.output_gpios = mapphone_col_gpios,
	.input_gpios = mapphone_row_gpios,
#ifndef CONFIG_ARM_OF
	.sw_fixup = fixup,
#endif
	.noutputs = ARRAY_SIZE(mapphone_col_gpios),
	.ninputs = ARRAY_SIZE(mapphone_row_gpios),
	.settle_time.tv.nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.flags = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_REMOVE_PHANTOM_KEYS |
		 GPIOKPF_PRINT_UNMAPPED_KEYS /*| GPIOKPF_PRINT_MAPPED_KEYS*/
};

static struct gpio_event_input_info mapphone_keypad_switch_info = {
	.info.func = gpio_event_input_func,
	.flags = 0,
	.type = EV_SW,
	.keymap = mapphone_keypad_switch_map,
	.keymap_size = ARRAY_SIZE(mapphone_keypad_switch_map)
};

static struct gpio_event_info *mapphone_keypad_info[] = {
	&mapphone_keypad_matrix_info.info,
	&mapphone_keypad_switch_info.info,
};

static struct gpio_event_platform_data mapphone_keypad_data = {
	.name = "sholes-keypad",
	.info = mapphone_keypad_info,
	.info_count = ARRAY_SIZE(mapphone_keypad_info)
};

static struct platform_device mapphone_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &mapphone_keypad_data,
	},
};

static int mapphone_reset_keys_up[] = {
	BTN_MOUSE,		/* XXX */
	0
};

static int mapphone_reset_keys_down[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_END,
	0
};

static struct keyreset_platform_data mapphone_reset_keys_pdata = {
	.crash_key = KEY_END,
	.keys_up = mapphone_reset_keys_up,
	.keys_down = mapphone_reset_keys_down,
};

struct platform_device mapphone_reset_keys_device = {
	.name = KEYRESET_NAME,
	.dev.platform_data = &mapphone_reset_keys_pdata,
};

#if defined(CONFIG_KEYBOARD_ADP5588) && defined(CONFIG_ARM_OF)
static void mapphone_dt_adp5588_init(struct device_node *kp_node)
{
	struct device_node *kp_led_node;
	const void *kp_prop;

	/* Assume GPIO matrix keypad by default */
	mapphone_adp5588_pdata.use_adp5588 = 0;

	kp_prop = of_get_property(kp_node, DT_PROP_KEYPAD_ADP5588, NULL);
	if (kp_prop)
		mapphone_adp5588_pdata.use_adp5588 = *(u8 *)kp_prop;

	if (!mapphone_adp5588_pdata.use_adp5588)
		return;

	printk(KERN_INFO "%s: Keypad device is ADP5588\n", __func__);

	mapphone_keypad_matrix_info.info.func = gpio_event_adp5588_func;
	mapphone_keypad_data.name = ADP5588_KEYPAD_NAME;

	mapphone_adp5588_pdata.reset_gpio = get_gpio_by_name("adp5588_reset_b");

	/* If RESET GPIO is not configured in device_tree, assume default */
	if (mapphone_adp5588_pdata.reset_gpio < 0) {
		printk(KERN_INFO "%s: ADP5588: RESET_GPIO not in device_tree\n",
			__func__);
		mapphone_adp5588_pdata.reset_gpio = ADP5588_RESET_GPIO;
	}

	mapphone_adp5588_pdata.int_gpio = get_gpio_by_name("adp5588_int_b");

	/* If INT GPIO is not configured in device_tree, assume default */
	if (mapphone_adp5588_pdata.int_gpio < 0) {
		printk(KERN_INFO "%s: ADP5588: INT_GPIO not in device_tree\n",
			__func__);
		mapphone_adp5588_pdata.int_gpio = ADP5588_INT_GPIO;
	}

	/* Assume CPCAP keypad leds by default */
	mapphone_adp5588_leds_pdata.use_leds = 0;

	kp_led_node = of_find_node_by_path(DT_KPAD_LED);
	if (kp_led_node) {
		kp_prop = of_get_property(kp_led_node, \
				DT_PROP_ADP5588_KPAD_LED, NULL);
		if (kp_prop)
			mapphone_adp5588_leds_pdata.use_leds = *(u8 *)kp_prop;

		of_node_put(kp_led_node);
	}
}
#endif

#ifdef CONFIG_INPUT_KEYRESET
static int mapphone_dt_kpreset_init(void)
{
	struct device_node *kpreset_node;
	const void *kpreset_prop;
	kpreset_node = of_find_node_by_path(DT_PATH_KEYRESET);
	if (kpreset_node) {
		kpreset_prop = of_get_property(kpreset_node, \
				DT_PROP_KEYRESET_UP, NULL);
		if (kpreset_prop)
			memcpy(mapphone_reset_keys_up, kpreset_prop, \
				sizeof(mapphone_reset_keys_up) - sizeof(int));

		kpreset_prop = of_get_property(kpreset_node, \
				DT_PROP_KEYRESET_DOWN, NULL);
		if (kpreset_prop)
			memcpy(mapphone_reset_keys_down, kpreset_prop, \
				sizeof(mapphone_reset_keys_down) - sizeof(int));

		kpreset_prop = of_get_property(kpreset_node, \
				DT_PROP_KEYRESET_CRASH, NULL);
		if (kpreset_prop)
			mapphone_reset_keys_pdata.crash_key =\
				*(int *)kpreset_prop;
		of_node_put(kpreset_node);
	}
	return kpreset_node ? 0 : -ENODEV;
}
#endif

#ifdef CONFIG_ARM_OF
static int __init mapphone_dt_kp_init(void)
{
	struct device_node *kp_node;
	const void *kp_prop;
	int slider_gpio;

	if ((kp_node = of_find_node_by_path(DT_PATH_KEYPAD))) {
		if ((kp_prop = of_get_property(kp_node, \
				DT_PROP_KEYPAD_ROWS, NULL)))
			mapphone_keypad_matrix_info.ninputs = \
				*(int *)kp_prop;

		if ((kp_prop = of_get_property(kp_node, \
				DT_PROP_KEYPAD_COLS, NULL)))
			mapphone_keypad_matrix_info.noutputs = \
				*(int *)kp_prop;

		if ((kp_prop = of_get_property(kp_node, \
				DT_PROP_KEYPAD_ROWREG, NULL)))
			mapphone_keypad_matrix_info.input_gpios = \
				(int *)kp_prop;

		if ((kp_prop = of_get_property(kp_node, \
				DT_PROP_KEYPAD_COLREG, NULL)))
			mapphone_keypad_matrix_info.output_gpios = \
				(int *)kp_prop;

		if ((kp_prop = of_get_property(kp_node, \
				DT_PROP_KEYPAD_MAPS, NULL)))
			mapphone_keypad_matrix_info.keymap = \
				(unsigned short *)kp_prop;

		slider_gpio = get_gpio_by_name("slider_data");
		if (slider_gpio < 0)
			slider_gpio = GPIO_SLIDER;
		mapphone_keypad_switch_map[0].gpio = slider_gpio;

		kp_prop = of_get_property(kp_node, \
				DT_PROP_KEYPAD_CLOSED_MAPS, NULL);
		if (kp_prop) {
			mapphone_keymap_closed = (unsigned short *)kp_prop;
			mapphone_keypad_matrix_info.sw_fixup = fixup;
		}

#ifdef CONFIG_KEYBOARD_ADP5588
		mapphone_dt_adp5588_init(kp_node);
#endif

		of_node_put(kp_node);
	}

	return kp_node ? 0 : -ENODEV;
}
#endif

static int __init mapphone_init_keypad(void)
{
#ifdef CONFIG_ARM_OF
	if (mapphone_dt_kp_init())
		printk(KERN_INFO "Keypad: using non-dt configuration\n");
#endif

#ifdef CONFIG_INPUT_KEYRESET
	if (mapphone_dt_kpreset_init())
		printk(KERN_INFO "Keypadreset init failed\n");
#endif

	/* keypad rows */
	omap_cfg_reg(K4_34XX_GPIO37);
	omap_cfg_reg(R3_34XX_GPIO39);

	/* keypad columns */
	omap_cfg_reg(K3_34XX_GPIO43_OUT);
	omap_cfg_reg(R8_34XX_GPIO56_OUT);
	omap_cfg_reg(P8_34XX_GPIO57_OUT);

	/* switches */
	omap_cfg_reg(AB2_34XX_GPIO177);
	omap_cfg_reg(AH17_34XX_GPIO100);

	platform_device_register(&mapphone_reset_keys_device);
	return platform_device_register(&mapphone_keypad_device);
}

device_initcall(mapphone_init_keypad);
