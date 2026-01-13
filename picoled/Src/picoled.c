/*
 * picoled.c
 *
 *  Created on: Dec 23, 2025
 *      Author: Nathan
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

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t*)ctx->_timer_buffer, 48); //fixme

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

pled_status_t pled_set_array(pled_ctx_t* ctx, pled_color_t* color, uint16_t start, uint16_t end){

	if(start >= ctx->n_leds || end >= ctx->n_leds){
		return PLED_ERROR;
	}

	for(int i = start; i <= end; i++){
		memcpy(ctx->led_array+i, color, sizeof(pled_color_t));
	}

	return PLED_OK;
}

pled_status_t pled_set_all(pled_ctx_t* ctx, pled_color_t* color){

	for(int i = 0; i < ctx->n_leds; i++){
		memcpy(ctx->led_array+i, color, sizeof(pled_color_t));
	}

	return PLED_OK;
}

// Display functions
pled_status_t pled_display(pled_ctx_t* ctx){

	if(ctx->_is_busy){
		return PLED_BUSY;
	}

	ctx->_led_index = 0;
	ctx->_is_busy = true;

	return PLED_OK;
}

bool pled_is_busy(pled_ctx_t* ctx){

	return ctx->_is_busy;
}

void _pled_irq_handler(pled_ctx_t* ctx, bool isHalfCplt){

	if(ctx->_is_busy){
		uint16_t* buff_addr = ctx->_timer_buffer;
		if(!isHalfCplt){ // DMA just finished streaming the 24 higher words
			buff_addr += 24;
		}

		if(ctx->_led_index < ctx->n_leds){
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
		else{
			memset(buff_addr, 0, 24*sizeof(uint16_t));
		}

		ctx->_led_index++;

		if(ctx->_led_index >= ctx->n_leds+2){

			ctx->_is_busy = false; //when timer is flushed, release the lib
		}
	}
}

extern pled_ctx_t pled_ctx; //fixme

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim){

	_pled_irq_handler(&pled_ctx, true);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){

	_pled_irq_handler(&pled_ctx, false);
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
