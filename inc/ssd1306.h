#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include "hardware/i2c.h"

// Estrutura que representa o dispositivo SSD1306.
typedef struct {
    i2c_inst_t *i2c;
    uint8_t address;
    uint8_t width;
    uint8_t height;
    // Buffer de vídeo: para um display 128x64, o tamanho é (128 * 64 / 8 = 1024 bytes).
    uint8_t buffer[1024];
} ssd1306_t;

// Declaração das funções do driver SSD1306
void ssd1306_init(ssd1306_t *dev, i2c_inst_t *i2c, uint8_t address, uint8_t width, uint8_t height);
void ssd1306_clear(ssd1306_t *dev);
void ssd1306_show(ssd1306_t *dev);
void ssd1306_draw_pixel(ssd1306_t *dev, int x, int y, uint8_t color);
void ssd1306_draw_rect(ssd1306_t *dev, int x, int y, int w, int h, uint8_t color);
void ssd1306_fill_rect(ssd1306_t *dev, int x, int y, int w, int h, uint8_t color);

// Funções para desenhar caracteres e strings usando fontes definidas em fonte.h
void ssd1306_draw_char(ssd1306_t *dev, int x, int y, char ch, uint8_t color);
void ssd1306_draw_string(ssd1306_t *dev, int x, int y, const char *str, uint8_t color);

#endif  // SSD1306_H