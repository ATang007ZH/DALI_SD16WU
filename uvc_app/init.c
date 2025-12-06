#include <stdio.h>
#include <stdlib.h>
#include "gd32f4xx.h"
#include "systick.h"
#include "dm1716a_config.h"
extern uint16_t adc_data[HORIZONTAL_TIME_BASE_48M * VERTICAL_TIME_BASE_48M + 1];
void delay(int a1)
{
    SysTick->LOAD = 25 * a1;
    SysTick->VAL = 0;
    SysTick->CTRL = 1;

    while ((SysTick->CTRL & 1) != 0 && (SysTick->CTRL & 0x10000) == 0)
        ;

    SysTick->CTRL = 0;
    SysTick->VAL = 0;
}
int delay_0(int result)
{
    SysTick->LOAD = 25000;
    SysTick->VAL = 0;
    SysTick->CTRL = 1;
    do
    {
        while ((SysTick->CTRL & 1) != 0 && (SysTick->CTRL & 0x10000) == 0)
            ;
    } while (result-- != 0);
    SysTick->CTRL = 0;
    SysTick->VAL = 0;
    return result;
}

const uint16_t word_20000410 = 0xD000;

const uint16_t word_20000412[] =
    {
        0,
        BIT(13),
        BIT(13),
        0,
        BIT(13),
        0,
        BIT(13),
        BIT(13),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        BIT(13),
        0,
        0,
        BIT(13),
        BIT(13),
        BIT(13),
        BIT(13),
        BIT(13),
        BIT(13),
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
};

void tpc112s4_send(unsigned int a1)
{
    gpio_bit_set(GPIOC, 0x800);
    delay(10);
    gpio_bit_reset(GPIOC, 0x800);
    delay(10);

    spi_i2s_data_transmit(SPI2, a1 >> 8);
    while (!spi_i2s_flag_get(SPI2, 2))
        ;
    spi_i2s_data_transmit(SPI2, a1);
    while (!spi_i2s_flag_get(SPI2, 2))
        ;

    delay(10);
    gpio_bit_set(GPIOC, 0x800);
}

void adjust_dac(int channel, int val)
{
    int v2;
    v2 = (channel << 0xC) + val;
    tpc112s4_send(v2);
}

void SystemReinit()
{
    int v0;
    int result;
    unsigned int i;

    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
    RCU_CTL |= 1u;
    RCU_CFG0 |= 0x80u;
    for (i = 0; i < 0xC350; ++i)
        ;
    RCU_CFG0 = 0;
    RCU_CTL &= 0xFEF6FFFF;
    RCU_PLL = 0x24003010;
    RCU_CTL &= ~0x40000u;
    RCU_INT = 0;
    v0 = 0;
    RCU_CTL |= 0x10000u;
    do
        ++v0;
    while ((RCU_CTL & 0x20000) == 0 && v0 != 0xFFFF);
    if ((RCU_CTL & 0x20000) == 0)
    {
        while (1)
            ;
    }
    RCU_APB1EN |= 0x10000000u;
    PMU_CTL |= 0xC000u;
    RCU_CFG0 |= 0x8000u;
    RCU_CFG0 |= 0x1400u;
    RCU_PLL = 0x8406019;
    RCU_CTL |= 0x1000000u;
    while ((RCU_CTL & 0x2000000) == 0)
        ;
    PMU_CTL |= 0x10000u;
    while ((PMU_CS & 0x10000) == 0)
        ;
    PMU_CTL |= 0x20000u;
    while ((PMU_CS & 0x20000) == 0)
        ;
    RCU_CFG0 &= 0xFFFFFFFC;
    RCU_CFG0 |= 2u;
    do
        result = RCU_CFG0;
    while ((RCU_CFG0 & 8) == 0);
}

void init_app()
{
    SystemReinit();
    systick_config();
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    systick_clksource_set(SYSTICK_CLKSOURCE_HCLK_DIV8);

    {         // sub_80072A0
        {     // sub_8009C18
            { // sub_8009B2C
                rcu_periph_clock_enable(RCU_GPIOA);
                rcu_periph_clock_enable(RCU_GPIOB);
                rcu_periph_clock_enable(RCU_GPIOC);
                rcu_periph_clock_enable(RCU_GPIOD);
                rcu_periph_clock_enable(RCU_GPIOE);
                gpio_mode_set(GPIOA, 2, 0, 0x100);
                gpio_output_options_set(GPIOA, 0, 2, 0x100);
                gpio_af_set(GPIOA, 1, 0x100);
                gpio_mode_set(GPIOB, 2, 0, 1);
                gpio_output_options_set(GPIOB, 0, 2, 1);
                gpio_af_set(GPIOB, 1, 1);
                gpio_mode_set(GPIOB, 2, 0, 0x100);
                gpio_output_options_set(GPIOB, 0, 2, 0x100);
                gpio_af_set(GPIOB, 1, 0x100);
                gpio_mode_set(GPIOC, 2, 0, 0x200);
                gpio_output_options_set(GPIOC, 0, 2, 0x200);
                gpio_af_set(GPIOC, 2, 0x200);
                gpio_mode_set(GPIOD, 1, 2, 0xFFFF);
                gpio_output_options_set(GPIOD, 0, 2, 0xFFFF);
            }
            { // sub_8004A08
              // size_mode
            }
            { // sub_8009928
                timer_parameter_struct v2 = {0};
                timer_oc_parameter_struct v3 = {0};
                int a1 = PIXEL_INTERVAL_BASE_96M;

                rcu_periph_clock_enable(RCU_TIMER7);
                timer_deinit(TIMER7);

                v2.period = a1 - 1;
                timer_init(TIMER7, &v2);
                v3.outputstate = 1;
                v3.ocpolarity = 2;
                timer_channel_output_config(TIMER7, 0, &v3);
                timer_channel_output_pulse_value_config(TIMER7, 0, a1 >> 1);
                timer_channel_output_mode_config(TIMER7, 0, 0x70);
                timer_channel_output_shadow_config(TIMER7, 0, 8);
                timer_auto_reload_shadow_enable(TIMER7);
                timer_input_trigger_source_select(TIMER7, 0);
                timer_master_output_trigger_source_select(TIMER7, 0x20);
                timer_slave_mode_select(TIMER7, 6);
                timer_dma_transfer_config(TIMER7, 9, 0);
                timer_dma_enable(TIMER7, TIMER7 >> 0x16);
                { // sub_8008B80
                    dma_single_data_parameter_struct v0 = {0};

                    rcu_periph_clock_enable(RCU_DMA1);
                    dma_deinit(DMA1, 1u);
                    v0.direction = 0x40;
                    v0.memory0_addr = (uint32_t)word_20000412;
                    v0.memory_inc = 0;
                    v0.periph_memory_width = DMA1 >> 0x13;
                    v0.number = 0x20;
                    v0.periph_addr = (uint32_t)&GPIO_OCTL(GPIOD);
                    v0.periph_inc = 1;
                    v0.priority = 0x30000;
                    v0.circular_mode = 1;
                    dma_single_data_mode_init(DMA1, 1, &v0);
                    dma_channel_subperipheral_select(DMA1, 1, 7);
                    dma_channel_disable(DMA1, 1);
                }
            }
            { // sub_8009508
                timer_parameter_struct v2 = {0};
                timer_oc_parameter_struct v3 = {0};
                int a1 = PIXEL_INTERVAL_BASE_96M;

                rcu_periph_clock_enable(RCU_TIMER0);
                timer_deinit(TIMER0);

                v2.period = a1 - 1;
                timer_init(TIMER0, &v2);
                v3.outputstate = 1;
                v3.ocpolarity = 2;
                timer_channel_output_config(TIMER0, 0, &v3);
                timer_channel_output_pulse_value_config(TIMER0, 0, a1 >> 1);
                timer_channel_output_mode_config(TIMER0, 0, 0x70);
                timer_channel_output_shadow_config(TIMER0, 0, 8);
                v3.outputnstate = 4;
                v3.ocnpolarity = 8;
                timer_channel_output_config(TIMER0, 1, &v3);
                timer_channel_output_pulse_value_config(TIMER0, 1, ((3 * v2.period) >> 2) & 0x1FFFFFFF);
                timer_channel_output_mode_config(TIMER0, 1, 0x70);
                timer_channel_output_shadow_config(TIMER0, 1, 8);
                timer_auto_reload_shadow_enable(TIMER0);
                timer_master_output_trigger_source_select(TIMER0, 0x20);
                timer_master_slave_mode_config(TIMER0, 0);
                timer_primary_output_config(TIMER0, 1);
                timer_dma_transfer_config(TIMER0, 0xD, 0);
                timer_dma_disable(TIMER0, TIMER0 >> 0x16);
                { // sub_8009600
                    dma_single_data_parameter_struct v0 = {0};

                    rcu_periph_clock_enable(RCU_DMA1);
                    dma_deinit(DMA1, 5u);
                    v0.direction = 0x40;
                    v0.memory0_addr = (uint32_t)word_20000410;
                    v0.memory_inc = 1;
                    v0.periph_memory_width = 0x800;
                    v0.circular_mode = 0;
                    v0.periph_addr = (uint32_t)&SPI_DATA(SPI3);
                    v0.periph_inc = 1;
                    v0.number = 1;
                    v0.priority = 0;
                    dma_single_data_mode_init(DMA1, 5, &v0);
                    dma_circulation_enable(DMA1, 5);
                    dma_channel_subperipheral_select(DMA1, 5, 6);
                    dma_flag_clear(DMA1, 5u, 0x20);
                    dma_channel_disable(DMA1, 5);
                }
                timer_disable(TIMER0);
            }
            { // sub_8009684
                timer_parameter_struct v2 = {0};
                timer_oc_parameter_struct v3 = {0};
                int a1 = HORIZONTAL_TIME_BASE_48M;

                rcu_periph_clock_enable(RCU_TIMER1);
                timer_deinit(TIMER1);

                v2.period = a1 - 1;
                timer_init(TIMER1, &v2);
                v3.outputstate = 1;
                v3.ocpolarity = 2;
                timer_channel_output_config(TIMER1, 0, &v3);
                timer_channel_output_pulse_value_config(TIMER1, 0, v2.period - 0x1B);
                timer_channel_output_mode_config(TIMER1, 0, 0x60);
                timer_channel_output_shadow_config(TIMER1, 0, 8);
                timer_auto_reload_shadow_enable(TIMER1);
                timer_input_trigger_source_select(TIMER1, 0);
                timer_slave_mode_select(TIMER1, 7);
                timer_enable(TIMER1);
            }
            { // sub_8009730
                timer_parameter_struct v3 = {0};
                timer_oc_parameter_struct v4 = {0};
                int a1 = (HORIZONTAL_TIME_BASE_48M * VERTICAL_TIME_BASE_48M) & 0xffff;
                rcu_periph_clock_enable(RCU_TIMER2);
                timer_deinit(TIMER2);

                v3.period = a1 - 1;
                timer_init(TIMER2, &v3);
                v4.outputstate = 1;
                v4.ocpolarity = 2;
                timer_channel_output_config(TIMER2, 3, &v4);
                timer_channel_output_pulse_value_config(TIMER2, 3, v3.period - 2);
                timer_channel_output_mode_config(TIMER2, 3, 0x60);
                timer_channel_output_shadow_config(TIMER2, 3, 8);
                timer_auto_reload_shadow_enable(TIMER2);
                timer_input_trigger_source_select(TIMER2, 0);
                timer_slave_mode_select(TIMER2, 7);
                timer_enable(TIMER2);
                TIMER_CNT(TIMER2) = 0;
            }
            timer_enable(TIMER0);
        }
        {     // sub_8004AB8
            { // sub_80064CC
                rcu_periph_clock_enable(RCU_GPIOC);
                gpio_mode_set(GPIOC, 1, 1, 0x800);
                gpio_output_options_set(GPIOC, 0, 1, GPIOC >> 0x13);
                gpio_af_set(GPIOC, 6, 0x1400);
                gpio_mode_set(GPIOC, 2, 0, 0x1400);
                gpio_output_options_set(GPIOC, 0, 1, 0x1400);
            }
            { //   sub_8008640
                spi_parameter_struct v0 = {0};
                rcu_periph_clock_enable(RCU_SPI2);
                v0.device_mode = 0x104;
                v0.trans_mode = 0;
                v0.clock_polarity_phase = 1;
                v0.frame_size = 0;
                v0.nss = 0x200;
                v0.prescale = 0x20;
                v0.endian = 0;
                spi_init(SPI2, &v0);
                // spi_crc_en(SPI2, 7); // TODO
                spi_crc_on(SPI2);
                spi_crc_polynomial_set(SPI2, 7);
                spi_enable(SPI2);
            }

            adjust_dac(0xc, (int)(0x53f * 1.515f)); // 3
            delay(500);
            adjust_dac(0xe, (int)(0x384 * 1.515f)); // 4
            delay(500);
            adjust_dac(0x8, (int)(0x8f7 * 1.515f)); // 2
            delay(500);
            adjust_dac(0xa, (int)(0x3e7 * 1.515f)); // 1
            delay(500);

            TIMER_CH0CV(TIMER1) = TIMER_CAR(TIMER1) - 30;
        }
        {     // sub_8005640
            { // sub_800648C
                rcu_periph_clock_enable(RCU_GPIOE);
                rcu_periph_clock_enable(RCU_SPI3);
                gpio_af_set(GPIOE, 5, 0x64); // SPI3_MISO_PE5 SPI3_MOSI_PE6 SPI3_SCK_PE2
                gpio_mode_set(GPIOE, 2, 0, 0x64);
                gpio_output_options_set(GPIOE, 0, 1, 0x64);
            }
            { // sub_8008688
                spi_parameter_struct v0 = {0};

                rcu_periph_clock_enable(RCU_SPI3);
                v0.device_mode = 0x104;
                v0.trans_mode = 0;
                v0.frame_size = 0x800;
                v0.nss = 0x200;

                spi_init(SPI3, &v0);
                spi_crc_on(SPI3);
                spi_crc_polynomial_set(SPI3, 7);
                spi_enable(SPI3);
                spi_dma_enable(SPI3, 0);
                spi_dma_enable(SPI3, 1);
            }
            TIMER_INTF(TIMER0) = 0;
            while ((TIMER_INTF(TIMER0) & 1) == 0)
                ;
            TIMER_INTF(TIMER0) = 0;
            spi_i2s_data_transmit(SPI3, 0xEFFC);
            while (!spi_i2s_flag_get(SPI3, 2))
                ;
            delay_0(2);
            { // sub_80086E0
                dma_single_data_parameter_struct v4 = {0};

                rcu_periph_clock_enable(RCU_DMA1);
                dma_deinit(DMA1, 0);
                v4.direction = 0;
                v4.memory_inc = 0;
                v4.periph_memory_width = 0x800;
                v4.periph_addr = (uint32_t)&SPI_DATA(SPI3);
                v4.periph_inc = 1;
                v4.memory0_addr = (uint32_t)adc_data;
                v4.circular_mode = 0;
                v4.number = 0x5515;
                v4.priority = 0x20000;
                dma_single_data_mode_init(DMA1, 0, &v4);
                dma_circulation_enable(DMA1, 0);
                dma_channel_subperipheral_select(DMA1, 0, 4);
                dma_channel_enable(DMA1, 0);
            }
        }
        { // sub_8007334
            int j = 0;
            while (TIMER_CNT(TIMER2) != 0xA)
                ;
            for (; j < sizeof(word_20000412) / sizeof(word_20000412[0]); j++)
            {
                while ((TIMER_INTF(TIMER0) & 1) == 0)
                    ;
                if (word_20000412[j] == 0)
                    gpio_bit_reset(GPIOD, BIT(13));
                else
                    gpio_bit_set(GPIOD, BIT(13));
                TIMER_INTF(TIMER0) = 0;
            }
        }
        { // sub_800527C
            TIMER_CNT(TIMER1) = 19;
            TIMER_CNT(TIMER2) = 0;
            timer_dma_enable(TIMER0, 0x100);
            dma_channel_enable(DMA1, 5);
            { // sub_8008748
                dma_channel_disable(DMA1, 0);
                dma_flag_clear(DMA1, 0, 0x20);
                dma_memory_address_config(DMA1, 0, 0, (uint32_t)adc_data);
                dma_transfer_number_config(DMA1, 0, 0x5515);
                dma_channel_enable(DMA1, 0);
            }
        }
        {     // sub_8009090
            { // sub_8009248
                rcu_periph_clock_enable(RCU_GPIOE);
                gpio_mode_set(GPIOE, 1, 0, 0x1400);
                gpio_output_options_set(GPIOE, 0, 2, 0x1400);
            }
            { // sub_80098B4
                timer_parameter_struct v0 = {0};

                rcu_periph_clock_enable(RCU_TIMER6);
                timer_deinit(TIMER6);
                v0.prescaler = 0x500;
                v0.period = 0x95F;
                timer_init(TIMER6, &v0);
                nvic_irq_enable(TIMER6_IRQn, 1, 3);
                timer_auto_reload_shadow_enable(TIMER6);
                timer_flag_clear(TIMER6, 1);
                timer_interrupt_disable(TIMER6, 1);
                timer_enable(TIMER6);
            }
            { // sub_80080EC
                rcu_periph_clock_enable(RCU_PMU);
                pmu_backup_write_enable();
                rcu_osci_on(RCU_IRC32K);
                rcu_osci_stab_wait(RCU_IRC32K);
                rcu_rtc_clock_config(0x200);
                rcu_periph_clock_enable(RCU_RTC);
                rtc_register_sync_wait();
            }
        }
        { // sub_800D690
            rcu_periph_clock_enable(RCU_GPIOE);
            rcu_periph_clock_enable(RCU_SYSCFG);
            rcu_periph_clock_enable(RCU_GPIOH);
            gpio_af_set(GPIOH, 2, 0x800); // TIMER4_CH1_PH11
            gpio_mode_set(GPIOH, 2, 0, 0x800);
            { // sub_80097D8
                timer_parameter_struct v0 = {0};

                rcu_periph_clock_enable(RCU_TIMER4);
                rcu_timer_clock_prescaler_config(0x1000000);
                timer_deinit(TIMER4);
                v0.period = 0xFFFF;
                timer_init(TIMER4, &v0);
                timer_external_trigger_as_external_clock_config(TIMER4, 0x60, 2, 0);
                TIMER_CNT(TIMER4) = 0;
                timer_auto_reload_shadow_enable(TIMER4);
                timer_enable(TIMER4);
            }
            { // sub_8009840
                timer_parameter_struct v0 = {0};

                rcu_periph_clock_enable(RCU_TIMER5);
                rcu_timer_clock_prescaler_config(0x1000000);
                timer_deinit(TIMER5);
                v0.prescaler = 0x4AFF;
                v0.period = 0x40F;
                timer_init(TIMER5, &v0);
                nvic_irq_enable(TIMER5_DAC_IRQn, 3, 1);
                timer_auto_reload_shadow_enable(TIMER5);
                timer_flag_clear(TIMER5, 1);
                timer_dma_disable(TIMER5, 1);
                timer_disable(TIMER5);
            }
            { // sub_80099E0
                timer_parameter_struct v0 = {0};

                rcu_periph_clock_enable(RCU_TIMER8);
                rcu_timer_clock_prescaler_config(0x1000000);
                timer_deinit(TIMER8);
                v0.prescaler = 0x4AFF;
                v0.period = 0x1387;
                timer_init(TIMER8, &v0);
                nvic_irq_enable(TIMER0_BRK_TIMER8_IRQn, 3, 2);
                timer_auto_reload_shadow_enable(TIMER8);
                timer_flag_clear(TIMER8, 1);
                timer_dma_enable(TIMER8, 1);
                timer_enable(TIMER8);
            }
        }
    }

    while ((TIMER_INTF(TIMER2) & 1) == 0)
        ;
    TIMER_INTF(TIMER2) = 0;
}