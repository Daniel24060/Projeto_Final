#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "ws2818b.pio.h"
#include "inc/ssd1306.h"

// =====================
// DEFINIÇÕES E CONFIGURAÇÕES
// =====================
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define ALARM_BLINK_INTERVAL_MS 500
#define DEBOUNCE_TIME_MS 200
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define MICROPHONE_PIN 28
#define NOISE_THRESHOLD 2500
#define BUZZER_HIGH_PIN 21    // GPIO 21 - PWM Slice 10
#define BUZZER_LOW_PIN 10     // GPIO 10 - PWM Slice 5
#define BUZZER_HIGH_FREQ 2000 // 2 kHz
#define BUZZER_LOW_FREQ 500   // 500 Hz
#define LED_MATRIX_PIN 7
#define BUTTON_EMERGENCY 5
#define BUTTON_RESET 6
#define I2C_SDA 14
#define I2C_SCL 15

// Estrutura para área de renderização
struct render_area frame_area = {
    .start_column = 0,
    .end_column = ssd1306_width - 1,
    .start_page = 0,
    .end_page = ssd1306_n_pages - 1
};

// Buffer para o display SSD1306
uint8_t ssd1306_buffer[ssd1306_buffer_length];

// =====================
// DEFINIÇÕES DE TIPOS
// =====================
typedef struct {
    uint8_t G;
    uint8_t R;
    uint8_t B;
} npLED_t;

typedef struct {
    PIO pio;
    uint sm;
    npLED_t leds[MATRIX_WIDTH * MATRIX_HEIGHT];
    uint16_t x_indices[12];
} LEDMatrix;

typedef struct {
    absolute_time_t last_trigger;
    bool pressed;
    uint8_t pin;
} Button;

typedef struct {
    LEDMatrix matrix;
    Button btn_emergency;
    Button btn_reset;
    bool alarm_active;
    bool needs_display_update;
    char display_message[64];
} AlarmSystem;

// =====================
// VARIÁVEIS GLOBAIS
// =====================
static AlarmSystem alarm_system;

// =====================
// IMPLEMENTAÇÃO LED MATRIX
// =====================
void ledmatrix_init(LEDMatrix *matrix, uint pin) {
    matrix->pio = pio0;
    uint offset = pio_add_program(matrix->pio, &ws2818b_program);
    matrix->sm = pio_claim_unused_sm(matrix->pio, true);
    ws2818b_program_init(matrix->pio, matrix->sm, offset, pin, 800000.f);
    memset(matrix->leds, 0, sizeof(matrix->leds));
    const uint16_t indices[] = {0, 4, 6, 8, 12, 16, 18, 20, 24};
    memcpy(matrix->x_indices, indices, sizeof(indices));
}

void ledmatrix_show_pattern(LEDMatrix *matrix, const npLED_t *color) {
    for (int i = 0; i < 9; i++) {
        uint idx = matrix->x_indices[i];
        matrix->leds[idx].R = color->R;
        matrix->leds[idx].G = color->G;
        matrix->leds[idx].B = color->B;
    }
    for (int i = 0; i < MATRIX_WIDTH * MATRIX_HEIGHT; i++) {
        pio_sm_put_blocking(matrix->pio, matrix->sm, matrix->leds[i].G);
        pio_sm_put_blocking(matrix->pio, matrix->sm, matrix->leds[i].R);
        pio_sm_put_blocking(matrix->pio, matrix->sm, matrix->leds[i].B);
    }
    sleep_us(100);
}

// =====================
// CONTROLE DOS BUZZERS COM PWM
// =====================
void buzzer_init(uint pin, uint freq) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    
    float divider = (float)clock_get_hz(clk_sys) / (freq * 256);
    pwm_config_set_clkdiv(&config, divider);
    pwm_config_set_wrap(&config, 255);
    pwm_init(slice_num, &config, true);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(pin), 0);
}

void buzzer_on(uint pin) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    pwm_set_chan_level(slice_num, channel, 128);
}

void buzzer_off(uint pin) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    pwm_set_chan_level(slice_num, channel, 0);
}

void buzzer_siren() {
    static bool siren_state = false;
    siren_state = !siren_state;
    siren_state ? buzzer_on(BUZZER_HIGH_PIN) : buzzer_on(BUZZER_LOW_PIN);
    siren_state ? buzzer_off(BUZZER_LOW_PIN) : buzzer_off(BUZZER_HIGH_PIN);
}

// =====================
// DISPLAY OLED 
// =====================
void display_init() {
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();
    
    calculate_render_area_buffer_length(&frame_area);
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    render_on_display(ssd1306_buffer, &frame_area);
}

void display_update(const char *msg) {
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd1306_buffer, 0, 0, msg);
    render_on_display(ssd1306_buffer, &frame_area);
}

// =====================
// CONTROLE DE BOTÕES
// =====================
void button_isr(uint gpio, uint32_t events) {
    Button *btn = (gpio == BUTTON_EMERGENCY) ? &alarm_system.btn_emergency : &alarm_system.btn_reset;
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(btn->last_trigger, now) > DEBOUNCE_TIME_MS * 1000) {
        btn->pressed = true;
        btn->last_trigger = now;
    }
}

// =====================
// DETECÇÃO DE RUIDOS
// =====================
void detect_noise() {
    adc_select_input(2);
    uint16_t mic_value = adc_read();
    if (mic_value > NOISE_THRESHOLD) {
        alarm_system.alarm_active = true;
        alarm_system.needs_display_update = true;
    }
}

// =====================
// INICIALIZAÇÃO DO SISTEMA
// =====================
void system_init() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(MICROPHONE_PIN);

    ledmatrix_init(&alarm_system.matrix, LED_MATRIX_PIN);

    buzzer_init(BUZZER_HIGH_PIN, BUZZER_HIGH_FREQ);
    buzzer_init(BUZZER_LOW_PIN, BUZZER_LOW_FREQ);

    display_init();

    gpio_init(BUTTON_EMERGENCY);
    gpio_set_dir(BUTTON_EMERGENCY, GPIO_IN);
    gpio_pull_up(BUTTON_EMERGENCY);
    gpio_set_irq_enabled_with_callback(BUTTON_EMERGENCY, GPIO_IRQ_EDGE_FALL, true, &button_isr);

    gpio_init(BUTTON_RESET);
    gpio_set_dir(BUTTON_RESET, GPIO_IN);
    gpio_pull_up(BUTTON_RESET);
    gpio_set_irq_enabled_with_callback(BUTTON_RESET, GPIO_IRQ_EDGE_FALL, true, &button_isr);

    alarm_system.alarm_active = false;
    alarm_system.needs_display_update = true;
    strcpy(alarm_system.display_message, "Sistema Pronto");
    display_update(alarm_system.display_message);
}

// =====================
// LÓGICA PRINCIPAL
// =====================
int main() {
    system_init();
    absolute_time_t last_update = get_absolute_time();
    npLED_t red = {.G = 0, .R = 255, .B = 0};
    npLED_t off = {0};

    while (true) {
        if (absolute_time_diff_us(last_update, get_absolute_time()) > 100000) {
            if (alarm_system.needs_display_update) {
                display_update(alarm_system.alarm_active ? "ALARME ATIVADO!" : "Sistema Pronto");
                alarm_system.needs_display_update = false;
            }
            last_update = get_absolute_time();
        }

        if (alarm_system.btn_emergency.pressed) {
            alarm_system.alarm_active = true;
            alarm_system.btn_emergency.pressed = false;
            alarm_system.needs_display_update = true;
        }
        if (alarm_system.btn_reset.pressed) {
            alarm_system.alarm_active = false;
            alarm_system.btn_reset.pressed = false;
            ledmatrix_show_pattern(&alarm_system.matrix, &off);
            buzzer_off(BUZZER_HIGH_PIN);
            buzzer_off(BUZZER_LOW_PIN);
            alarm_system.needs_display_update = true;
        }

        detect_noise();

        if (alarm_system.alarm_active) {
            static absolute_time_t last_toggle;
            static bool blink_state = false;
            
            if (absolute_time_diff_us(last_toggle, get_absolute_time()) > ALARM_BLINK_INTERVAL_MS * 1000) {
                last_toggle = get_absolute_time();
                blink_state = !blink_state;
                
                blink_state ? ledmatrix_show_pattern(&alarm_system.matrix, &red) : 
                            ledmatrix_show_pattern(&alarm_system.matrix, &off);
                
                buzzer_siren();
            }
        }

        sleep_ms(100);
    }
    return 0;
}