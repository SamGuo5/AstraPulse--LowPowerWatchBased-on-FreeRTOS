#include "bsp.h"
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;
I2C_HandleTypeDef hi2c4;
RTC_HandleTypeDef hrtc;
ADC_HandleTypeDef hadc1;
static bool battery_adc_ready;
static bool battery_adc_clock_ready;
static bool motion_bus_clock_ready;
static bool motion_bus_ready;
static bool watchdog_ready;
static bool watchdog_reset_detected;

enum {
    BSP_IWDG_KEY_RELOAD=0xaaaau,
    BSP_IWDG_KEY_ENABLE=0xccccu,
    BSP_IWDG_KEY_WRITE_ACCESS=0x5555u,
    BSP_IWDG_PRESCALER_256=6u,
    /* About 8 seconds at the nominal 32 kHz LSI clock. */
    BSP_IWDG_RELOAD_8_SECONDS=999u
};

static void clock_init(void) {
    RCC_OscInitTypeDef osc={0};RCC_ClkInitTypeDef clk={0};
    if(HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1)!=HAL_OK)Error_Handler();
    osc.OscillatorType=RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
    osc.HSIState=RCC_HSI_ON;osc.HSICalibrationValue=RCC_HSICALIBRATION_DEFAULT;
    osc.LSIState=RCC_LSI_ON;osc.LSIDiv=RCC_LSI_DIV1;
    osc.MSIState=RCC_MSI_ON;osc.MSICalibrationValue=RCC_MSICALIBRATION_DEFAULT;osc.MSIClockRange=RCC_MSIRANGE_0;
    osc.PLL.PLLState=RCC_PLL_ON;osc.PLL.PLLSource=RCC_PLLSOURCE_MSI;osc.PLL.PLLMBOOST=RCC_PLLMBOOST_DIV4;
    osc.PLL.PLLM=3;osc.PLL.PLLN=10;osc.PLL.PLLP=2;osc.PLL.PLLQ=2;osc.PLL.PLLR=1;
    osc.PLL.PLLRGE=RCC_PLLVCIRANGE_1;osc.PLL.PLLFRACN=0;
    if(HAL_RCC_OscConfig(&osc)!=HAL_OK)Error_Handler();
    clk.ClockType=RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_PCLK3;
    clk.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK;clk.AHBCLKDivider=RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider=RCC_HCLK_DIV1;clk.APB2CLKDivider=RCC_HCLK_DIV1;clk.APB3CLKDivider=RCC_HCLK_DIV1;
    if(HAL_RCC_ClockConfig(&clk,FLASH_LATENCY_4)!=HAL_OK)Error_Handler();
}

static void gpio_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio={0};
    gpio.Pin=GPIO_PIN_4|GPIO_PIN_5;gpio.Mode=GPIO_MODE_IT_FALLING;gpio.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOA,&gpio);
    gpio.Pin=GPIO_PIN_11|GPIO_PIN_12;HAL_GPIO_Init(GPIOC,&gpio);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);
    gpio.Pin=GPIO_PIN_8;gpio.Mode=GPIO_MODE_OUTPUT_PP;gpio.Pull=GPIO_NOPULL;gpio.Speed=GPIO_SPEED_FREQ_LOW;HAL_GPIO_Init(GPIOA,&gpio);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
    gpio.Pin=GPIO_PIN_12|GPIO_PIN_8;gpio.Mode=GPIO_MODE_OUTPUT_PP;gpio.Pull=GPIO_NOPULL;gpio.Speed=GPIO_SPEED_FREQ_LOW;HAL_GPIO_Init(GPIOB,&gpio);
    HAL_NVIC_SetPriority(EXTI4_IRQn,6,0);HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI5_IRQn,6,0);HAL_NVIC_EnableIRQ(EXTI5_IRQn);
    HAL_NVIC_SetPriority(EXTI11_IRQn,6,0);HAL_NVIC_EnableIRQ(EXTI11_IRQn);
    HAL_NVIC_SetPriority(EXTI12_IRQn,6,0);HAL_NVIC_EnableIRQ(EXTI12_IRQn);
}

void HAL_MspInit(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *handle) {
    RCC_PeriphCLKInitTypeDef clock={0};GPIO_InitTypeDef gpio={0};
    if(handle->Instance==I2C1){
        clock.PeriphClockSelection=RCC_PERIPHCLK_I2C1;clock.I2c1ClockSelection=RCC_I2C1CLKSOURCE_PCLK1;
        if(HAL_RCCEx_PeriphCLKConfig(&clock)!=HAL_OK)Error_Handler();
        __HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_I2C1_CLK_ENABLE();
        gpio.Pin=GPIO_PIN_3|GPIO_PIN_6;gpio.Mode=GPIO_MODE_AF_OD;gpio.Pull=GPIO_PULLUP;
        gpio.Speed=GPIO_SPEED_FREQ_LOW;gpio.Alternate=GPIO_AF4_I2C1;HAL_GPIO_Init(GPIOB,&gpio);
    }else if(handle->Instance==I2C2){
        clock.PeriphClockSelection=RCC_PERIPHCLK_I2C2;clock.I2c2ClockSelection=RCC_I2C2CLKSOURCE_PCLK1;
        if(HAL_RCCEx_PeriphCLKConfig(&clock)!=HAL_OK)Error_Handler();
        __HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_I2C2_CLK_ENABLE();
        gpio.Pin=GPIO_PIN_13|GPIO_PIN_14;gpio.Mode=GPIO_MODE_AF_OD;gpio.Pull=GPIO_PULLUP;
        gpio.Speed=GPIO_SPEED_FREQ_VERY_HIGH;gpio.Alternate=GPIO_AF4_I2C2;HAL_GPIO_Init(GPIOB,&gpio);
    }else if(handle->Instance==I2C3){
        clock.PeriphClockSelection=RCC_PERIPHCLK_I2C3;clock.I2c3ClockSelection=RCC_I2C3CLKSOURCE_PCLK3;
        if(HAL_RCCEx_PeriphCLKConfig(&clock)!=HAL_OK)Error_Handler();
        __HAL_RCC_GPIOC_CLK_ENABLE();__HAL_RCC_I2C3_CLK_ENABLE();
        gpio.Pin=GPIO_PIN_0|GPIO_PIN_1;gpio.Mode=GPIO_MODE_AF_OD;gpio.Pull=GPIO_PULLUP;
        gpio.Speed=GPIO_SPEED_FREQ_HIGH;gpio.Alternate=GPIO_AF4_I2C3;HAL_GPIO_Init(GPIOC,&gpio);
    }else if(handle->Instance==I2C4){
        motion_bus_clock_ready=false;
        clock.PeriphClockSelection=RCC_PERIPHCLK_I2C4;clock.I2c4ClockSelection=RCC_I2C4CLKSOURCE_PCLK1;
        if(HAL_RCCEx_PeriphCLKConfig(&clock)!=HAL_OK)return;
        __HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_I2C4_CLK_ENABLE();
        gpio.Pin=GPIO_PIN_10;gpio.Mode=GPIO_MODE_AF_OD;gpio.Pull=GPIO_PULLUP;
        gpio.Speed=GPIO_SPEED_FREQ_MEDIUM;gpio.Alternate=GPIO_AF3_I2C4;HAL_GPIO_Init(GPIOB,&gpio);
        gpio.Pin=GPIO_PIN_7;gpio.Speed=GPIO_SPEED_FREQ_LOW;gpio.Alternate=GPIO_AF5_I2C4;HAL_GPIO_Init(GPIOB,&gpio);
        motion_bus_clock_ready=true;
    }
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *handle) {
    if(handle->Instance!=RTC)return;
    RCC_PeriphCLKInitTypeDef clock={0};
    HAL_PWR_EnableBkUpAccess();
    clock.PeriphClockSelection=RCC_PERIPHCLK_RTC;
    clock.RTCClockSelection=RCC_RTCCLKSOURCE_LSI;
    if(HAL_RCCEx_PeriphCLKConfig(&clock)!=HAL_OK)Error_Handler();
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    __HAL_RCC_RTCAPB_CLKAM_ENABLE();
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *handle) {
    if(handle->Instance!=ADC1)return;
    RCC_PeriphCLKInitTypeDef clock={0};GPIO_InitTypeDef gpio={0};
    battery_adc_clock_ready=false;
    clock.PeriphClockSelection=RCC_PERIPHCLK_ADCDAC;
    clock.AdcDacClockSelection=RCC_ADCDACCLKSOURCE_HSI;
    if(HAL_RCCEx_PeriphCLKConfig(&clock)!=HAL_OK)return;
    __HAL_RCC_ADC12_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin=GPIO_PIN_0;gpio.Mode=GPIO_MODE_ANALOG;gpio.Pull=GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA,&gpio);
    battery_adc_clock_ready=true;
}

static void adc1_init(void) {
    ADC_ChannelConfTypeDef channel={0};
    battery_adc_ready=false;
    hadc1.Instance=ADC1;hadc1.Init.ClockPrescaler=ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution=ADC_RESOLUTION_12B;hadc1.Init.GainCompensation=0;
    hadc1.Init.ScanConvMode=ADC_SCAN_DISABLE;hadc1.Init.EOCSelection=ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait=DISABLE;hadc1.Init.ContinuousConvMode=DISABLE;
    hadc1.Init.NbrOfConversion=1;hadc1.Init.DiscontinuousConvMode=DISABLE;
    hadc1.Init.ExternalTrigConv=ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests=DISABLE;hadc1.Init.TriggerFrequencyMode=ADC_TRIGGER_FREQ_HIGH;
    hadc1.Init.Overrun=ADC_OVR_DATA_PRESERVED;hadc1.Init.LeftBitShift=ADC_LEFTBITSHIFT_NONE;
    hadc1.Init.ConversionDataManagement=ADC_CONVERSIONDATA_DR;hadc1.Init.OversamplingMode=DISABLE;
    if(HAL_ADC_Init(&hadc1)!=HAL_OK || !battery_adc_clock_ready)return;
    channel.Channel=ADC_CHANNEL_5;channel.Rank=ADC_REGULAR_RANK_1;
    /* A long aperture is inexpensive at 1 Hz and tolerates a high-value divider. */
    channel.SamplingTime=ADC_SAMPLETIME_68CYCLES;channel.SingleDiff=ADC_SINGLE_ENDED;
    channel.OffsetNumber=ADC_OFFSET_NONE;channel.Offset=0;
    if(HAL_ADC_ConfigChannel(&hadc1,&channel)!=HAL_OK)return;
    /* Battery sensing is optional: never hold the whole watch in a boot-time
       fault handler when PA0 is floating or an ADC calibration is unavailable. */
    battery_adc_ready=true;
}

static void rtc_init(void) {
    hrtc.Instance=RTC;hrtc.Init.HourFormat=RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv=127;hrtc.Init.SynchPrediv=255;
    hrtc.Init.OutPut=RTC_OUTPUT_DISABLE;hrtc.Init.OutPutRemap=RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity=RTC_OUTPUT_POLARITY_HIGH;hrtc.Init.OutPutType=RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutPullUp=RTC_OUTPUT_PULLUP_NONE;hrtc.Init.BinMode=RTC_BINARY_NONE;
    if(HAL_RTC_Init(&hrtc)!=HAL_OK)Error_Handler();
    RTC_PrivilegeStateTypeDef privilege={0};
    privilege.rtcPrivilegeFull=RTC_PRIVILEGE_FULL_NO;
    privilege.backupRegisterPrivZone=RTC_PRIVILEGE_BKUP_ZONE_NONE;
    privilege.backupRegisterStartZone2=RTC_BKP_DR0;
    privilege.backupRegisterStartZone3=RTC_BKP_DR0;
    if(HAL_RTCEx_PrivilegeModeSet(&hrtc,&privilege)!=HAL_OK)Error_Handler();
    HAL_NVIC_SetPriority(RTC_IRQn,6,0);HAL_NVIC_EnableIRQ(RTC_IRQn);
}

static void i2c1_init(void) {
    hi2c1.Instance=I2C1;hi2c1.Init.Timing=0x30909DEC;hi2c1.Init.OwnAddress1=0;
    hi2c1.Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;hi2c1.Init.DualAddressMode=I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2=0;hi2c1.Init.OwnAddress2Masks=I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;hi2c1.Init.NoStretchMode=I2C_NOSTRETCH_DISABLE;
    if(HAL_I2C_Init(&hi2c1)!=HAL_OK)Error_Handler();
    if(HAL_I2CEx_ConfigAnalogFilter(&hi2c1,I2C_ANALOGFILTER_ENABLE)!=HAL_OK)Error_Handler();
    if(HAL_I2CEx_ConfigDigitalFilter(&hi2c1,0)!=HAL_OK)Error_Handler();
}

static void i2c2_init(void) {
    hi2c2.Instance=I2C2;hi2c2.Init.Timing=0x00F07BFF;hi2c2.Init.OwnAddress1=0;
    hi2c2.Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;hi2c2.Init.DualAddressMode=I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2=0;hi2c2.Init.OwnAddress2Masks=I2C_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;hi2c2.Init.NoStretchMode=I2C_NOSTRETCH_DISABLE;
    if(HAL_I2C_Init(&hi2c2)!=HAL_OK)Error_Handler();
    if(HAL_I2CEx_ConfigAnalogFilter(&hi2c2,I2C_ANALOGFILTER_ENABLE)!=HAL_OK)Error_Handler();
    if(HAL_I2CEx_ConfigDigitalFilter(&hi2c2,0)!=HAL_OK)Error_Handler();
}

static void i2c3_init(void) {
    hi2c3.Instance=I2C3;hi2c3.Init.Timing=0x30909DEC;hi2c3.Init.OwnAddress1=0;
    hi2c3.Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;hi2c3.Init.DualAddressMode=I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.OwnAddress2=0;hi2c3.Init.OwnAddress2Masks=I2C_OA2_NOMASK;
    hi2c3.Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;hi2c3.Init.NoStretchMode=I2C_NOSTRETCH_DISABLE;
    if(HAL_I2C_Init(&hi2c3)!=HAL_OK)Error_Handler();
    if(HAL_I2CEx_ConfigAnalogFilter(&hi2c3,I2C_ANALOGFILTER_ENABLE)!=HAL_OK)Error_Handler();
    if(HAL_I2CEx_ConfigDigitalFilter(&hi2c3,0)!=HAL_OK)Error_Handler();
}

static bool i2c4_init(void) {
    motion_bus_ready=false;
    hi2c4.Instance=I2C4;hi2c4.Init.Timing=0x30909DEC;hi2c4.Init.OwnAddress1=0;
    hi2c4.Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;hi2c4.Init.DualAddressMode=I2C_DUALADDRESS_DISABLE;
    hi2c4.Init.OwnAddress2=0;hi2c4.Init.OwnAddress2Masks=I2C_OA2_NOMASK;
    hi2c4.Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;hi2c4.Init.NoStretchMode=I2C_NOSTRETCH_DISABLE;
    if(HAL_I2C_Init(&hi2c4)!=HAL_OK||!motion_bus_clock_ready)return false;
    if(HAL_I2CEx_ConfigAnalogFilter(&hi2c4,I2C_ANALOGFILTER_ENABLE)!=HAL_OK)return false;
    if(HAL_I2CEx_ConfigDigitalFilter(&hi2c4,0)!=HAL_OK)return false;
    motion_bus_ready=true;return true;
}

void bsp_init(void) {
    watchdog_reset_detected=(RCC->CSR&RCC_CSR_IWDGRSTF)!=0u;
    __HAL_RCC_CLEAR_RESET_FLAGS();
    HAL_Init();clock_init();gpio_init();rtc_init();adc1_init();i2c1_init();i2c2_init();i2c3_init();(void)i2c4_init();
}
BspKey bsp_key_raw(void) {
    if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_12)==GPIO_PIN_RESET)return BSP_KEY_UP;
    if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_4)==GPIO_PIN_RESET)return BSP_KEY_DOWN;
    if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_5)==GPIO_PIN_RESET)return BSP_KEY_ENTER;
    if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_11)==GPIO_PIN_RESET)return BSP_KEY_BACK;
    return BSP_KEY_NONE;
}
void bsp_led_set(uint8_t on){HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,on?GPIO_PIN_SET:GPIO_PIN_RESET);}
void bsp_buzzer_set(uint8_t on){HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,on?GPIO_PIN_SET:GPIO_PIN_RESET);}
bool bsp_battery_adc_read(uint16_t *raw){
    if(raw==NULL || !battery_adc_ready)return false;
    if(HAL_ADC_Start(&hadc1)!=HAL_OK)return false;
    if(HAL_ADC_PollForConversion(&hadc1,2)!=HAL_OK){HAL_ADC_Stop(&hadc1);return false;}
    *raw=(uint16_t)HAL_ADC_GetValue(&hadc1);
    return HAL_ADC_Stop(&hadc1)==HAL_OK;
}
bool bsp_i2c4_ready(void){return motion_bus_ready;}
bool bsp_i2c4_recover(void){
    motion_bus_ready=false;(void)HAL_I2C_DeInit(&hi2c4);
    return i2c4_init();
}
BspI2c4Diagnostics bsp_i2c4_diagnostics(void){
    BspI2c4Diagnostics diagnostics={0};
    diagnostics.initialized=motion_bus_ready?1u:0u;
    diagnostics.scl_high=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_10)==GPIO_PIN_SET?1u:0u;
    diagnostics.sda_high=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_7)==GPIO_PIN_SET?1u:0u;
    diagnostics.enable_high=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==GPIO_PIN_SET?1u:0u;
    diagnostics.hal_error=hi2c4.Instance==I2C4?HAL_I2C_GetError(&hi2c4):HAL_I2C_ERROR_NONE;
    return diagnostics;
}
bool bsp_watchdog_start(void){
    const uint32_t update_flags=IWDG_SR_PVU|IWDG_SR_RVU|IWDG_SR_WVU|IWDG_SR_EWU;
    IWDG->KR=BSP_IWDG_KEY_ENABLE;
    IWDG->KR=BSP_IWDG_KEY_WRITE_ACCESS;
    IWDG->PR=BSP_IWDG_PRESCALER_256;
    IWDG->RLR=BSP_IWDG_RELOAD_8_SECONDS;
    IWDG->EWCR=IWDG_EWCR_EWIC;
    /* The LSI is asynchronous. Bound the wait so watchdog setup can never
       become a new boot-time lockup. */
    uint32_t timeout=SystemCoreClock/100u;
    while((IWDG->SR&update_flags)!=0u&&timeout>0u)--timeout;
    /* Once 0xCCCC has been written the watchdog cannot be stopped. Even if
       this defensive wait expires, reload it and report it active so the
       monitor never creates a reset loop. */
    IWDG->WINR=IWDG_WINR_WIN;
    IWDG->KR=BSP_IWDG_KEY_RELOAD;
    watchdog_ready=true;
    return true;
}
void bsp_watchdog_refresh(void){if(watchdog_ready)IWDG->KR=BSP_IWDG_KEY_RELOAD;}
bool bsp_was_watchdog_reset(void){return watchdog_reset_detected;}

static uint32_t rtc_milliseconds_of_day(void){
    RTC_TimeTypeDef time={0};RTC_DateTypeDef date={0};
    if(HAL_RTC_GetTime(&hrtc,&time,RTC_FORMAT_BIN)!=HAL_OK||
       HAL_RTC_GetDate(&hrtc,&date,RTC_FORMAT_BIN)!=HAL_OK)return 0u;
    uint32_t fraction=0u;
    if(time.SecondFraction>0u&&time.SubSeconds<=time.SecondFraction){
        fraction=((time.SecondFraction-time.SubSeconds)*1000u)/(time.SecondFraction+1u);
    }
    return ((uint32_t)time.Hours*3600u+(uint32_t)time.Minutes*60u+time.Seconds)*1000u+fraction;
}

uint32_t bsp_stop2_sleep(uint32_t requested_ms){
    if(requested_ms<10u)return 0u;
    if(requested_ms>4000u)requested_ms=4000u;
    /* RTC/16 is nominally 2048 Hz with the 32 kHz LSI. Round up so the
       scheduled wake never occurs earlier than the requested RTOS interval. */
    uint32_t counter=(requested_ms*2048u+999u)/1000u;
    if(counter>0u)--counter;
    if(HAL_RTCEx_SetWakeUpTimer_IT(&hrtc,counter,RTC_WAKEUPCLOCK_RTCCLK_DIV16,0u)!=HAL_OK)return 0u;
    HAL_NVIC_ClearPendingIRQ(RTC_IRQn);
    const uint32_t before=rtc_milliseconds_of_day();
    bsp_watchdog_refresh();
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    /* STOP2 resumes on MSI/HSI. Rebuild the original PLL tree before any task
       or peripheral transaction is allowed to continue. */
    clock_init();
    const uint32_t after=rtc_milliseconds_of_day();
    (void)HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    WRITE_REG(RTC->SCR,RTC_SCR_CWUTF);
    HAL_NVIC_ClearPendingIRQ(RTC_IRQn);
    bsp_watchdog_refresh();
    return after>=before?after-before:(86400000u-before)+after;
}

__weak void bsp_key_wakeup_callback(void){}
void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin){
    if(pin==GPIO_PIN_4||pin==GPIO_PIN_5||pin==GPIO_PIN_11||pin==GPIO_PIN_12)bsp_key_wakeup_callback();
}
void EXTI4_IRQHandler(void){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);}
void EXTI5_IRQHandler(void){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);}
void EXTI11_IRQHandler(void){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);}
void EXTI12_IRQHandler(void){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);}
void RTC_IRQHandler(void){HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);}
