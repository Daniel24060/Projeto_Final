#include "ssd1306_i2c.h"
extern void calculate_render_area_buffer_length(struct render_area *area);
extern void ssd1306_send_command(uint8_t cmd);
extern void ssd1306_send_command_list(uint8_t *ssd, int number);
extern void ssd1306_send_buffer(uint8_t ssd[], int buffer_length);
extern void ssd1306_init();
extern void ssd1306_scroll(bool set);
extern void render_on_display(uint8_t *ssd, struct render_area *area);
extern void ssd1306_set_pixel(uint8_t *ssd, int x, int y, bool set);
extern void ssd1306_draw_line(uint8_t *ssd, int x_0, int y_0, int x_1, int y_1, bool set);
extern void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character);
void ssd1306_draw_string(uint8_t *buffer, int x, int y, const char *str);
extern void ssd1306_command(ssd1306_t *ssd, uint8_t command);
extern void ssd1306_config(ssd1306_t *ssd);
extern void ssd1306_init_bm(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c);
extern void ssd1306_send_data(ssd1306_t *ssd);
extern void ssd1306_draw_bitmap(ssd1306_t *ssd, const uint8_t *bitmap);
#include <stdint.h>

#define SSD1306_BUFFER_LENGTH 1024  // Exemplo para um display 128x64 (128*64/8)
uint8_t ssd1306_buffer[ssd1306_buffer_length] = {1};

extern uint8_t ssd1306_buffer[SSD1306_BUFFER_LENGTH];

// Declaração da função para limpar o buffer do display
void ssd1306_clear(void);

// Outras funções do seu display...
void ssd1306_update(void); // Função que envia o buffer para o display
