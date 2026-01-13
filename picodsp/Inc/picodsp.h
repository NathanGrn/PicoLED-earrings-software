/*
 * picodsp.h
 *
 *  Created on: Jan 13, 2026
 *      Author: Nathan
 */

#ifndef INC_PICODSP_H_
#define INC_PICODSP_H_

#include "stdint.h"
#include "stdbool.h"
#include "arm_math.h"

int32_t isqrt(int32_t x);

// compute the mean value for a buffer with size = 2^n
int32_t compute_mean_value(uint16_t* buffer, uint16_t two_pow_n_size);

// compute the rms value for a buffer with size = 2^n
int32_t compute_RMS_value(uint16_t* buffer, uint16_t two_pow_n_size);

// copy, remove mean value and convert u16_buff of size = 2^n to f32_buff of the same size
void remove_dc_and_fill_float_buff(uint16_t* u16_buff, float32_t* f32_buff, uint16_t two_pow_n_size);

void apply_hanning_window(float32_t* buff, uint32_t len);

// compute absolute value of a complex fft in place, return value is the resulting size (should be len/2)
uint32_t compute_abs_fft(float32_t* buff, uint32_t len);

float32_t sum_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end);

float32_t average_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end);

void find_max_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end, uint32_t* max_bin_index, float32_t* max_bin_value);

void find_max_windowed_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end, uint32_t window_size, uint32_t* max_bin_index, float32_t* max_bin_value);

#endif /* INC_PICODSP_H_ */
