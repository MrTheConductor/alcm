#ifndef __HK32F030M_ADC_H__
#define __HK32F030M_ADC_H__

#include "hk32f030m.h"

#ifdef __cplusplus
extern "C" {
#endif

// ADC Init structure
typedef struct {
    uint32_t ADC_Resolution;
    uint32_t ADC_ContinuousConvMode;
    uint32_t ADC_ExternalTrigConvEdge;
    uint32_t ADC_ExternalTrigConv;
    uint32_t ADC_DataAlign;
    uint32_t ADC_ScanDirection;
} ADC_InitTypeDef;

// ADC Channels
#define ADC_Channel_0  ((uint32_t)0x00000001)
#define ADC_Channel_1  ((uint32_t)0x00000002)
#define ADC_Channel_2  ((uint32_t)0x00000004)
#define ADC_Channel_3  ((uint32_t)0x00000008)
#define ADC_Channel_4  ((uint32_t)0x00000010)
#define ADC_Channel_5  ((uint32_t)0x00000020)
#define ADC_Channel_6  ((uint32_t)0x00000040)
#define ADC_Channel_7  ((uint32_t)0x00000080)

// ADC Sample Time
#define ADC_SampleTime_1_5Cycles    ((uint32_t)0x00000000)
#define ADC_SampleTime_7_5Cycles    ((uint32_t)0x00000001)
#define ADC_SampleTime_13_5Cycles   ((uint32_t)0x00000002)
#define ADC_SampleTime_28_5Cycles   ((uint32_t)0x00000003)
#define ADC_SampleTime_41_5Cycles   ((uint32_t)0x00000004)
#define ADC_SampleTime_55_5Cycles   ((uint32_t)0x00000005)
#define ADC_SampleTime_71_5Cycles   ((uint32_t)0x00000006)
#define ADC_SampleTime_239_5Cycles  ((uint32_t)0x00000007)

// Function declarations (stubs)
void ADC_DeInit(ADC_TypeDef *ADCx);
void ADC_Init(ADC_TypeDef *ADCx, ADC_InitTypeDef *ADC_InitStruct);
void ADC_StructInit(ADC_InitTypeDef *ADC_InitStruct);
void ADC_Cmd(ADC_TypeDef *ADCx, uint32_t NewState);
void ADC_StartOfConversion(ADC_TypeDef *ADCx);
void ADC_StopOfConversion(ADC_TypeDef *ADCx);
uint32_t ADC_GetFlagStatus(ADC_TypeDef *ADCx, uint32_t ADC_FLAG);
void ADC_ClearFlag(ADC_TypeDef *ADCx, uint32_t ADC_FLAG);
uint16_t ADC_GetConversionValue(ADC_TypeDef *ADCx);
void ADC_ChannelConfig(ADC_TypeDef *ADCx, uint32_t ADC_Channel, uint32_t ADC_SampleTime);
void ADC_Calibration(ADC_TypeDef *ADCx);

#ifdef __cplusplus
}
#endif

#endif /* __HK32F030M_ADC_H__ */
