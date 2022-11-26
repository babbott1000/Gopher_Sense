// Autogenerated by SensorCannon
//

#ifndef MODULE_HW_CONFIG_H
#define MODULE_HW_CONFIG_H

#include "gsense_structs.h"


#define NUM_ADC1_PARAMS 3
#define NUM_ADC2_PARAMS 3
#define NUM_ADC3_PARAMS 2
#define NUM_BUCKETS 2

// analog params must be in channel order
#if NUM_ADC1_PARAMS > 0
extern ANALOG_SENSOR_PARAM adc1_sensor_params[NUM_ADC1_PARAMS];
#endif // NUM_ADC1_PARAMS > 0

#if NUM_ADC2_PARAMS > 0
extern ANALOG_SENSOR_PARAM adc2_sensor_params[NUM_ADC2_PARAMS];
#endif // NUM_ADC2_PARAMS > 0

#if NUM_ADC3_PARAMS > 0
extern ANALOG_SENSOR_PARAM adc3_sensor_params[NUM_ADC3_PARAMS];
#endif // NUM_ADC3_PARAMS > 0

#if NUM_BUCKETS > 0
extern BUCKET bucket_list[NUM_BUCKETS];
#endif // NUM_BUCKETS > 0

#endif // MODULE_HW_CONFIG_H
// End autogenerated file