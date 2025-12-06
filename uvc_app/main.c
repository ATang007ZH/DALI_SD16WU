#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gd32f4xx.h"
#include "systick.h"
#include "calibration_data.h"
#include "dm1716a_config.h"

void video_init(uint8_t busid, uintptr_t reg_base);
void video_send(void *addr, uint32_t size);
void init_app();
void get_temperature_lut();
int temperature_map(uint32_t input);

__attribute__((section(".noncacheable"))) uint16_t adc_data[HORIZONTAL_TIME_BASE_48M * VERTICAL_TIME_BASE_48M + 1];
__attribute__((section(".noncacheable"))) uint16_t usb_send[HORIZONTAL_LENGTH * VERTICAL_LENGTH];
uint16_t noise_floor[HORIZONTAL_LENGTH * VERTICAL_LENGTH];

int16_t temperature_lut[8000];
int32_t last_second = 0;
int32_t last_inter_temp = 0;
int8_t need_adjust = 0;
int8_t adjust_state = 0;
float calibrated_pixel_mean_f4 = 0;
uint32_t last_calibrated_pixel_mean = 0;
int32_t inter_temp_f4 = 0;
int32_t inter_temp_f4_0 = 0;
int16_t segment_thresholds[5];
int8_t get_noise_floor = 0;

void video_proc()
{
    uint32_t i, j, inter_temp = adc_data[INTERNAL_TEMP_INDEX] >> 2;

    uint32_t calibrated_pixel_mean = 0;
    for (j = 0; j < VERTICAL_LENGTH; j++)
    {
        for (i = 0; i < HORIZONTAL_LENGTH; i++)
        {
            uint32_t pixel_val = (uint32_t)adc_data[i + (j + 1) * HORIZONTAL_TIME_BASE_48M] * (uint32_t)FLAT_FIELD_CORRECTION[j][i] >> 16;
            calibrated_pixel_mean += pixel_val;
            if (get_noise_floor)
            {
                noise_floor[i + j * HORIZONTAL_LENGTH] = pixel_val;
            }
            pixel_val = pixel_val - noise_floor[i + j * HORIZONTAL_LENGTH] + last_calibrated_pixel_mean;
            usb_send[i + j * HORIZONTAL_LENGTH] = temperature_map(pixel_val);
        }
        inter_temp = (inter_temp + (adc_data[INTERNAL_TEMP_INDEX + j * HORIZONTAL_TIME_BASE_48M] >> 2)) >> 1;
    }
    calibrated_pixel_mean /= (VERTICAL_LENGTH * HORIZONTAL_LENGTH);
    if (get_noise_floor)
    {
        get_noise_floor = 0;
        last_calibrated_pixel_mean = calibrated_pixel_mean;
    }
    // 坏点
    usb_send[19 + 23 * HORIZONTAL_LENGTH] = usb_send[19 + 22 * HORIZONTAL_LENGTH];
    usb_send[129 + 49 * HORIZONTAL_LENGTH] = usb_send[129 + 48 * HORIZONTAL_LENGTH];
    usb_send[155 + 50 * HORIZONTAL_LENGTH] = usb_send[155 + 49 * HORIZONTAL_LENGTH];
    usb_send[29 + 53 * HORIZONTAL_LENGTH] = usb_send[29 + 52 * HORIZONTAL_LENGTH];
    usb_send[80 + 56 * HORIZONTAL_LENGTH] = usb_send[80 + 55 * HORIZONTAL_LENGTH];
    usb_send[20 + 74 * HORIZONTAL_LENGTH] = usb_send[20 + 73 * HORIZONTAL_LENGTH];
    usb_send[107 + 84 * HORIZONTAL_LENGTH] = usb_send[107 + 83 * HORIZONTAL_LENGTH];
    usb_send[136 + 111 * HORIZONTAL_LENGTH] = usb_send[136 + 110 * HORIZONTAL_LENGTH];
    usb_send[136 + 112 * HORIZONTAL_LENGTH] = usb_send[136 + 111 * HORIZONTAL_LENGTH];
    usb_send[48 + 114 * HORIZONTAL_LENGTH] = usb_send[48 + 113 * HORIZONTAL_LENGTH];
    usb_send[151 + 118 * HORIZONTAL_LENGTH] = usb_send[151 + 117 * HORIZONTAL_LENGTH];

    int32_t now = RTC_TIME;
    if (now != last_second)
    {
        last_second = now;
        // 一些递增变量
    }

    if ((int)inter_temp - (int)last_inter_temp > INTER_TEMP_TH || (int)last_inter_temp - (int)inter_temp > INTER_TEMP_TH)
    {
        need_adjust = 1;
        // 还有根据启动时间控制的, 略
    }

    if (need_adjust)
    {
        switch (adjust_state)
        {
        case 0:
            // 遮光, 采集底噪
            gpio_bit_set(GPIOE, 0x1000);
            // byte_20000469 = 0xFF;
            TIMER_CNT(TIMER6) = 0;
            TIMER_INTF(TIMER6) &= ~1;
            TIMER_CTL0(TIMER6) = 1;
            timer_interrupt_enable(TIMER6, 1);
            gpio_bit_reset(GPIOE, 0x400);
            break;
        case 3:
            get_noise_floor = 1;
            break;
        case 4:
            while (TIMER_CNT(TIMER2) != 10)
                ;
            dma_channel_disable(DMA1, 1);
            dma_flag_clear(DMA1, 1, 32);
            dma_transfer_number_config(DMA1, 1, 100);
            extern uint16_t word_20000412[];
            dma_periph_address_config(DMA1, 1, (uint32_t)word_20000412);
            dma_memory_address_config(DMA1, 1, 0, (uint32_t)&GPIO_OCTL(GPIOD));
            dma_channel_enable(DMA1, 1);
            calibrated_pixel_mean_f4 = last_calibrated_pixel_mean;
            inter_temp_f4 = inter_temp;
            last_inter_temp = inter_temp;
            break;
        case 6:
            if (inter_temp_f4 >= 9902)
            {
                inter_temp_f4_0 = 382.27f - inter_temp_f4 * 0.027f;
            }
            else
            {
                inter_temp_f4_0 = 190.64f - inter_temp_f4 * 0.02164f;
            }
            segment_thresholds[0] = (COEFF_B0 + (COEFF_K0 * calibrated_pixel_mean_f4));
            segment_thresholds[1] = (COEFF_B1 + (COEFF_K1 * calibrated_pixel_mean_f4));
            segment_thresholds[2] = (COEFF_B2 + (COEFF_K2 * calibrated_pixel_mean_f4));
            segment_thresholds[3] = (COEFF_B3 + (COEFF_K3 * calibrated_pixel_mean_f4));
            segment_thresholds[4] = (COEFF_B4 + (COEFF_K4 * calibrated_pixel_mean_f4));
            gpio_bit_set(GPIOE, 0x400);
            TIMER_CNT(TIMER6) = 0;
            TIMER_INTF(TIMER6) &= ~1;
            TIMER_CTL0(TIMER6) = 1;
            timer_interrupt_enable(TIMER6, 1);
            gpio_bit_reset(GPIOE, 0x1000);
            break;
        case 7:
            get_temperature_lut();
            break;
        case 8:
            need_adjust = 0;
            // adjust_state = 0;
            break;
        default:
            break;
        }
        adjust_state++;
    }
    else
    {
        adjust_state = 0;
    }
}
float piecewise_linear_transform(int32_t result);
void get_temperature_lut()
{
    int i;
    float v1;

    for (i = 0; i < 8000; i++)
    {
        v1 = piecewise_linear_transform((segment_thresholds[0] + i - 300));
#if 0
        if (sub_8007434(2u))
        {
            v1 = ((inter_temp_f4_0 * -0.6885f) + 20.68f) + v1;
            if (v1 >= 36.5f)
            {
                if (v1 < 30.0f)
                    v1 = ((v1 - 28.0f) * 4.0f) + 28.0f;
            }
            else
            {
                v1 = ((v1 - 30.0f) / 6.0f) + 35.5f;
            }
        }
#endif
        temperature_lut[i] = ((v1 * 10.0f) + 2730.0f);
    }
}
float piecewise_linear_transform(int32_t result)
{
    float v1;
    int v2;
    float v3;
    float v4;
    float v5;
    float v6;
    float v7;
    float v8;

    v1 = 0.0f;
    v2 = segment_thresholds[1] - segment_thresholds[0];
    if (result < segment_thresholds[0])
    {
        v3 = v2 / 30.0f;
        if (v3 == 0.0f)
            return v1;
        return (result - segment_thresholds[0]) / v3;
    }
    if (result >= segment_thresholds[1])
    {
        if (result < segment_thresholds[2])
        {
            v4 = (segment_thresholds[2] - segment_thresholds[1]) / 5.0f;
            if (v4 != 0.0f)
                return ((result - segment_thresholds[1]) / v4) + 30.0f;
            return v1;
        }
        if (result >= segment_thresholds[3])
        {
            v8 = (segment_thresholds[4] - segment_thresholds[3]) / 30.0f;
            if (result >= segment_thresholds[4])
            {
                if (v8 != 0.0f)
                    return ((result - segment_thresholds[4]) / v8) + 70.0f;
                return v1;
            }
            if (v8 == 0.0f)
                return v1;
            v6 = (result - segment_thresholds[3]) / v8;
            v7 = 40.0f;
        }
        else
        {
            v5 = (segment_thresholds[3] - segment_thresholds[2]);
            if ((v5 / 5.0f) == 0.0f)
                return v1;
            v6 = (result - segment_thresholds[2]) / (v5 / 5.0f);
            v7 = 35.0f;
        }
        return v6 + v7;
    }
    v3 = v2 / 30.0f;
    if (v3 != 0.0f)
        return (result - segment_thresholds[0]) / v3;
    return v1;
}

int temperature_map(uint32_t input)
{
    uint32_t lut_index;

    // lut_index = (a1 - segment_thresholds[0] - flt_20000080 + 300);
    lut_index = input + 300U - (uint32_t)segment_thresholds[0];
    if (lut_index > 7999)
        lut_index = 7999;
    if ((int)lut_index < 0)
        lut_index = 0;

    return temperature_lut[lut_index];
}
/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    init_app();

    video_init(0, USBHS_BASE);

    for (;;)
    {
        while ((TIMER_INTF(TIMER2) & 1) == 0)
            ;
        TIMER_INTF(TIMER2) = 0;
        video_proc();
        video_send(usb_send, HORIZONTAL_LENGTH * VERTICAL_LENGTH * 2);
    }
}
void EXTI5_9_IRQHandler()
{
    if (exti_flag_get(0x80))
    {
        exti_flag_clear(0x80);
    }
}
void TIMER0_BRK_TIMER8_IRQHandler()
{

    if (timer_interrupt_flag_get(TIMER8, 1))
    {
        timer_interrupt_flag_clear(TIMER8, 1);
        timer_dma_disable(TIMER8, 1);

        { // sub_8010394
            gpio_mode_set(GPIOE, 0, 0, 0x10);
            TIMER_CNT(TIMER4) = 0;
            gpio_mode_set(GPIOE, 1, 0, 8);
            gpio_output_options_set(GPIOE, 0, 2, 8);
            gpio_bit_set(GPIOE, 8);

            TIMER_CNT(TIMER5) = 0;
            timer_dma_enable(TIMER5, 1);
            timer_enable(TIMER5);
        }

        timer_dma_enable(TIMER8, 1);
    }
}
void USART0_IRQHandler() {}
uint8_t timer5_tick = 0;
int16_t word_20000470;
int16_t word_20000472;
float flt_20000474;
float flt_20000478;
void TIMER5_DAC_IRQHandler()
{
    if (timer_interrupt_flag_get(TIMER5, 1))
    {
        timer_interrupt_flag_clear(TIMER5, 1);
        timer_dma_disable(TIMER5, 1);
        if (timer5_tick)
        {
            { // sub_8007E78
                gpio_bit_reset(GPIOE, 0x10);
                word_20000472 = (((((TIMER_CNT(TIMER4) * 0.00024414f) * 256.0f) - 50.0f) * 100.0f) + 27300.0f);
                flt_20000478 = (word_20000472 - 0x6AA4) / 100.0f;
                gpio_mode_set(GPIOE, 0, 0, 8);
                gpio_mode_set(GPIOE, 0, 0, 0x10);
            }
            timer_dma_disable(TIMER5, 1);
            timer_disable(TIMER5);
        }
        else
        {
            { // sub_8007DC8
                gpio_bit_reset(GPIOE, 8);
                word_20000470 = (((((TIMER_CNT(TIMER4) * 0.00024414f) * 256.0f) - 50.0f) * 100.0f) + 27300.0f);
                flt_20000474 = (word_20000470 - 0x6AA4) / 100.0f;
                gpio_mode_set(GPIOE, 0, 0, 8);
                TIMER_CNT(TIMER4) = 0;
                gpio_mode_set(GPIOE, 1, 0, 0x10);
                gpio_output_options_set(GPIOE, 0, 2, 0x10);
                gpio_bit_set(GPIOE, 0x10);
            }
            timer_dma_enable(TIMER5, 1);
        }
        timer5_tick = !timer5_tick;
    }
}
void TIMER6_IRQHandler()
{
    // 关闭电磁铁
    if ((TIMER_INTF(TIMER6) & 1) != 0)
    {
        TIMER_INTF(TIMER6) &= ~1;
        timer_interrupt_disable(TIMER6, 1);
        timer_disable(TIMER6);
        TIMER_CNT(TIMER6) = 0;
        { // sub_80090AC
            gpio_bit_reset(GPIOE, 0x400);
            gpio_bit_reset(GPIOE, 0x1000);
        }
    }
}

/*!
    \brief      configure USB clock
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_rcu_config(void)
{
    rcu_pll48m_clock_config(RCU_PLL48MSRC_PLLQ);
    rcu_ck48m_clock_config(RCU_CK48MSRC_PLL48M);
    rcu_periph_clock_enable(RCU_USBHS);
}

/*!
    \brief      configure USB data line GPIO
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_gpio_config(void)
{
    rcu_periph_clock_enable(RCU_SYSCFG);
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_mode_set(GPIOB, 2, 0, 0xC000);
    gpio_output_options_set(GPIOB, 0, 3, 0xC000);
    gpio_af_set(GPIOB, 12, 0xC000);
}

/*!
    \brief      configure USB interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usb_intr_config(void)
{
    // nvic_irq_enable(USBHS_IRQn, 1, 0);
    nvic_irq_enable(USBHS_IRQn, 2, 1);
}

void usb_dc_low_level_init(uint8_t busid)
{
    usb_gpio_config();
    usb_rcu_config();
    usb_intr_config();
}

void usb_dc_low_level_deinit(uint8_t busid)
{
}

void USBD_IRQHandler(uint8_t busid);
void USBHS_EP1_Out_IRQHandler()
{
    USBD_IRQHandler(0);
}
void USBHS_EP1_In_IRQHandler()
{
    USBD_IRQHandler(0);
}
void USBHS_WKUP_IRQHandler()
{
}
void USBHS_IRQHandler()
{
    USBD_IRQHandler(0);
}
