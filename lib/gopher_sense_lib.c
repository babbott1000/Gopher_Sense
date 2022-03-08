#include "gopher_sense_lib.h"
#include "gopher_sense.h"
#include "GopherCAN.h"
#include "base_types.h"
#include "main.h"

// INCLUDE YOUR HW CONFIG FILE HERE
#include "dam_hw_config.h"


ADC_HandleTypeDef* adc1;
ADC_HandleTypeDef* adc2;
ADC_HandleTypeDef* adc3;

TIM_HandleTypeDef* adc1_timer;
TIM_HandleTypeDef* adc2_timer;
TIM_HandleTypeDef* adc3_timer;

//DMA_HandleTypeDef hdma_adc1;
//DMA_HandleTypeDef hdma_adc2;
//DMA_HandleTypeDef hdma_adc3;

static volatile U16 adc1_sample_buffer[NUM_ADC1_PARAMS];
static volatile U16 adc2_sample_buffer[NUM_ADC2_PARAMS];
static volatile U16 adc3_sample_buffer[NUM_ADC3_PARAMS];

#define ADC_VOLTAGE 3.3
#define VOLTAGE_3V3 3.3
#define VOLTAGE_5V  5.0
#define TIM_CLOCK_BASE_FREQ (HAL_RCC_GetSysClockFreq())
#define TIM_MAX_VAL 65536


//******************* ADC Config *******************
void configLibADC(ADC_HandleTypeDef* ad1, ADC_HandleTypeDef* ad2, ADC_HandleTypeDef* ad3)
{
    adc1 = ad1;
    adc2 = ad2;
    adc3 = ad3;
}


void  HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc_handle)
{
    // stop the DMA and start the timer
    HAL_ADC_Stop_DMA(adc_handle);

#if NUM_ADC1_PARAMS > 0
    if (adc_handle == adc1)
    {
        __HAL_TIM_SET_COUNTER(adc1_timer, 0);
        HAL_TIM_Base_Start_IT(adc1_timer);
        add_data_to_buffer(adc1_sensor_params, adc1_sample_buffer, NUM_ADC1_PARAMS);
    }
#endif // NUM_ADC1_PARAMS > 0
#if NUM_ADC2_PARAMS > 0
    if (adc_handle == adc2)
    {
        __HAL_TIM_SET_COUNTER(adc2_timer, 0);
        HAL_TIM_Base_Start_IT(adc2_timer);
        add_data_to_buffer(adc2_sensor_params, adc2_sample_buffer, NUM_ADC2_PARAMS);
    }
#endif // NUM_ADC2_PARAMS > 0
#if NUM_ADC3_PARAMS > 0
    if (adc_handle == adc3)
    {
        __HAL_TIM_SET_COUNTER(adc3_timer, 0);
        HAL_TIM_Base_Start_IT(adc3_timer);
        add_data_to_buffer(adc3_sensor_params, adc3_sample_buffer, NUM_ADC3_PARAMS);
    }
#endif // NUM_ADC3_PARAMS > 0
}


// for each parameter in this array, transfer the data from the
void add_data_to_buffer(ANALOG_SENSOR_PARAM* param_array, volatile U16* sample_buffer, U32 num_params)
{
	ANALOG_SENSOR_PARAM* param = param_array;
	volatile U16* buffer = sample_buffer;

	while(param - param_array < num_params)
	{
		add_to_buffer(&param->buffer, (U32)*buffer);
		param++;
		buffer++;
	}
}


//******************* Timer interaction *******************
void configLibTIM(TIM_HandleTypeDef* t1, U16 t1_freq,
                  TIM_HandleTypeDef* t2, U16 t2_freq,
                  TIM_HandleTypeDef* t3, U16 t3_freq, U16 psc)
{
    adc1_timer = t1;
    adc2_timer = t2;
    adc3_timer = t3;
    configTimer(adc1_timer, psc, t1_freq);
    configTimer(adc2_timer, psc, t2_freq);
    configTimer(adc3_timer, psc, t3_freq);
}


void configTimer(TIM_HandleTypeDef* timer, U16 psc,  U16 timer_int_freq_hz)
{
    __HAL_TIM_DISABLE(timer);
    __HAL_TIM_SET_COUNTER(timer, 0);
    // TODO Maybe look at how timers are set up to make sure timings are correct
    U32 reload;
    do {
        reload = (TIM_CLOCK_BASE_FREQ/psc) / timer_int_freq_hz;
        psc *= 2;
    } while (reload > TIM_MAX_VAL);

    psc /= 2;
    __HAL_TIM_SET_PRESCALER(timer, psc);
    __HAL_TIM_SET_AUTORELOAD(timer, reload);
    __HAL_TIM_ENABLE(timer);
}


void startTimers (void)
{
    HAL_TIM_Base_Start_IT(adc1_timer);
    HAL_TIM_Base_Start_IT(adc2_timer);
    HAL_TIM_Base_Start_IT(adc3_timer);
}


void stopTimers (void)
{
    HAL_TIM_Base_Stop_IT(adc1_timer);
    HAL_TIM_Base_Stop_IT(adc2_timer);
    HAL_TIM_Base_Stop_IT(adc3_timer);
    __HAL_TIM_SET_COUNTER(adc1_timer, 0);
    __HAL_TIM_SET_COUNTER(adc2_timer, 0);
    __HAL_TIM_SET_COUNTER(adc3_timer, 0);
}


// Call this inside the period elapsed callback
void DAQ_TimerCallback (TIM_HandleTypeDef* timer)
{
    HAL_TIM_Base_Stop_IT(timer);

    if (timer == adc1_timer && NUM_ADC1_PARAMS > 0)
    {
        HAL_ADC_Start_DMA(adc1, (uint32_t*)adc1_sample_buffer, NUM_ADC1_PARAMS);
    }
    else if (timer == adc2_timer && NUM_ADC2_PARAMS > 0)
    {
        HAL_ADC_Start_DMA(adc2, (uint32_t*)adc2_sample_buffer, NUM_ADC2_PARAMS);
    }
    else if (timer == adc3_timer && NUM_ADC3_PARAMS > 0)
    {
        HAL_ADC_Start_DMA(adc3, (uint32_t*)adc3_sample_buffer, NUM_ADC3_PARAMS);
    }
}




//******************* CAN Handling *******************

// Redesign option: pull this into a queue and handle not in an ISR
void sensor_can_message_handle (CAN_HandleTypeDef* hcan, U32 rx_mailbox)
{
    CAN_RxHeaderTypeDef rx_header;
    CAN_MSG message;
    CAN_SENSOR_PARAM* param = can_sensor_params;
    CAN_SENSOR* sensor;
    SENSOR_CAN_MESSAGE* can_info;

    // Get the message
    if (HAL_CAN_GetRxMessage(hcan, rx_mailbox, &rx_header, message.data) != HAL_OK)
    {
        // Handle errors ?
        return;
    }
    message.rtr_bit = rx_header.RTR;
    message.dlc = rx_header.DLC;
    message.id = (rx_header.IDE ? rx_header.ExtId : rx_header.StdId);

    // Check the CAN params for a match
    while (param - can_sensor_params < NUM_CAN_SENSOR_PARAMS)
    {
        sensor = param->can_sensor;
        can_info = &sensor->messages[param->message_idx];

        // TODO support floating point numbers in CAN messages
        // check for ID match between this param message id and the message id
        if (can_info->message_id == message.id)
        {
            U16 data = 0;
            U8 shift = 0;
            // Get correct data based on byte order
            if (sensor->byte_order == LSB)
            {
                for (U8 b = can_info->data_start; b <= can_info->data_end; b++)
                {
                    data &= message.data[b] << shift;
                    shift += 8;
                }
            }

            else if (sensor->byte_order == MSB)
            {
                for (U8 b = can_info->data_end; b >= can_info->data_start; b--)
                {
                    data &= message.data[b] << shift;
                    shift += 8;
                }
            }
            // Add the data to the buffer
            add_to_buffer(&param->buffer, data);
        }

        param++;
    }
}




//******************* Buffer interaction *******************

// Note: Semaphore probably not needed for buffer interaction because reset is atomic

S8 buffer_full (U32_BUFFER* buffer)
{
    if (buffer == NULL)
    {
        return BUFFER_ERR;
    }
    return buffer->fill_level == buffer->buffer_size;
}

S8 add_to_buffer (U32_BUFFER* buffer, U32 toadd)
{
    if (buffer == NULL)
    {
        return BUFFER_ERR;
    }

    if (buffer_full(buffer))
    {
        return BUFFER_ERR;
    }

    buffer->buffer[buffer->fill_level] = toadd;
    buffer->fill_level++;
    return BUFFER_SUCCESS;
}

S8 reset_buffer (U32_BUFFER* buffer)
{
    if (buffer == NULL)
    {
        return BUFFER_ERR;
    }

    buffer->fill_level = 0;
    return BUFFER_SUCCESS;
}

// Could average up to the fill level, returns error for now
S8 average_buffer (U32_BUFFER* buffer, U32* avg)
{
    if (buffer == NULL || !buffer_full(buffer))
    {
        return BUFFER_ERR;
    }

    U32 calc_avg = 0;
    U32* curr_buf = buffer->buffer;

    while (curr_buf - buffer->buffer > buffer->buffer_size)
    {
        calc_avg += *curr_buf;
        curr_buf++;
    }

    *avg = calc_avg / buffer->buffer_size;
    return BUFFER_SUCCESS;

}


S8 apply_can_sensor_conversion (CAN_SENSOR* sensor, U8 msg_idx, float data_in, float* data_out)
{
	if (!sensor || !data_out) return CONV_ERR;

	DATA_SCALAR scalar = sensor->messages[msg_idx].output.scalar;
	*data_out = (data_in + scalar.offset) * scalar.quantization; // TODO Verify this is the case for all sensors
    return CONV_SUCCESS;
}


S8 apply_analog_sensor_conversion (ANALOG_SENSOR* sensor, float data_in, float* data_out)
{
	if (!sensor || !data_out) return CONV_ERR;

    switch (sensor->model.input_type)
    {
		case VOLTAGE:
			return convert_voltage_load(sensor, data_in, data_out);
		case RESISTIVE:
			return convert_resistive_load(sensor, data_in, data_out);
		case CURRENT:
			return convert_current_load(sensor, data_in, data_out);
		default:
			// apply no conversion
			*data_out = data_in;
    }

    return CONV_ERR;
}


S8 convert_voltage_load (ANALOG_SENSOR* sensor, float data_in, float* data_out)
{
	OUTPUT_MODEL model = sensor->model;
	float v_sensor;

	// Convert the ADC reading to a voltage value
	v_sensor = adc_to_volts(data_in, sensor->output.data_size_bits);

	// Convert the voltage read by the ADC to the voltage read at the pin

	// check to make sure there are no pull-ups. There should not be on a voltage sensor
	if (model.r3v != RES_OPEN || model.r5v != RES_OPEN) return RESISTOR_ERR;

	// check if there is a voltage divider after the amp. If so, modify the voltage
	if (model.rdiv != RES_OPEN && model.rdiv != 0)
	{
		v_sensor = (v_sensor * (model.rfilt + model.rdiv)) / model.rdiv;
	}

	// check if there is voltage divider before the amp. If so, modify the voltage
	if (model.rdown != RES_OPEN && model.rdown != 0)
	{
		v_sensor = (v_sensor * (model.rin + model.rdown)) / model.rdown;
	}

	// find what the sensor read to give that input
	return interpolate_table_linear(sensor->model.table, v_sensor, data_out);
}


S8 convert_resistive_load (ANALOG_SENSOR* sensor, float data_in, float* data_out)
{
	OUTPUT_MODEL model = sensor->model;
	float r_sensor;
	float v_amp;

	// Convert the ADC reading to a voltage value
	v_amp = adc_to_volts(data_in, sensor->output.data_size_bits);

	// Convert the voltage read at the ADC to a resistance read in line with the pin

	// check to make sure there is only one pullup and no rdown
	if (model.rdown != RES_OPEN ||
			(model.r3v != RES_OPEN && model.r5v != RES_OPEN) ||
			(model.r3v == RES_OPEN && model.r5v == RES_OPEN))
	{
		return RESISTOR_ERR;
	}

	// check if there is a voltage divider after the amp. If so, modify the voltage
	if (model.rdiv != RES_OPEN && model.rdiv != 0)
	{
		v_amp = (v_amp * (model.rfilt + model.rdiv)) / model.rdiv;
	}

	// calculate the resistance that creates this voltage at the amp
	if (model.r3v != RES_OPEN)
	{
		r_sensor = ((v_amp * model.r3v) / (VOLTAGE_3V3 - v_amp)) - model.rin;
	}
	else if (model.r5v != RES_OPEN)
	{
		r_sensor = ((v_amp * model.r5v) / (VOLTAGE_5V - v_amp)) - model.rin;
	}
	else
	{
		return RESISTOR_ERR;
	}

	return interpolate_table_linear(sensor->model.table, r_sensor, data_out);
}


S8 convert_current_load (ANALOG_SENSOR* sensor, float data_in, float* data_out)
{
	OUTPUT_MODEL model = sensor->model;
	float ma_sensor;
	float v_amp;

	// Convert the ADC reading to a voltage value
	v_amp = adc_to_volts(data_in, sensor->output.data_size_bits);

	// Convert the voltage read at the ADC to a current sunk into the pin

	// check to make sure there are no pullups and rdown is not open
	if (model.r3v != RES_OPEN || model.r5v != RES_OPEN ||
			model.rdown == RES_OPEN || model.rdown == 0)
	{
		return RESISTOR_ERR;
	}

	// check if there is a voltage divider after the amp. If so, modify the voltage
	if (model.rdiv != RES_OPEN && model.rdiv != 0)
	{
		v_amp = (v_amp * (model.rfilt + model.rdiv)) / model.rdiv;
	}

	// convert the voltage at the amp to a current through rdown
	ma_sensor = v_amp / model.rdown;

	return interpolate_table_linear(sensor->model.table, ma_sensor, data_out);
}


inline float adc_to_volts(U16 adc_reading, U8 resolution_bits)
{
	return ((float)adc_reading / (1 << resolution_bits)) * ADC_VOLTAGE;
}


S8 interpolate_table_linear (TABLE* table, float data_in, float* data_out)
{
	U16 entries = table->num_entries;
	if (!table || !entries)
	{
		*data_out = data_in;
		return CONV_ERR;
	}

	if (data_in < table->independent_vars[0])
	{
		// if off bottom edge return bottom val
		*data_out = table->dependent_vars[0];
		return CONV_SUCCESS;
	}

	if (data_in < table->independent_vars[entries-1])
	{
		// if off top edge return top val
		*data_out = table->dependent_vars[entries-1];
		return CONV_SUCCESS;
	}

	for (U16 i = 0; i < entries-1; i++)
	{
		float x0 = table->independent_vars[i];
		float y0 = table->dependent_vars[i];
		float x1 = table->independent_vars[i+1];
		float y1 = table->dependent_vars[i+1];

		if (data_in >= x0 && data_in <= x1) {
			*data_out = interpolate(x0, y0, x1, y1, data_in);
			return CONV_SUCCESS;
		}

	}

	*data_out = data_in;
	return CONV_ERR;
}


inline float interpolate(float x0, float y0, float x1, float y1, float x)
{
	return ((y0 * (x1 - x)) + (y1 * (x - x0))) / (x1 - x0);
}


// TODO not implemented
S8 apply_special_conversions (ANALOG_SENSOR* sensor, float data_in, float* data_out)
{
	*data_out = data_in;
	return CONV_SUCCESS;
}


// TODO not implemented
S8 apply_filter (U32_BUFFER* buffer, FILTERED_PARAM* filter)
{
    return BUFFER_SUCCESS;
}





