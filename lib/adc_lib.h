// adc_lib.h
//  Header file for adc_lib.c

#ifndef ADC_LIB_H
#define ADC_LIB_H

#include "gsense_structs.h"
#include "gopher_sense.h"
#include "GopherCAN.h"
#include "main.h"


//Define error return codes here
#define BUFFER_ERR -1
#define BUFFER_SUCCESS 1
#define CONV_ERR -2
#define RESISTOR_ERR -3
#define BUFFER_EMPTY -4
#define TMR_NOT_CONFIGURED -5
#define ADC_NOT_CONFIGURED -6
#define CONV_SUCCESS 2

// general defines
#define NEED_ADC ((NUM_ADC1_PARAMS > 0) || (NUM_ADC2_PARAMS > 0) || (NUM_ADC3_PARAMS > 0))
#define NEED_HW_TIMER NEED_ADC

// Function prototypes
#if NEED_ADC
S8 configLibADC(ADC_HandleTypeDef* ad1, ADC_HandleTypeDef* ad2,
		        ADC_HandleTypeDef* ad3);
#endif
void startDataAq(void);
void stopDataAq(void);
#if NEED_HW_TIMER
void DAQ_TimerCallback(TIM_HandleTypeDef* timer);
#endif
void add_data_to_buffer(ANALOG_SENSOR_PARAM* param_array,
		                volatile U16* sample_buffer, U32 num_params);
S8 configLibTIM(TIM_HandleTypeDef* tim, U16 tim_freq, U16 psc);
S8 buffer_full(U16_BUFFER* buffer);
S8 add_to_buffer(U16_BUFFER* buffer, U16 toadd);
S8 reset_buffer(U16_BUFFER* buffer);
S8 reset_buffer(U16_BUFFER* buffer);
S8 average_buffer(U16_BUFFER* buffer, U16* avg);
S8 average_buffer_as_float(U16_BUFFER* buffer, float* avg);
S8 apply_analog_sensor_conversion(ANALOG_SENSOR* sensor,
		                          U16 data_in, float* data_out);


#endif // ADC_LIB_H

// End of adc_lib.h
