#include "hk32f030m_adc.h"

// ADC stub implementations - no-ops for simulation

void ADC_DeInit(ADC_TypeDef *ADCx) {
    // No-op
}

void ADC_Init(ADC_TypeDef *ADCx, ADC_InitTypeDef *ADC_InitStruct) {
    // No-op
}

void ADC_StructInit(ADC_InitTypeDef *ADC_InitStruct) {
    ADC_InitStruct->ADC_Resolution = 0;
    ADC_InitStruct->ADC_ContinuousConvMode = 0;
    ADC_InitStruct->ADC_ExternalTrigConvEdge = 0;
    ADC_InitStruct->ADC_ExternalTrigConv = 0;
    ADC_InitStruct->ADC_DataAlign = 0;
    ADC_InitStruct->ADC_ScanDirection = 0;
}

void ADC_Cmd(ADC_TypeDef *ADCx, uint32_t NewState) {
    // No-op
}

void ADC_StartOfConversion(ADC_TypeDef *ADCx) {
    // No-op
}

void ADC_StopOfConversion(ADC_TypeDef *ADCx) {
    // No-op
}

uint32_t ADC_GetFlagStatus(ADC_TypeDef *ADCx, uint32_t ADC_FLAG) {
    return 1; // Always ready
}

void ADC_ClearFlag(ADC_TypeDef *ADCx, uint32_t ADC_FLAG) {
    // No-op
}

uint16_t ADC_GetConversionValue(ADC_TypeDef *ADCx) {
    return 0; // Will be overridden by footpads_hw
}

void ADC_ChannelConfig(ADC_TypeDef *ADCx, uint32_t ADC_Channel, uint32_t ADC_SampleTime) {
    // No-op
}

void ADC_Calibration(ADC_TypeDef *ADCx) {
    // No-op
}
