/*
 * picodsp.c
 *
 *  Created on: Jan 13, 2026
 *      Author: Nathan
 */

#include "picodsp.h"

// from https://rosettacode.org/wiki/Isqrt_(integer_square_root)_of_X#C
int32_t isqrt(int32_t x) {
    int32_t q = 1, r = 0;
    while (q <= x) {
        q <<= 2;
    }
    while (q > 1) {
        int32_t t;
        q >>= 2;
        t = x - r - q;
        r >>= 1;
        if (t >= 0) {
            x = t;
            r += q;
        }
    }
    return r;
}

int32_t compute_mean_value(uint16_t* buffer, uint16_t two_pow_n_size){

	int32_t acc = 0;

	for(int i = 0; i<(1<<two_pow_n_size); i++){
		acc += buffer[i];
	}

	acc = acc >> two_pow_n_size;

	return acc;
}

int32_t compute_RMS_value(uint16_t* buffer, uint16_t two_pow_n_size){

	int32_t mean = compute_mean_value(buffer, two_pow_n_size);
	int32_t acc = 0;

	for(int i = 0; i<(1<<two_pow_n_size); i++){

		int32_t dc_removed = (int32_t)buffer[i]-mean;
		acc += dc_removed*dc_removed;
	}
	acc = acc >> two_pow_n_size;

	return isqrt(acc);
}

void remove_dc_and_fill_float_buff(uint16_t* u16_buff, float32_t* f32_buff, uint16_t two_pow_n_size){

	int32_t dc_offset = compute_mean_value(u16_buff, two_pow_n_size);
	for(int i = 0; i<(1<<two_pow_n_size); i++){
		f32_buff[i] = (float32_t)(u16_buff[i]-dc_offset);
	}

	return;
}


// dirty but not enough flash to store the 512points hanning window weights :)
void apply_hanning_window(float32_t* buff, uint32_t len){

	for(int i = 0; i<len; i++){
		float32_t w = sinf((PI*(float32_t)i)/((float32_t)(len-1)));
		buff[i] = w*w*buff[i];
	}
}

uint32_t compute_abs_fft(float32_t* buff, uint32_t len){

	uint32_t j = 1;

	for(int i=1; i<len; i+=2){

		buff[j]=sqrtf(buff[i]*buff[i] + buff[i+1]*buff[i+1]);
		j++;
	}

	buff[j] = buff[len-1];
	return j+1;
}

float32_t sum_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end){

  float32_t acc = 0;
  for(int i = bin_start; i<bin_end; i++){
    acc += fft[i];
  }

	return acc;
}

float32_t average_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end){

	return sum_bins(fft, bin_start, bin_end)/(float32_t)(bin_end-bin_start);
}

void find_max_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end, uint32_t* max_bin_index, float32_t* max_bin_value){

  *max_bin_index = 0;
  *max_bin_value = 0.0;

  for(int i=bin_start; i<=bin_end; i++){

    if(fft[i] >= *max_bin_value){
      *max_bin_index = i;
      *max_bin_value = fft[i];
    }
  }
}

void find_max_windowed_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end, uint32_t window_size, uint32_t* max_bin_index, float32_t* max_bin_value){

  *max_bin_index = 0;
  *max_bin_value = 0.0;
  float32_t max_acc = 0.0;

  uint32_t window_half_size = window_size/2;
  uint32_t start_window = bin_start<window_half_size?window_half_size:bin_start;
  uint32_t stop_window = bin_end-window_half_size;

  for(int window_center = start_window; window_center<=stop_window; window_center++){

    uint32_t window_start = window_center-window_half_size;
    uint32_t window_end = window_center+window_half_size;
    float32_t acc = 0;

    for(int i=window_start; i<=window_end; i++){

      acc += fft[i];
    }

    if(acc>=max_acc){
      max_acc = acc;
      *max_bin_index = window_center;
      *max_bin_value = fft[window_center];
    }
  }
}
