/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Reaction speed tester with 4 LEDs, button interrupt, UART
 ******************************************************************************
 */

#include <stdint.h>

#define PERIPH_BASE         (0x40000000UL)
#define APB1PERIPH_BASE     PERIPH_BASE
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE      (PERIPH_BASE + 0x00020000UL)

#define RCC_BASE            (AHBPERIPH_BASE + 0x00001000UL)
#define AFIO_BASE           (APB2PERIPH_BASE + 0x00000000UL)
#define EXTI_BASE           (APB2PERIPH_BASE + 0x00000400UL)
#define GPIOA_BASE          (APB2PERIPH_BASE + 0x00000800UL)
#define GPIOB_BASE          (APB2PERIPH_BASE + 0x00000C00UL)
#define GPIOC_BASE          (APB2PERIPH_BASE + 0x00001000UL)
#define USART2_BASE         (APB1PERIPH_BASE + 0x00004400UL)
#define SYSTICK_BASE        (0xE000E010UL)
#define NVIC_ISER1          (*(volatile uint32_t *)0xE000E104UL)

#define BIT(n)              (1UL << (n))

#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18UL))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x1CUL))

#define GPIOA_CRL           (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))

#define GPIOB_CRL           (*(volatile uint32_t *)(GPIOB_BASE + 0x00UL))
#define GPIOB_CRH           (*(volatile uint32_t *)(GPIOB_BASE + 0x04UL))
#define GPIOB_ODR           (*(volatile uint32_t *)(GPIOB_BASE + 0x0CUL))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x10UL))

#define GPIOC_CRL           (*(volatile uint32_t *)(GPIOC_BASE + 0x00UL))
#define GPIOC_CRH           (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL))
#define GPIOC_BSRR          (*(volatile uint32_t *)(GPIOC_BASE + 0x10UL))

#define AFIO_EXTICR4        (*(volatile uint32_t *)(AFIO_BASE + 0x14UL))

#define EXTI_IMR            (*(volatile uint32_t *)(EXTI_BASE + 0x00UL))
#define EXTI_FTSR           (*(volatile uint32_t *)(EXTI_BASE + 0x0CUL))
#define EXTI_PR             (*(volatile uint32_t *)(EXTI_BASE + 0x14UL))

#define USART2_SR           (*(volatile uint32_t *)(USART2_BASE + 0x00UL))
#define USART2_DR           (*(volatile uint32_t *)(USART2_BASE + 0x04UL))
#define USART2_BRR          (*(volatile uint32_t *)(USART2_BASE + 0x08UL))
#define USART2_CR1          (*(volatile uint32_t *)(USART2_BASE + 0x0CUL))

#define SYSTICK_CTRL        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00UL))
#define SYSTICK_LOAD        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04UL))
#define SYSTICK_VAL         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08UL))

#define LED_3_PIN           8U       /* Countdown 3 LED: PC8 */
#define LED_2_PIN           6U       /* Countdown 2 LED: PC6 */
#define LED_1_PIN           5U       /* Countdown 1 LED: PC5 */
#define GO_LED_PIN          14U      /* Reaction LED: PB14 */
#define BUZZER_PIN          15U      /* Buzzer: PB15 */
#define BUTTON_PIN          13U      /* Built-in USER button: PC13 */
#define OLED_SCL_PIN        8U       /* Arduino SCL / OLED SCK: PB8 */
#define OLED_SDA_PIN        9U       /* Arduino SDA: PB9 */
#define OLED_ADDR           0x3CU    /* SSD1306 default I2C address */

#define NOTE_REST           0U
#define NOTE_LOW_5          196U
#define NOTE_LOW_6          220U
#define NOTE_1              262U
#define NOTE_2              294U
#define NOTE_3              330U
#define NOTE_4              349U
#define NOTE_5              392U
#define NOTE_6              440U
#define NOTE_FLAT_7         466U
#define NOTE_7              494U
#define NOTE_HIGH_1         523U
#define NOTE_HIGH_2         587U
#define NOTE_HIGH_3         659U
#define NOTE_HIGH_4         698U
#define NOTE_HIGH_5         784U
#define NOTE_HIGH_6         880U
#define NOTE_HIGH_7         988U
#define NOTE_DOUBLE_HIGH_1  1046U
#define NOTE_DURATION_MS    105U
#define COUNTDOWN_BEEP_HZ   1000U
#define GO_BEEP_HZ          2000U
#define COUNTDOWN_BEEP_MS   180U
#define COUNTDOWN_STEP_MS   1000U
#define GO_BEEP_MS          500U

typedef enum
{
    GAME_IDLE,
    GAME_COUNTDOWN,
    GAME_WAIT_RANDOM,
    GAME_GO,
    GAME_RESULT
} game_state_t;

static void gpio_init(void);
static void usart2_init(void);
static void systick_init(void);
static void exti_button_init(void);
static void oled_init(void);
static void oled_clear(void);
static void oled_show_ready(void);
static void oled_show_go(void);
static void oled_show_success(void);
static void oled_show_fail(void);
static void usart2_write_char(char ch);
static void usart2_write_string(const char *text);
static void usart2_write_uint(uint32_t value);
static void usart2_write_time_ms(uint32_t time_ms);
static void delay_ms(uint32_t delay);
static void delay_us(uint32_t delay);
static uint32_t get_random_delay_ms(void);
static void countdown_led_on(uint8_t pin);
static void countdown_led_off(uint8_t pin);
static void countdown_leds_off(void);
static void go_led_on(void);
static void go_led_off(void);
static void buzzer_on(void);
static void buzzer_off(void);
static void play_tone(uint32_t frequency_hz, uint32_t duration_ms);
static void play_countdown_beep(void);
static void play_go_beep(void);
static void play_success_song(void);
static void play_fail_sound(void);
static void all_leds_on(void);
static void all_leds_off(void);
static void success_animation(void);
static void fail_animation(void);

static volatile uint32_t system_ms;
static volatile uint32_t last_button_ms;
static volatile game_state_t game_state = GAME_IDLE;
static volatile uint8_t start_requested;
static volatile uint8_t too_early;
static volatile uint8_t reaction_done;
static volatile uint32_t led_on_time_ms;
static volatile uint32_t reaction_time_ms;

void SystemInit(void)
{
    /* Keep the reset clock setup: HSI 8 MHz. */
}

int main(void)
{
    gpio_init();
    usart2_init();
    systick_init();
    oled_init();
    exti_button_init();

    all_leds_off();
    oled_clear();
    oled_show_ready();
    usart2_write_string("Serial ready\r\n");
    usart2_write_string("Press button to start\r\n");

    for (;;)
    {
        if (start_requested != 0U)
        {
            uint32_t wait_start_ms;
            uint32_t random_delay_ms;

            start_requested = 0U;
            too_early = 0U;
            reaction_done = 0U;
            all_leds_off();
            oled_clear();

            game_state = GAME_COUNTDOWN;
            usart2_write_string("Ready\r\n");

            usart2_write_string("3\r\n");
            countdown_led_on(LED_3_PIN);
            play_countdown_beep();
            delay_ms(COUNTDOWN_STEP_MS - COUNTDOWN_BEEP_MS);
            countdown_led_off(LED_3_PIN);

            usart2_write_string("2\r\n");
            countdown_led_on(LED_2_PIN);
            play_countdown_beep();
            delay_ms(COUNTDOWN_STEP_MS - COUNTDOWN_BEEP_MS);
            countdown_led_off(LED_2_PIN);

            usart2_write_string("1\r\n");
            countdown_led_on(LED_1_PIN);
            play_countdown_beep();
            delay_ms(COUNTDOWN_STEP_MS - COUNTDOWN_BEEP_MS);
            countdown_led_off(LED_1_PIN);

            countdown_leds_off();

            game_state = GAME_WAIT_RANDOM;
            random_delay_ms = get_random_delay_ms();
            wait_start_ms = system_ms;

            while (((system_ms - wait_start_ms) < random_delay_ms) && (too_early == 0U))
            {
            }

            if (too_early != 0U)
            {
                usart2_write_string("Too early!\r\n");
                oled_show_fail();
                play_fail_sound();
                fail_animation();
                usart2_write_string("Press button to start\r\n");
                game_state = GAME_IDLE;
                continue;
            }

            game_state = GAME_GO;
            led_on_time_ms = system_ms;
            go_led_on();
            usart2_write_string("GO!\r\n");
            oled_show_go();
            play_go_beep();

            while (reaction_done == 0U)
            {
            }

            game_state = GAME_RESULT;
            go_led_off();
            usart2_write_string("Reaction Time: ");
            usart2_write_time_ms(reaction_time_ms);
            usart2_write_string(" sec\r\n");

            if (reaction_time_ms <= 1000U)
            {
                usart2_write_string("Success!\r\n");
                oled_show_success();
                success_animation();
                play_success_song();
            }
            else
            {
                usart2_write_string("Fail!\r\n");
                oled_show_fail();
                play_fail_sound();
                fail_animation();
            }

            usart2_write_string("Press button to start\r\n");
            game_state = GAME_IDLE;
        }
    }
}

static void gpio_init(void)
{
    RCC_APB2ENR |= BIT(0) | BIT(2) | BIT(3) | BIT(4); /* AFIO, GPIOA, GPIOB, GPIOC */

    /* PC5, PC6: general purpose push-pull output, 2 MHz. */
    GPIOC_CRL &= ~((0xFUL << (LED_1_PIN * 4U)) | (0xFUL << (LED_2_PIN * 4U)));
    GPIOC_CRL |=  ((0x2UL << (LED_1_PIN * 4U)) | (0x2UL << (LED_2_PIN * 4U)));

    /* PC8: general purpose push-pull output, 2 MHz. */
    GPIOC_CRH &= ~(0xFUL << ((LED_3_PIN - 8U) * 4U));
    GPIOC_CRH |=  (0x2UL << ((LED_3_PIN - 8U) * 4U));

    /* PB14, PB15: general purpose push-pull output, 2 MHz. */
    GPIOB_CRH &= ~((0xFUL << ((GO_LED_PIN - 8U) * 4U)) |
                   (0xFUL << ((BUZZER_PIN - 8U) * 4U)));
    GPIOB_CRH |=  ((0x2UL << ((GO_LED_PIN - 8U) * 4U)) |
                   (0x2UL << ((BUZZER_PIN - 8U) * 4U)));

    /* PB8, PB9: software I2C output, 2 MHz for OLED. */
    GPIOB_CRH &= ~((0xFUL << ((OLED_SCL_PIN - 8U) * 4U)) |
                   (0xFUL << ((OLED_SDA_PIN - 8U) * 4U)));
    GPIOB_CRH |=  ((0x2UL << ((OLED_SCL_PIN - 8U) * 4U)) |
                   (0x2UL << ((OLED_SDA_PIN - 8U) * 4U)));
    GPIOB_BSRR = BIT(OLED_SCL_PIN) | BIT(OLED_SDA_PIN);

    /* PC13: input with pull-up for the built-in USER button. */
    GPIOC_CRH &= ~(0xFUL << ((BUTTON_PIN - 8U) * 4U));
    GPIOC_CRH |=  (0x8UL << ((BUTTON_PIN - 8U) * 4U));
    GPIOC_BSRR = BIT(BUTTON_PIN);

    /* PA2: USART2_TX alternate-function push-pull, 50 MHz. */
    GPIOA_CRL &= ~(0xFUL << (2U * 4U));
    GPIOA_CRL |=  (0xBUL << (2U * 4U));

    /* PA3: USART2_RX floating input. */
    GPIOA_CRL &= ~(0xFUL << (3U * 4U));
    GPIOA_CRL |=  (0x4UL << (3U * 4U));
}

static void usart2_init(void)
{
    RCC_APB1ENR |= BIT(17);          /* USART2 clock */

    USART2_CR1 = 0U;
    USART2_BRR = 0x0341U;            /* 8 MHz / 9600 bps */
    USART2_CR1 = BIT(13) | BIT(3) | BIT(2); /* UE, TE, RE */
}

static void systick_init(void)
{
    SYSTICK_LOAD = 8000U - 1U;       /* 1 ms tick from HSI 8 MHz */
    SYSTICK_VAL = 0U;
    SYSTICK_CTRL = BIT(2) | BIT(1) | BIT(0); /* CLKSOURCE, TICKINT, ENABLE */
}

static void exti_button_init(void)
{
    AFIO_EXTICR4 &= ~(0xFUL << 4U);
    AFIO_EXTICR4 |=  (0x2UL << 4U);  /* EXTI13 source = GPIOC */

    EXTI_IMR  |= BIT(BUTTON_PIN);
    EXTI_FTSR |= BIT(BUTTON_PIN);    /* PC13 USER button press is falling edge */
    EXTI_PR    = BIT(BUTTON_PIN);

    NVIC_ISER1 = BIT(8);             /* EXTI15_10_IRQn = 40 */
}

static void oled_i2c_delay(void)
{
    delay_us(5U);
}

static void oled_scl_high(void)
{
    GPIOB_BSRR = BIT(OLED_SCL_PIN);
}

static void oled_scl_low(void)
{
    GPIOB_BSRR = BIT(OLED_SCL_PIN + 16U);
}

static void oled_sda_high(void)
{
    GPIOB_BSRR = BIT(OLED_SDA_PIN);
}

static void oled_sda_low(void)
{
    GPIOB_BSRR = BIT(OLED_SDA_PIN + 16U);
}

static void oled_i2c_start(void)
{
    oled_sda_high();
    oled_scl_high();
    oled_i2c_delay();
    oled_sda_low();
    oled_i2c_delay();
    oled_scl_low();
}

static void oled_i2c_stop(void)
{
    oled_sda_low();
    oled_i2c_delay();
    oled_scl_high();
    oled_i2c_delay();
    oled_sda_high();
    oled_i2c_delay();
}

static void oled_i2c_write(uint8_t data)
{
    for (uint8_t bit = 0U; bit < 8U; bit++)
    {
        if ((data & 0x80U) != 0U)
        {
            oled_sda_high();
        }
        else
        {
            oled_sda_low();
        }

        oled_i2c_delay();
        oled_scl_high();
        oled_i2c_delay();
        oled_scl_low();
        data <<= 1U;
    }

    /* ACK clock. SDA is released high; the ACK bit itself is not checked. */
    oled_sda_high();
    oled_i2c_delay();
    oled_scl_high();
    oled_i2c_delay();
    oled_scl_low();
}

static void oled_write_command(uint8_t command)
{
    oled_i2c_start();
    oled_i2c_write((OLED_ADDR << 1U) | 0U);
    oled_i2c_write(0x00U);
    oled_i2c_write(command);
    oled_i2c_stop();
}

static void oled_write_data(uint8_t data)
{
    oled_i2c_start();
    oled_i2c_write((OLED_ADDR << 1U) | 0U);
    oled_i2c_write(0x40U);
    oled_i2c_write(data);
    oled_i2c_stop();
}

static void oled_set_position(uint8_t page, uint8_t column)
{
    oled_write_command((uint8_t)(0xB0U | page));
    oled_write_command((uint8_t)(0x00U | (column & 0x0FU)));
    oled_write_command((uint8_t)(0x10U | (column >> 4U)));
}

static uint8_t oled_go_font(char ch, uint8_t row, uint8_t col)
{
    static const uint8_t g[7] = {0x0EU, 0x11U, 0x10U, 0x17U, 0x11U, 0x11U, 0x0EU};
    static const uint8_t o[7] = {0x0EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
    static const uint8_t bang[7] = {0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x00U, 0x04U};
    const uint8_t *glyph = g;

    if (ch == 'O')
    {
        glyph = o;
    }
    else if (ch == '!')
    {
        glyph = bang;
    }

    return (uint8_t)((glyph[row] >> (4U - col)) & 0x01U);
}

static uint8_t oled_small_font(char ch, uint8_t row, uint8_t col)
{
    static const uint8_t r[7] = {0x1EU, 0x11U, 0x11U, 0x1EU, 0x14U, 0x12U, 0x11U};
    static const uint8_t e[7] = {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x1FU};
    static const uint8_t a[7] = {0x0EU, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U};
    static const uint8_t d[7] = {0x1EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x1EU};
    static const uint8_t y_glyph[7] = {0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x04U, 0x04U};
    static const uint8_t s[7] = {0x0FU, 0x10U, 0x10U, 0x0EU, 0x01U, 0x01U, 0x1EU};
    static const uint8_t u[7] = {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
    static const uint8_t c[7] = {0x0EU, 0x11U, 0x10U, 0x10U, 0x10U, 0x11U, 0x0EU};
    static const uint8_t f[7] = {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x10U};
    static const uint8_t i[7] = {0x0EU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU};
    static const uint8_t l[7] = {0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x1FU};
    const uint8_t *glyph = e;

    if (ch == 'R')
    {
        glyph = r;
    }
    else if (ch == 'A')
    {
        glyph = a;
    }
    else if (ch == 'D')
    {
        glyph = d;
    }
    else if (ch == 'Y')
    {
        glyph = y_glyph;
    }
    else if (ch == 'S')
    {
        glyph = s;
    }
    else if (ch == 'U')
    {
        glyph = u;
    }
    else if (ch == 'C')
    {
        glyph = c;
    }
    else if (ch == 'F')
    {
        glyph = f;
    }
    else if (ch == 'I')
    {
        glyph = i;
    }
    else if (ch == 'L')
    {
        glyph = l;
    }

    return (uint8_t)((glyph[row] >> (4U - col)) & 0x01U);
}

static uint8_t oled_text_pixel(uint8_t x, uint8_t y, const char *text, uint8_t length, uint8_t scale, uint8_t start_y)
{
    const uint8_t glyph_width = 5U;
    const uint8_t glyph_height = 7U;
    const uint8_t gap = 2U;
    const uint8_t text_width = (uint8_t)(((glyph_width * scale) * length) + ((gap * scale) * (length - 1U)));
    const uint8_t text_height = (uint8_t)(glyph_height * scale);
    const uint8_t start_x = (uint8_t)((128U - text_width) / 2U);
    uint8_t local_x;
    uint8_t local_y;
    uint8_t char_index;
    uint8_t char_span;
    uint8_t in_char_x;

    if ((x < start_x) || (y < start_y) ||
        (x >= (start_x + text_width)) || (y >= (start_y + text_height)))
    {
        return 0U;
    }

    local_x = (uint8_t)(x - start_x);
    local_y = (uint8_t)(y - start_y);
    char_span = (uint8_t)((glyph_width + gap) * scale);
    char_index = (uint8_t)(local_x / char_span);

    if (char_index >= length)
    {
        return 0U;
    }

    in_char_x = (uint8_t)(local_x - (char_index * char_span));
    if (in_char_x >= (glyph_width * scale))
    {
        return 0U;
    }

    if ((text[char_index] == 'G') || (text[char_index] == 'O') || (text[char_index] == '!'))
    {
        return oled_go_font(text[char_index],
                            (uint8_t)(local_y / scale),
                            (uint8_t)(in_char_x / scale));
    }

    return oled_small_font(text[char_index],
                           (uint8_t)(local_y / scale),
                           (uint8_t)(in_char_x / scale));
}

static void oled_init(void)
{
    delay_ms(100U);

    oled_write_command(0xAEU); /* Display off */
    oled_write_command(0xD5U);
    oled_write_command(0x80U);
    oled_write_command(0xA8U);
    oled_write_command(0x1FU); /* 128x32 multiplex */
    oled_write_command(0xD3U);
    oled_write_command(0x00U);
    oled_write_command(0x40U);
    oled_write_command(0x8DU);
    oled_write_command(0x14U);
    oled_write_command(0x20U);
    oled_write_command(0x00U); /* Horizontal addressing */
    oled_write_command(0xA1U);
    oled_write_command(0xC8U);
    oled_write_command(0xDAU);
    oled_write_command(0x02U);
    oled_write_command(0x81U);
    oled_write_command(0xCFU);
    oled_write_command(0xD9U);
    oled_write_command(0xF1U);
    oled_write_command(0xDBU);
    oled_write_command(0x40U);
    oled_write_command(0xA4U);
    oled_write_command(0xA6U);
    oled_write_command(0xAFU); /* Display on */
}

static void oled_clear(void)
{
    for (uint8_t page = 0U; page < 4U; page++)
    {
        oled_set_position(page, 0U);
        for (uint8_t column = 0U; column < 128U; column++)
        {
            oled_write_data(0x00U);
        }
    }
}

static void oled_draw_text(const char *text, uint8_t length, uint8_t scale, uint8_t start_y)
{
    for (uint8_t page = 0U; page < 4U; page++)
    {
        oled_set_position(page, 0U);
        for (uint8_t column = 0U; column < 128U; column++)
        {
            uint8_t data = 0U;

            for (uint8_t bit = 0U; bit < 8U; bit++)
            {
                uint8_t y = (uint8_t)((page * 8U) + bit);

                if (oled_text_pixel(column, y, text, length, scale, start_y) != 0U)
                {
                    data |= BIT(bit);
                }
            }

            oled_write_data(data);
        }
    }
}

static void oled_show_ready(void)
{
    const char text[] = {'R', 'E', 'A', 'D', 'Y'};
    oled_draw_text(text, 5U, 1U, 12U);
}

static void oled_show_go(void)
{
    const char text[] = {'G', 'O', '!'};
    oled_draw_text(text, 3U, 2U, 4U);
}

static void oled_show_success(void)
{
    const char text[] = {'S', 'U', 'C', 'C', 'E', 'S', 'S'};
    oled_draw_text(text, 7U, 1U, 10U);
}

static void oled_show_fail(void)
{
    const char text[] = {'F', 'A', 'I', 'L'};
    oled_draw_text(text, 4U, 1U, 12U);
}

static void usart2_write_char(char ch)
{
    while ((USART2_SR & BIT(7)) == 0U)
    {
    }

    USART2_DR = (uint32_t)ch;
}

static void usart2_write_string(const char *text)
{
    while (*text != '\0')
    {
        usart2_write_char(*text++);
    }
}

static void usart2_write_uint(uint32_t value)
{
    char buffer[10];
    uint32_t index = 0U;

    if (value == 0U)
    {
        usart2_write_char('0');
        return;
    }

    while (value != 0U)
    {
        buffer[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (index != 0U)
    {
        usart2_write_char(buffer[--index]);
    }
}

static void usart2_write_time_ms(uint32_t time_ms)
{
    uint32_t seconds = time_ms / 1000U;
    uint32_t milliseconds = time_ms % 1000U;

    usart2_write_uint(seconds);
    usart2_write_char('.');
    usart2_write_char((char)('0' + (milliseconds / 100U)));
    usart2_write_char((char)('0' + ((milliseconds / 10U) % 10U)));
    usart2_write_char((char)('0' + (milliseconds % 10U)));
}

static void delay_ms(uint32_t delay)
{
    uint32_t start = system_ms;

    while ((system_ms - start) < delay)
    {
    }
}

static void delay_us(uint32_t delay)
{
    for (volatile uint32_t count = 0U; count < (delay * 2U); count++)
    {
    }
}

static uint32_t get_random_delay_ms(void)
{
    static uint32_t seed = 0x12345678UL;

    seed ^= system_ms;
    seed = (seed * 1664525UL) + 1013904223UL;

    return 1000U + (seed % 4001U);   /* 1.000 s to 5.000 s */
}

static void countdown_led_on(uint8_t pin)
{
    GPIOC_BSRR = BIT(pin);
}

static void countdown_led_off(uint8_t pin)
{
    GPIOC_BSRR = BIT(pin + 16U);
}

static void countdown_leds_off(void)
{
    countdown_led_off(LED_3_PIN);
    countdown_led_off(LED_2_PIN);
    countdown_led_off(LED_1_PIN);
}

static void go_led_on(void)
{
    GPIOB_BSRR = BIT(GO_LED_PIN);
}

static void go_led_off(void)
{
    GPIOB_BSRR = BIT(GO_LED_PIN + 16U);
}

static void buzzer_on(void)
{
    GPIOB_BSRR = BIT(BUZZER_PIN);
}

static void buzzer_off(void)
{
    GPIOB_BSRR = BIT(BUZZER_PIN + 16U);
}

static void play_tone(uint32_t frequency_hz, uint32_t duration_ms)
{
    uint32_t half_period_us;
    uint32_t cycles;

    if (frequency_hz == NOTE_REST)
    {
        buzzer_off();
        delay_ms(duration_ms);
        return;
    }

    half_period_us = 500000U / frequency_hz;
    cycles = ((duration_ms * 1000U) / (half_period_us * 2U));

    for (uint32_t i = 0U; i < cycles; i++)
    {
        buzzer_on();
        delay_us(half_period_us);
        buzzer_off();
        delay_us(half_period_us);
    }

    buzzer_off();
}

static void play_countdown_beep(void)
{
    play_tone(COUNTDOWN_BEEP_HZ, COUNTDOWN_BEEP_MS);
}

static void play_go_beep(void)
{
    play_tone(GO_BEEP_HZ, GO_BEEP_MS);
}

static void play_success_song(void)
{
    static const uint16_t song[] =
    {
        NOTE_HIGH_1, NOTE_HIGH_5, NOTE_HIGH_3, NOTE_HIGH_6,
        NOTE_DOUBLE_HIGH_1, NOTE_HIGH_7, NOTE_HIGH_6, NOTE_HIGH_5,
        NOTE_HIGH_3, NOTE_HIGH_5, NOTE_HIGH_6, NOTE_HIGH_4,
        NOTE_HIGH_5, NOTE_HIGH_3, NOTE_HIGH_1, NOTE_HIGH_2,
        NOTE_DOUBLE_HIGH_1,
    };

    for (uint32_t i = 0U; i < (sizeof(song) / sizeof(song[0])); i++)
    {
        play_tone(song[i], NOTE_DURATION_MS);
        delay_ms(25U);
    }
}

static void play_fail_sound(void)
{
    for (uint32_t repeat = 0U; repeat < 2U; repeat++)
    {
        for (uint32_t beep = 0U; beep < 3U; beep++)
        {
            play_tone(1000U, 160U);
            delay_ms(120U);
        }

        delay_ms(300U);
    }
}

static void all_leds_on(void)
{
    countdown_led_on(LED_3_PIN);
    countdown_led_on(LED_2_PIN);
    countdown_led_on(LED_1_PIN);
    go_led_on();
}

static void all_leds_off(void)
{
    countdown_leds_off();
    go_led_off();
    buzzer_off();
}

static void success_animation(void)
{
    for (uint32_t repeat = 0U; repeat < 3U; repeat++)
    {
        countdown_led_on(LED_3_PIN);
        delay_ms(150U);
        countdown_led_off(LED_3_PIN);

        countdown_led_on(LED_2_PIN);
        delay_ms(150U);
        countdown_led_off(LED_2_PIN);

        countdown_led_on(LED_1_PIN);
        delay_ms(150U);
        countdown_led_off(LED_1_PIN);

        delay_ms(150U);
    }
}

static void fail_animation(void)
{
    all_leds_on();
    delay_ms(2000U);
    all_leds_off();
}

void SysTick_Handler(void)
{
    system_ms++;
}

void EXTI15_10_IRQHandler(void)
{
    if ((EXTI_PR & BIT(BUTTON_PIN)) != 0U)
    {
        uint32_t now = system_ms;

        EXTI_PR = BIT(BUTTON_PIN);

        if ((now - last_button_ms) < 50U)
        {
            return;
        }

        last_button_ms = now;

        if (game_state == GAME_IDLE)
        {
            start_requested = 1U;
        }
        else if (game_state == GAME_WAIT_RANDOM)
        {
            too_early = 1U;
        }
        else if (game_state == GAME_GO)
        {
            reaction_time_ms = now - led_on_time_ms;
            reaction_done = 1U;
        }
    }
}

int __io_putchar(int ch)
{
    usart2_write_char((char)ch);
    return ch;
}
