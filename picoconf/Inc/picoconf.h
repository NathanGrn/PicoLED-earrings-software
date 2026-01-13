/*
 * picoconf.h
 *
 *  Created on: Jan 14, 2026
 *      Author: Nathan
 */

#ifndef INC_PICOCONF_H_
#define INC_PICOCONF_H_

#include "stm32g0xx_hal.h"

/*
 * picoconf uses 1 page of flash (defined in ld script and in .c) as a circular buffer
 * to store conf in a double word format (uint64_t).
 * Each time a conf is saved, the next available double word slot is programmed in the flash.
 * When the page is fully programmed, this lib reset the page and program the first slot accordingly.
 */

// If any double word value is >= to 'min_conf_invalid_value' in config flash,
// this function call picoconf_flash_reset() with 'default_conf'
void picoconf_flash_init(uint64_t min_conf_invalid_value, uint64_t default_conf);

// Retrieve config double word from config flash
// If config flash is empty return default_conf
uint64_t picoconf_get_conf_from_flash(uint64_t default_conf);

// Save 'conf' double word in config flash
void picoconf_save_conf_to_flash(uint64_t conf);

// unprog the flash and set the first double-word to default_conf_value
void picoconf_flash_reset(uint64_t default_conf_value);

#endif /* INC_PICOCONF_H_ */
