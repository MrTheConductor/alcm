/*
 * Copyright (c) 2024-2025, Mitchell White <mitchell.n.white@gmail.com>
 *
 * This file is part of Advanced LCM (ALCM) project.
 *
 * ALCM is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ALCM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with ALCM. If not, see <https://www.gnu.org/licenses/>.
 */
#include "footpads_hw.h"
#include "hk32f030m.h"

#define SCALING_FACTOR 0.0012890625f

// Function to calibrate the ADC for footpads hardware
void footpads_hw_calibrate()
{
    ADC_DeInit(ADC1);
    ADC_GetCalibrationFactor(ADC1);
    ADC_Cmd(ADC1, ENABLE);
}

// Function to initialize the footpads hardware
void footpads_hw_init()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC, ENABLE);

    // Configure PD3 and PC4 as analog input
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource3, GPIO_AF_7);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_7);

    ADC_StructInit(&ADC_InitStructure);
    ADC_Init(ADC1, &ADC_InitStructure);

    footpads_hw_calibrate();
}

// Function to read ADC value from a specific channel
uint16_t read_adc(uint8_t channel)
{
    uint16_t adc_value = 0;

    ADC_ChannelConfig(ADC1, channel, ADC_SampleTime_239_5Cycles);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY))
        ;
    ADC_StartOfConversion(ADC1);

    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
        ;
    adc_value = ADC_GetConversionValue(ADC1);
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    return adc_value;
}

// Function to get the left footpad value
float footpads_hw_get_left()
{
    return (float)(read_adc(ADC_Channel_2) * SCALING_FACTOR);
}

// Function to get the right footpad value
float footpads_hw_get_right()
{
    return (float)(read_adc(ADC_Channel_3) * SCALING_FACTOR);
}