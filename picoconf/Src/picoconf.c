/*
 * picoconf.c
 *
 *  Created on: Jan 14, 2026
 *      Author: Nathan
 */

#include "picoconf.h"

#define PAGE_SIZE (2*1024) //Each page is 2KB
const uint8_t reserved_flash_sector[PAGE_SIZE] __attribute__((__used__))  __attribute__ ((section (".reserved_flash")));

uint32_t _search_next_u64_unprog(void);

void picoconf_flash_init(uint64_t min_conf_invalid_value, uint64_t default_conf){

	__IO uint64_t* read_address = (__IO uint64_t*) reserved_flash_sector;

	for(int i = 0; i < PAGE_SIZE/8; i++){

		uint64_t val = read_address[i];

		if(val != 0xFFFFFFFFFFFFFFFF && val >= min_conf_invalid_value){

			picoconf_flash_reset(default_conf);
		}
	}
}

uint64_t picoconf_get_conf_from_flash(uint64_t default_conf){

	__IO uint64_t* read_address = (__IO uint64_t*) reserved_flash_sector;

	uint32_t index = _search_next_u64_unprog();
	if(index == 0) {
		return default_conf;
	}

	return read_address[index-1];
}

void picoconf_save_conf_to_flash(uint64_t conf){

	if(conf == picoconf_get_conf_from_flash(conf+1)){
		return;
	}

	uint32_t index = _search_next_u64_unprog();

	if(index >= PAGE_SIZE/8){
		picoconf_flash_reset(conf);
	}
	else{

		__IO uint64_t* read_address = (__IO uint64_t*) reserved_flash_sector;
		read_address += index;

		HAL_FLASH_Unlock();

		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)read_address, conf);

		HAL_FLASH_Lock();
	}
}

// unprog the flash and set the first double-word to default-value
void picoconf_flash_reset(uint64_t default_conf_value){

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef page1_erase;
	page1_erase.TypeErase = FLASH_TYPEERASE_PAGES;
	page1_erase.Banks = FLASH_BANK_1;
	page1_erase.Page = 1;
	page1_erase.NbPages = 1;

	uint32_t page_error;
	HAL_FLASHEx_Erase(&page1_erase, &page_error);

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)reserved_flash_sector, default_conf_value);

	HAL_FLASH_Lock();
}

uint32_t _search_next_u64_unprog(void){

	__IO uint64_t* read_address = (__IO uint64_t*) reserved_flash_sector;

	for(int i = 0; i < PAGE_SIZE/8; i++){

		if(read_address[i] == 0xFFFFFFFFFFFFFFFF){
			return i;
		}
	}

	return PAGE_SIZE/8;
}
