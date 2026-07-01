/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Read a potentiometer with ADC1 and print it over USART2.
 ******************************************************************************
 *
 * Wiring for STM32F103xB:
 *   Potentiometer left/right pins : 3.3 V and GND
 *   Potentiometer center pin      : PA0 / ADC1_IN0
 *   Serial output                 : PA2 / USART2_TX, 9600 8N1
 *   Optional serial RX            : PA3 / USART2_RX
 *
 ******************************************************************************
 */

#include <stdint.h>

#define PERIPH_BASE        0x40000000UL
#define APB1PERIPH_BASE    PERIPH_BASE
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE     (PERIPH_BASE + 0x00020000UL)

#define GPIOA_BASE         (APB2PERIPH_BASE + 0x00000800UL)
#define RCC_BASE           (AHBPERIPH_BASE + 0x00001000UL)
#define ADC1_BASE          (APB2PERIPH_BASE + 0x00002400UL)
#define USART2_BASE        (APB1PERIPH_BASE + 0x00004400UL)
#define FLASH_R_BASE       0x40022000UL

#define REG32(addr)        (*(volatile uint32_t *)(addr))

#define RCC_CR             REG32(RCC_BASE + 0x00UL)
#define RCC_CFGR           REG32(RCC_BASE + 0x04UL)
#define RCC_APB2ENR        REG32(RCC_BASE + 0x18UL)
#define RCC_APB1ENR        REG32(RCC_BASE + 0x1CUL)

#define FLASH_ACR          REG32(FLASH_R_BASE + 0x00UL)

#define GPIOA_CRL          REG32(GPIOA_BASE + 0x00UL)
#define GPIOA_CRH          REG32(GPIOA_BASE + 0x04UL)

#define ADC1_SR            REG32(ADC1_BASE + 0x00UL)
#define ADC1_CR2           REG32(ADC1_BASE + 0x08UL)
#define ADC1_SMPR2         REG32(ADC1_BASE + 0x10UL)
#define ADC1_SQR3          REG32(ADC1_BASE + 0x34UL)
#define ADC1_DR            REG32(ADC1_BASE + 0x4CUL)

#define USART2_SR          REG32(USART2_BASE + 0x00UL)
#define USART2_DR          REG32(USART2_BASE + 0x04UL)
#define USART2_BRR         REG32(USART2_BASE + 0x08UL)
#define USART2_CR1         REG32(USART2_BASE + 0x0CUL)

#define RCC_CR_HSION       (1UL << 0)
#define RCC_CR_HSIRDY      (1UL << 1)
#define RCC_CR_PLLON       (1UL << 24)
#define RCC_CR_PLLRDY      (1UL << 25)

#define RCC_APB2ENR_IOPAEN (1UL << 2)
#define RCC_APB2ENR_AFIOEN (1UL << 0)
#define RCC_APB2ENR_ADC1EN (1UL << 9)
#define RCC_APB1ENR_USART2EN (1UL << 17)

#define ADC_CR2_ADON       (1UL << 0)
#define ADC_CR2_CAL        (1UL << 2)
#define ADC_CR2_RSTCAL     (1UL << 3)
#define ADC_CR2_EXTTRIG    (1UL << 20)
#define ADC_CR2_SWSTART    (1UL << 22)
#define ADC_SR_EOC         (1UL << 1)

#define USART_SR_TXE       (1UL << 7)
#define USART_CR1_TE       (1UL << 3)
#define USART_CR1_UE       (1UL << 13)

#define CPU_CLOCK_HZ       36000000UL
#define USART_BAUD         9600UL
#define ADC_MAX_COUNTS     4095UL
#define VREF_MV            3300UL

void SystemInit(void);

static void delay_cycles(volatile uint32_t cycles);
static void gpio_init(void);
static void adc1_init(void);
static uint16_t adc1_read_channel0(void);
static void usart2_init(void);
static void usart2_write_char(char ch);
static void usart2_write_string(const char *text);
static void usart2_write_u32(uint32_t value);

void SystemInit(void)
{
    RCC_CR |= RCC_CR_HSION;
    while ((RCC_CR & RCC_CR_HSIRDY) == 0U) {
    }

    FLASH_ACR = 0x12U;              /* Prefetch on, 2 wait states. */
    RCC_CFGR = 0x001C0000U;         /* HSI/2 * 9 = 36 MHz, no external crystal. */

    /*
     * Keep the code robust on boards that start from internal HSI only.
     * SYSCLK becomes 36 MHz with HSI/2 * 9; peripherals are configured from
     * the actual APB clocks below.
     */
    RCC_CR |= RCC_CR_PLLON;
    while ((RCC_CR & RCC_CR_PLLRDY) == 0U) {
    }

    RCC_CFGR = (RCC_CFGR & ~0x3UL) | 0x2UL;
    while (((RCC_CFGR >> 2U) & 0x3UL) != 0x2UL) {
    }
}

int main(void)
{
    gpio_init();
    usart2_init();
    adc1_init();

    usart2_write_string("\r\nSTM32 ADC UART ready\r\n");
    usart2_write_string("PA0 -> ADC1_IN0, PA2 -> USART2_TX 9600\r\n");

    for (;;) {
        const uint16_t adc_value = adc1_read_channel0();
        const uint32_t voltage_mv = ((uint32_t)adc_value * VREF_MV) / ADC_MAX_COUNTS;

        usart2_write_string("ADC=");
        usart2_write_u32(adc_value);
        usart2_write_string(", Voltage=");
        usart2_write_u32(voltage_mv / 1000U);
        usart2_write_char('.');
        usart2_write_char((char)('0' + ((voltage_mv / 100U) % 10U)));
        usart2_write_char((char)('0' + ((voltage_mv / 10U) % 10U)));
        usart2_write_char((char)('0' + (voltage_mv % 10U)));
        usart2_write_string("V\r\n");

        delay_cycles(CPU_CLOCK_HZ / 20U);
    }
}

static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles-- != 0U) {
        __asm volatile ("nop");
    }
}

static void gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;

    /* PA0: analog input, CNF0=00 MODE0=00. */
    GPIOA_CRL &= ~(0xFUL << 0U);

    /* PA2: alternate function push-pull, 50 MHz, CNF2=10 MODE2=11. */
    GPIOA_CRL &= ~(0xFUL << 8U);
    GPIOA_CRL |= (0xBUL << 8U);

    /* PA3: floating input, useful if RX is connected later. */
    GPIOA_CRL &= ~(0xFUL << 12U);
    GPIOA_CRL |= (0x4UL << 12U);
}

static void adc1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* ADC clock is PCLK2 / 6: safe for both 36 MHz and 72 MHz system clocks. */
    RCC_CFGR &= ~(0x3UL << 14U);
    RCC_CFGR |= (0x2UL << 14U);

    ADC1_SMPR2 &= ~(0x7UL << 0U);
    ADC1_SMPR2 |= (0x7UL << 0U);    /* Channel 0 sample time: 239.5 cycles. */
    ADC1_SQR3 = 0U;                 /* First conversion: channel 0. */
    ADC1_CR2 &= ~(0x7UL << 17U);
    ADC1_CR2 |= ADC_CR2_EXTTRIG | (0x7UL << 17U);

    ADC1_CR2 |= ADC_CR2_ADON;
    delay_cycles(10000UL);

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
    ADC1_CR2 |= ADC_CR2_SWSTART;

    while ((ADC1_SR & ADC_SR_EOC) == 0U) {
    }

    return (uint16_t)(ADC1_DR & 0x0FFFU);
}

static void usart2_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    USART2_BRR = (CPU_CLOCK_HZ + (USART_BAUD / 2U)) / USART_BAUD;
    USART2_CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void usart2_write_char(char ch)
{
    while ((USART2_SR & USART_SR_TXE) == 0U) {
    }

    USART2_DR = (uint32_t)ch;
}

static void usart2_write_string(const char *text)
{
    while (*text != '\0') {
        usart2_write_char(*text++);
    }
}

static void usart2_write_u32(uint32_t value)
{
    char buffer[10];
    uint32_t index = 0U;

    if (value == 0U) {
        usart2_write_char('0');
        return;
    }

    while ((value > 0U) && (index < sizeof(buffer))) {
        buffer[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (index > 0U) {
        usart2_write_char(buffer[--index]);
    }
}
