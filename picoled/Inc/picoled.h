/*
 * picoled.h
 *
 *  Created on: Dec 23, 2025
 *      Author: Nathan
 */

#ifndef INC_PICOLED_H_
#define INC_PICOLED_H_

#include "stdint.h"

typedef struct __attribute__((packed)) {
	uint8_t green;
	uint8_t red;
	uint8_t blue;
}pled_color_t;

typedef struct {
	uint16_t n_leds;
	pled_color_t* led_array;
}pled_ctx_t;

typedef enum {

	PLED_OK = 0,
	PLED_BUSY,
	PLED_ERROR
}pled_status_t;

// Init function
pled_status_t pled_init(pled_ctx_t* ctx, pled_color_t* led_array, uint16_t n_leds);

// Set color function
pled_status_t pled_set(pled_ctx_t* ctx, pled_color_t* color, uint16_t index);
pled_status_t pled_set_all(pled_ctx_t* ctx, pled_color_t* color);

// Display function
pled_status_t pled_display(pled_ctx_t* ctx);

// Some utilities
typedef struct {
	float hue;
	float sat;
	float val;
}pled_hsv_t;

void hsv2pled(pled_hsv_t* hsv, pled_color_t* rgb);

#endif /* INC_PICOLED_H_ */
