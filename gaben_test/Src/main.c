/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Read a potentiometer on PA0 and print resistance on USART2.
 ******************************************************************************
 */

#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x40010800UL
#define ADC1_BASE       0x40012400UL
#define USART2_BASE     0x40004400UL

#define RCC_CR          REG32(RCC_BASE + 0x00UL)
#define RCC_CFGR        REG32(RCC_BASE + 0x04UL)
#define RCC_APB2ENR     REG32(RCC_BASE + 0x18UL)
#define RCC_APB1ENR     REG32(RCC_BASE + 0x1CUL)

#define GPIOA_CRL       REG32(GPIOA_BASE + 0x00UL)
#define GPIOA_CRH       REG32(GPIOA_BASE + 0x04UL)

#define ADC1_SR         REG32(ADC1_BASE + 0x00UL)
#define ADC1_SMPR2      REG32(ADC1_BASE + 0x10UL)
#define ADC1_SQR1       REG32(ADC1_BASE + 0x2CUL)
#define ADC1_SQR3       REG32(ADC1_BASE + 0x34UL)
#define ADC1_DR         REG32(ADC1_BASE + 0x4CUL)
#define ADC1_CR2        REG32(ADC1_BASE + 0x08UL)

#define USART2_SR       REG32(USART2_BASE + 0x00UL)
#define USART2_DR       REG32(USART2_BASE + 0x04UL)
#define USART2_BRR      REG32(USART2_BASE + 0x08UL)
#define USART2_CR1      REG32(USART2_BASE + 0x0CUL)

#define RCC_CR_HSION        (1UL << 0)
#define RCC_CR_HSIRDY       (1UL << 1)
#define RCC_APB2ENR_AFIOEN  (1UL << 0)
#define RCC_APB2ENR_IOPAEN  (1UL << 2)
#define RCC_APB2ENR_ADC1EN  (1UL << 9)
#define RCC_APB1ENR_USART2EN (1UL << 17)

#define ADC_SR_EOC          (1UL << 1)
#define ADC_CR2_ADON        (1UL << 0)
#define ADC_CR2_CAL         (1UL << 2)
#define ADC_CR2_RSTCAL      (1UL << 3)

#define USART_SR_TXE        (1UL << 7)
#define USART_CR1_TE        (1UL << 3)
#define USART_CR1_UE        (1UL << 13)

#define SYSTEM_CLOCK_HZ     8000000UL
#define USART_BAUD_RATE     9600UL
#define ADC_MAX_VALUE       4095UL
#define ADC_VREF_MV         3300UL
#define POT_TOTAL_OHMS      5000UL

void SystemInit(void)
{
    RCC_CR |= RCC_CR_HSION;
    while ((RCC_CR & RCC_CR_HSIRDY) == 0U) {
    }

    RCC_CFGR = 0x00000000UL;
}

static void delay_ms(uint32_t ms)
{
    while (ms-- != 0U) {
        for (volatile uint32_t i = 0; i < 1000U; ++i) {
            __asm volatile ("nop");
        }
    }
}

static void serial_write_char(char ch)
{
    while ((USART2_SR & USART_SR_TXE) == 0U) {
    }
    USART2_DR = (uint32_t)ch;
}

static void serial_write_string(const char *text)
{
    while (*text != '\0') {
        serial_write_char(*text++);
    }
}

static void serial_write_u32(uint32_t value)
{
    char digits[10];
    uint32_t index = 0U;

    if (value == 0U) {
        serial_write_char('0');
        return;
    }

    while (value != 0U) {
        digits[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (index != 0U) {
        serial_write_char(digits[--index]);
    }
}

static void serial_write_millivolts(uint32_t millivolts)
{
    serial_write_u32(millivolts / 1000U);
    serial_write_char('.');
    serial_write_char((char)('0' + ((millivolts / 100U) % 10U)));
    serial_write_char((char)('0' + ((millivolts / 10U) % 10U)));
    serial_write_char((char)('0' + (millivolts % 10U)));
}

static void serial_write_tenths(uint32_t tenths)
{
    serial_write_u32(tenths / 10U);
    serial_write_char('.');
    serial_write_char((char)('0' + (tenths % 10U)));
}

static void serial_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIOA_CRL &= ~(0xFUL << 8);
    GPIOA_CRL |=  (0xAUL << 8); /* PA2: USART2_TX, alternate function push-pull. */

    USART2_BRR = ((SYSTEM_CLOCK_HZ + (USART_BAUD_RATE / 2U)) / USART_BAUD_RATE);
    USART2_CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void adc1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    GPIOA_CRL &= ~(0xFUL << 0);  /* PA0: analog input, ADC1_IN0. */
    ADC1_SMPR2 = (ADC1_SMPR2 & ~(0x7UL << 0)) | (0x5UL << 0);
    ADC1_SQR1 = 0U;
    ADC1_SQR3 = 0U;

    ADC1_CR2 |= ADC_CR2_ADON;
    delay_ms(1U);

    ADC1_CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1_CR2 & ADC_CR2_RSTCAL) != 0U) {
    }

    ADC1_CR2 |= ADC_CR2_CAL;
    while ((ADC1_CR2 & ADC_CR2_CAL) != 0U) {
    }
}

static uint16_t adc1_read_channel0(void)
{
    ADC1_CR2 |= ADC_CR2_ADON;
    while ((ADC1_SR & ADC_SR_EOC) == 0U) {
    }

    return (uint16_t)(ADC1_DR & 0x0FFFU);
}

int main(void)
{
    serial_init();
    adc1_init();

    serial_write_string("Potentiometer monitor started\r\n");
    serial_write_string("PA0=ADC1_IN0, USART2_TX=PA2, baud=9600\r\n");
    serial_write_string("Vref=3.300 V, Pot=5000 ohm\r\n");

    for (;;) {
        uint32_t adc_value = adc1_read_channel0();
        uint32_t voltage_mv = ((adc_value * ADC_VREF_MV) + (ADC_MAX_VALUE / 2U)) / ADC_MAX_VALUE;
        uint32_t position_tenths = ((adc_value * 1000U) + (ADC_MAX_VALUE / 2U)) / ADC_MAX_VALUE;
        uint32_t resistance_ohms = ((adc_value * POT_TOTAL_OHMS) + (ADC_MAX_VALUE / 2U)) / ADC_MAX_VALUE;

        serial_write_string("ADC=");
        serial_write_u32(adc_value);
        serial_write_string(", Voltage=");
        serial_write_millivolts(voltage_mv);
        serial_write_string(" V, Position=");
        serial_write_tenths(position_tenths);
        serial_write_string(" %, Resistance=");
        serial_write_u32(resistance_ohms);
        serial_write_string(" ohm\r\n");

        delay_ms(500U);
    }
}
