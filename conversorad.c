#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "ssd1306.h"

// Definições de pinos
#define LED_VERDE       11
#define LED_AZUL        12
#define LED_VERMELHA    13
#define JOYSTICK_X_ADC_CH  1   // GP27 (ADC1) -> Eixo X
#define JOYSTICK_Y_ADC_CH  0   // GP26 (ADC0) -> Eixo Y (inverso)
#define JOYSTICK_BTN    22
#define BUTTON_A        5
#define I2C_PORT        i2c1
#define I2C_SDA_PIN     14
#define I2C_SCL_PIN     15
#define SSD1306_ADDR    0x3C

// Configuração
#define PWM_WRAP_VALUE  4095
#define DEBOUNCE_DELAY_US 200000
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  64
#define SQUARE_SIZE     8
#define DEADZONE        100    // Zona morta (desvio mínimo para considerar fora do centro)

volatile absolute_time_t last_joystick_btn_time, last_button_A_time;
volatile bool pwm_enabled = true;
volatile int border_style = 0;

// Função de callback para interrupções de botão
void gpio_callback(uint gpio, uint32_t events) {
    absolute_time_t now = get_absolute_time();
    gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_FALL);

    if (gpio == JOYSTICK_BTN && absolute_time_diff_us(last_joystick_btn_time, now) > DEBOUNCE_DELAY_US) {
        last_joystick_btn_time = now;
        gpio_put(LED_VERDE, !gpio_get(LED_VERDE));
        border_style = (border_style + 1) % 2;
        printf("Joystick botão pressionado. border_style = %d\n", border_style);
    }

    if (gpio == BUTTON_A && absolute_time_diff_us(last_button_A_time, now) > DEBOUNCE_DELAY_US) {
        last_button_A_time = now;
        pwm_enabled = !pwm_enabled;
        if (!pwm_enabled) {
            pwm_set_gpio_level(LED_AZUL, 0);
            pwm_set_gpio_level(LED_VERMELHA, 0);
        }
        printf("Botão A pressionado. PWM %s\n", pwm_enabled ? "ativado" : "desativado");
    }
}

int main() {
    stdio_init_all();
    printf("Inicializando...\n");

    // Configuração do LED Verde
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0);

    // Configuração dos LEDs PWM
    gpio_set_function(LED_AZUL, GPIO_FUNC_PWM);
    gpio_set_function(LED_VERMELHA, GPIO_FUNC_PWM);
    uint slice_blue = pwm_gpio_to_slice_num(LED_AZUL);
    uint slice_red = pwm_gpio_to_slice_num(LED_VERMELHA);
    pwm_set_wrap(slice_blue, PWM_WRAP_VALUE);
    pwm_set_wrap(slice_red, PWM_WRAP_VALUE);
    pwm_set_enabled(slice_blue, true);
    pwm_set_enabled(slice_red, true);

    // Inicialização do ADC
    adc_init();
    adc_gpio_init(27);  // Joystick X
    adc_gpio_init(26);  // Joystick Y

    // Configuração do I2C
    i2c_init(I2C_PORT, 100 * 1000);  // Velocidade reduzida para maior estabilidade
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    ssd1306_t display;
    if (i2c_write_blocking(I2C_PORT, SSD1306_ADDR, NULL, 0, false) == PICO_ERROR_GENERIC) {
        printf("Erro: Display SSD1306 não encontrado no endereço 0x%02x\n", SSD1306_ADDR);
        return 1;
    }
    ssd1306_init(&display, I2C_PORT, SSD1306_ADDR, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    printf("Display SSD1306 inicializado.\n");

    // Configuração dos botões
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        // Leitura dos valores ADC
        adc_select_input(JOYSTICK_X_ADC_CH);
        uint16_t adc_x = adc_read();
        adc_select_input(JOYSTICK_Y_ADC_CH);
        uint16_t adc_y = adc_read();

        // Controle de PWM (LEDs desligados na zona morta)
        if (pwm_enabled) {
            if (abs((int)adc_y - 2048) > DEADZONE) {
                uint32_t pwm_level_blue = ((uint32_t)abs((int)adc_y - 2048) * PWM_WRAP_VALUE) / 2048;
                pwm_set_gpio_level(LED_AZUL, pwm_level_blue > PWM_WRAP_VALUE ? PWM_WRAP_VALUE : pwm_level_blue);
            } else {
                pwm_set_gpio_level(LED_AZUL, 0);  // Apaga o LED Azul se na zona morta
            }

            if (abs((int)adc_x - 2048) > DEADZONE) {
                uint32_t pwm_level_red = ((uint32_t)abs((int)adc_x - 2048) * PWM_WRAP_VALUE) / 2048;
                pwm_set_gpio_level(LED_VERMELHA, pwm_level_red > PWM_WRAP_VALUE ? PWM_WRAP_VALUE : pwm_level_red);
            } else {
                pwm_set_gpio_level(LED_VERMELHA, 0);  // Apaga o LED Vermelho se na zona morta
            }
        } else {
            pwm_set_gpio_level(LED_AZUL, 0);
            pwm_set_gpio_level(LED_VERMELHA, 0);
        }

        // Atualização do display 
        ssd1306_clear(&display);
        uint8_t pos_x = (uint32_t)adc_x * (DISPLAY_WIDTH - SQUARE_SIZE) / 4095;
        uint8_t pos_y = (uint32_t)(4095 - adc_y) * (DISPLAY_HEIGHT - SQUARE_SIZE) / 4095;  // Inverte o eixo Y
        ssd1306_fill_rect(&display, pos_x, pos_y, SQUARE_SIZE, SQUARE_SIZE, 1);
        
        if (border_style == 0) {
            ssd1306_draw_rect(&display, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 1);
        } else {
            for (int x = 0; x < DISPLAY_WIDTH; x += 4) {
                ssd1306_draw_pixel(&display, x, 0, 1);
                ssd1306_draw_pixel(&display, x, DISPLAY_HEIGHT - 1, 1);
            }
        }
        
        ssd1306_show(&display);
        sleep_ms(50);
    }

    return 0;
}
