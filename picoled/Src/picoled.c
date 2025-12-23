/*
 * picoled.c
 *
 *  Created on: Dec 23, 2025
 *      Author: nathan
 */

#include "picoled.h"
#include "math.h"
#include "string.h"


// Init function
pled_status_t pled_init(pled_ctx_t* ctx, pled_color_t* led_array, uint16_t n_leds) {

	ctx->led_array = led_array;
	ctx->n_leds = n_leds;

	return PLED_OK;
}

// Set color function
pled_status_t pled_set(pled_ctx_t* ctx, pled_color_t* color, uint16_t index){

	if(index >= ctx->n_leds){
		return PLED_ERROR;
	}

	memcpy(ctx->led_array+index, color, sizeof(pled_color_t));

	return PLED_OK;
}
pled_status_t pled_set_all(pled_ctx_t* ctx, pled_color_t* color){

	for(int i = 0; i < ctx->n_leds; i++){
		memcpy(ctx->led_array+i, color, sizeof(pled_color_t));
	}

	return PLED_ERROR;
}

// Display function
pled_status_t pled_display(pled_ctx_t* ctx){

	return PLED_BUSY;
}

// Some utilities
void hsv2pled(pled_hsv_t* hsv, pled_color_t* rgb){

	float k_r = fmodf((5.0 + hsv->hue/60.0), 6.0);
	float k_g = fmodf((3.0 + hsv->hue/60.0), 6.0);
	float k_b = fmodf((1.0 + hsv->hue/60.0), 6.0);

	rgb->green = (hsv->val - hsv->val*hsv->sat*fmaxf(0.0, fminf(fminf(k_g, 4-k_g),1.0)))*255.0;
	rgb->red = (hsv->val - hsv->val*hsv->sat*fmaxf(0.0, fminf(fminf(k_r, 4-k_r),1.0)))*255.0;
	rgb->blue = (hsv->val - hsv->val*hsv->sat*fmaxf(0.0, fminf(fminf(k_b, 4-k_b),1.0)))*255.0;
}
