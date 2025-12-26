/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "picoled.h"
#include "stdlib.h"
#include "math.h"
#include "arm_math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum led_mode{

	RANDOM_BLINK = 0,
	SLOW_COLOR_CHANGE,
	AUDIO_RESPONSE,
	END_MODE_LIST //add new mode before this enum value
} led_mode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFF_EXPONENT 9 //must be 9
#define BUFF_SIZE (1<<BUFF_EXPONENT)

#define EARRING_STATIC_BRIGHTNESS 0.05f // Value between 0 and 1 included

#define RANDOM_BLINK_BRIGHTNESS 0.4f
#define RANDOM_BLINK_TEST_PERIOD 100 // integer in millisecond
#define RANDOM_BLINK_TEST_CHANCE 54 // integer between 0 and 99 included
#define RANDOM_BLINK_FLASH_DURATION 35 // integer in millisecond

#define SLOW_COLOR_CHANGE_PERIOD 120.0f // Period in second for a full color rotation (0 to 360° on hsv wheel)
#define COLOR_CHANGE_ROLLER_STATIC_BRIGHTNESS 0.05f

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch2;

/* USER CODE BEGIN PV */
pled_ctx_t pled_ctx;
arm_rfft_fast_instance_f32 fft_s;

uint8_t conv_complete = 1;
uint16_t audio_buffer[BUFF_SIZE] = {0};
float32_t fft_buffer[BUFF_SIZE] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

led_mode_t run_fsm(led_mode_t current_mode, bool do_transition){

	if(do_transition){
		current_mode++;
		if(current_mode >= END_MODE_LIST){
			current_mode = 0;
		}
	}

	return current_mode;
}

void do_random_blink(pled_ctx_t* _pled_ctx){

	static uint32_t last_millis = 0;

	static pled_hsv_t hsv = {.hue = 0, .sat=1.0, .val=0.01};
	static pled_color_t black = {0};
	pled_color_t pled_color = {0};

	static uint32_t led_time_table[4] = {0};

	if(HAL_GetTick()-last_millis >= RANDOM_BLINK_TEST_PERIOD){

		//1st led is white
		hsv.sat = 0;
		hsv.val = EARRING_STATIC_BRIGHTNESS;
		hsv2pled(&hsv, &pled_color);
		pled_set(_pled_ctx, &pled_color, 0);

		hsv.sat = 1.0;
		//2 to 5 are random blink each
		for(int i = 0; i < 4; i++){

			int32_t blink = rand()%100;
			int32_t color = rand()%3;

			if(blink<=RANDOM_BLINK_TEST_CHANCE && led_time_table[i]==0){
				led_time_table[i] = HAL_GetTick();
				hsv.val = RANDOM_BLINK_BRIGHTNESS;
				hsv.hue = color*120;
				hsv2pled(&hsv, &pled_color);
				pled_set(_pled_ctx, &pled_color, i+1);
			}
		}

		last_millis = HAL_GetTick();
	}

	for(int i = 0; i<4; i++){

		if((led_time_table[i] != 0) && (HAL_GetTick()-led_time_table[i]>=RANDOM_BLINK_FLASH_DURATION)){
			pled_set(_pled_ctx, &black, i+1);
			led_time_table[i] = 0;
		}
	}

	return;
}

void do_slow_color_change(pled_ctx_t* _pled_ctx){

	static uint32_t last_millis = 0;
	static pled_hsv_t hsv = {.hue = 0.0, .sat=1.0, .val=0.01};
	pled_color_t pled_color = {0};

	float color_increment = 360.0/(SLOW_COLOR_CHANGE_PERIOD*1000.0);

	uint32_t actual_ms = HAL_GetTick();
	if(actual_ms-last_millis >= 1){

		hsv.hue += color_increment;//*(float)(actual_ms-last_millis);
		if(hsv.hue >= 360.0){
			hsv.hue = 0.0;
		}

		hsv.val = EARRING_STATIC_BRIGHTNESS;
		hsv2pled(&hsv, &pled_color);
		pled_set(_pled_ctx, &pled_color, 0);

		hsv.val = COLOR_CHANGE_ROLLER_STATIC_BRIGHTNESS;
		hsv2pled(&hsv, &pled_color);
		pled_set_array(_pled_ctx, &pled_color, 1, 4);

		last_millis = actual_ms;
	}

	return;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
	HAL_ADC_Stop_DMA(&hadc1);
	conv_complete = 1;
}

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

int32_t compute_mean_value(uint16_t* buffer){

	int32_t acc = 0;

	for(int i = 0; i<BUFF_SIZE; i++){
		acc += buffer[i];
	}

	acc = acc >> BUFF_EXPONENT;

	return acc;
}

int32_t compute_RMS_value(uint16_t* buffer){

	int32_t mean = compute_mean_value(buffer);
	int32_t acc = 0;

	for(int i = 0; i<BUFF_SIZE; i++){

		int32_t dc_removed = (int32_t)buffer[i]-mean;
		acc += dc_removed*dc_removed;
	}
	acc = acc >> BUFF_EXPONENT;

	return isqrt(acc);
}

void remove_dc_and_fill_float_buff(uint16_t* u16_buff, float* f32_buff){

	int32_t dc_offset = compute_mean_value(u16_buff);
	for(int i = 0; i<BUFF_SIZE; i++){
		f32_buff[i] = (float)(u16_buff[i]-dc_offset);
	}

	return;
}

uint32_t compute_abs_fft(float32_t* buff){

	uint32_t j = 1;

	for(int i=1; i<BUFF_SIZE; i+=2){

		buff[j]=sqrtf(buff[i]*buff[i] + buff[i+1]*buff[i+1]);
		j++;
	}

	buff[j] = buff[BUFF_SIZE-1];
	return j+1;
}

float32_t average_bins(float32_t* fft, uint32_t bin_start, uint32_t bin_end){

  float32_t acc = 0;
  for(int i = bin_start; i<bin_end; i++){
    acc += fft[i];
  }

  acc = acc/(float32_t)(bin_end-bin_start);

  return acc;
}

void do_audio_response(pled_ctx_t* _pled_ctx){

  static float32_t gain = 0.003;

  //ADC input stuff
	// Wait for all samples to be acquired
	if(conv_complete){

		//copy raw audio buffer to float array
		remove_dc_and_fill_float_buff(audio_buffer, fft_buffer);

		//re-launch acquisition
		conv_complete = 0;
		HAL_ADC_Start_DMA(&hadc1, (uint32_t*)audio_buffer, BUFF_SIZE); //This take ~50ms

		//compute fft
		arm_rfft_fast_f32(&fft_s, fft_buffer, fft_buffer, 0);
		compute_abs_fft(fft_buffer);

		//do led stuff here
    pled_color_t black = {0};
    pled_set_all(&pled_ctx, &black);
    pled_display(&pled_ctx);

    float32_t vocal_bins = average_bins(fft_buffer, 15, 73)-140.0;//(15, 73)
    vocal_bins = fmaxf(vocal_bins, 1.0);

    hsv.val = 0.01;
    hsv.val += gain*log10f(vocal_bins);
    hsv.val = fmaxf(0.0,fminf(hsv.val, 1.0));
	}

	return;
}

bool debounce_button(bool button_value){

	static uint32_t last_millis = 0;
	static bool last_state = false;

	if(button_value != last_state){
		if(HAL_GetTick()-last_millis > 40){
			last_state = button_value;
		}
	}
	else{
		last_millis = HAL_GetTick();
	}

	return last_state;
}

bool falling_edge_detect(bool button_value){

	static bool last_state = false;
	bool falling_edge_detected = false;

	if(!button_value && last_state){
		falling_edge_detected = true;
	}
	last_state = button_value;

	return falling_edge_detected;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  //Calibrate the ADC
  HAL_ADCEx_Calibration_Start(&hadc1);

  // Init LED output then enable 5V supply
  pled_color_t led_array[5];
  pled_init(&pled_ctx, led_array, 5);
  HAL_Delay(10);
  HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

  // Clear the LEDs just in case
  pled_color_t black = {0};
  pled_set_all(&pled_ctx, &black);
  pled_display(&pled_ctx);

  // Init the fft config
  arm_rfft_fast_init_f32(&fft_s, BUFF_SIZE);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	static led_mode_t current_mode = RANDOM_BLINK;
	bool button_value = debounce_button(HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin));
	current_mode = run_fsm(current_mode, falling_edge_detect(button_value));

	switch (current_mode) {
		case RANDOM_BLINK:
			do_random_blink(&pled_ctx);
			break;

		case SLOW_COLOR_CHANGE:
			do_slow_color_change(&pled_ctx);
			break;

		case AUDIO_RESPONSE:
			do_audio_response(&pled_ctx);
			break;

		default:
			break;
	}

  while(pled_is_busy(&pled_ctx)){;}
	pled_display(&pled_ctx);
	//HAL_Delay(10);


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV32;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_79CYCLES_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
  hadc1.Init.OversamplingMode = ENABLE;
  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_32;
  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_1;
  hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 63;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BTN_Pin */
  GPIO_InitStruct.Pin = BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EN_5V_Pin */
  GPIO_InitStruct.Pin = EN_5V_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(EN_5V_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
