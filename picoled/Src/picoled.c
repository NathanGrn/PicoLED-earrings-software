/*
 * picoled.c
 *
 *  Created on: Dec 23, 2025
 *      Author: nathan
 */

#include "picoled.h"
#include "main.h"
#include "math.h"
#include "string.h"



// Init function

extern TIM_HandleTypeDef htim1; //fixme

pled_status_t pled_init(pled_ctx_t* ctx, pled_color_t* led_array, uint16_t n_leds) {

	ctx->led_array = led_array;
	ctx->n_leds = n_leds;

	ctx->_is_busy = false;
	ctx->_led_index = 0;
	memset(ctx->_timer_buffer, 0, sizeof(ctx->_timer_buffer));

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)ctx->_timer_buffer, 300); //fixme

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

// Display functions
pled_status_t pled_display(pled_ctx_t* ctx){

	if(ctx->_is_busy){
		return PLED_BUSY;
	}

	uint16_t* buff_addr = ctx->_timer_buffer+100;

	for(ctx->_led_index = 0; ctx->_led_index<ctx->n_leds; ctx->_led_index++){
		for(int i = 0; i<3; i++){
			for(int j = 0; j<8; j++){
				uint8_t mask = 1<<(7-j);
				if(((uint8_t*)&(ctx->led_array[ctx->_led_index]))[i]&mask){
					buff_addr[i*8+j] = 30;
				}
				else{
					buff_addr[i*8+j] = 12;
				}
			}
		}
	}

	ctx->_led_index = 0;
	ctx->_is_busy = true;

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)  ctx->_timer_buffer, 300);

	return PLED_OK;
}

extern pled_ctx_t pled_ctx; //fixme

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){

	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);
	pled_ctx._is_busy = false;
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
