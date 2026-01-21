#include "power_sensor.h"
#include "stm32f0xx_hal_adc.h"
#include "stm32f0xx_hal_def.h"
#include "uart_router.h"
#include "ui_mgmt_link.h"
#include <stdint.h>

extern ADC_HandleTypeDef hadc;
#define USB_CC1_DETECT 6
#define USB_CC2_DETECT 7
#define CHARGE_FEEDBACK 8
#define BATTERY_MON 9

static void change_adc_channel(const uint32_t channel)
{
    ADC_ChannelConfTypeDef config = {0};
    config.Channel = channel;
    config.Rank = ADC_RANK_CHANNEL_NUMBER;
    config.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    HAL_ADC_ConfigChannel(&hadc, &config);
}

static uint16_t read_adc_channel(const uint32_t channel)
{
    change_adc_channel(channel);
    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);

    return HAL_ADC_GetValue(&hadc);
}

static void send_usb_detect_to_ui(const uint8_t detect)
{
    uart_stream_t* stream = uart_router_get_ui_stream();
    // Write to the ui that the usb was detected
    uart_router_copy_byte_to_tx(&stream->tx, Sensor_Info);
    uart_router_copy_byte_to_tx(&stream->tx, 2);
    uart_router_copy_byte_to_tx(&stream->tx, 0);
    uart_router_copy_byte_to_tx(&stream->tx, 0);
    uart_router_copy_byte_to_tx(&stream->tx, 0);
    uart_router_copy_byte_to_tx(&stream->tx, Usb_Detect);
    uart_router_copy_byte_to_tx(&stream->tx, detect);
}

void power_sensor_detect_usb(power_info_t* info)
{
    // TODO normalize the values from the adc, because they will not be perfectly the same nor zero
    uint16_t cc1 = read_adc_channel(USB_CC1_DETECT);
    uint16_t cc2 = read_adc_channel(USB_CC2_DETECT);

    uint16_t selected;

    if (cc1 > cc2)
    {
        selected = cc1;
    }
    else if (cc1 <= cc2)
    {
        selected = cc2;
    }
    else
    {
        // Not connected
    }
}

void power_sensor_detect_charging(power_info_t* info)
{
}

void power_sensor_measure_charge(power_info_t* info)
{
}
