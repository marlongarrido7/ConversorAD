#include "ssd1306.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "fonte.h"  // Inclui as definições de fontes

#define SSD1306_CMD  0x00
#define SSD1306_DATA 0x40

// Função auxiliar para enviar um comando ao display
static void ssd1306_send_command(ssd1306_t *dev, uint8_t cmd) {
    uint8_t data[2] = { SSD1306_CMD, cmd };
    i2c_write_blocking(dev->i2c, dev->address, data, 2, false);
}

void ssd1306_init(ssd1306_t *dev, i2c_inst_t *i2c, uint8_t address, uint8_t width, uint8_t height) {
    dev->i2c = i2c;
    dev->address = address;
    dev->width = width;
    dev->height = height;
    // Zera o buffer
    for (int i = 0; i < (width * height / 8); i++) {
        dev->buffer[i] = 0x00;
    }

    // Sequência de inicialização do SSD1306
    sleep_ms(100);
    ssd1306_send_command(dev, 0xAE); // Desliga o display
    ssd1306_send_command(dev, 0xD5); // Configura divisão do clock
    ssd1306_send_command(dev, 0x80);
    ssd1306_send_command(dev, 0xA8); // Configura multiplex
    ssd1306_send_command(dev, height - 1);
    ssd1306_send_command(dev, 0xD3); // Configura deslocamento
    ssd1306_send_command(dev, 0x00);
    ssd1306_send_command(dev, 0x40); // Configura linha de início
    ssd1306_send_command(dev, 0x8D); // Habilita charge pump
    ssd1306_send_command(dev, 0x14);
    ssd1306_send_command(dev, 0x20); // Modo de endereçamento da memória
    ssd1306_send_command(dev, 0x00); // Horizontal
    ssd1306_send_command(dev, 0xA1); // Remapeamento de segmentos
    ssd1306_send_command(dev, 0xC8); // Direção de scan de COM remapeada
    ssd1306_send_command(dev, 0xDA); // Configura pinos COM
    ssd1306_send_command(dev, 0x12);
    ssd1306_send_command(dev, 0x81); // Configura contraste
    ssd1306_send_command(dev, 0xCF);
    ssd1306_send_command(dev, 0xD9); // Configura período de pré-carga
    ssd1306_send_command(dev, 0xF1);
    ssd1306_send_command(dev, 0xDB); // Configura nível de VCOMH
    ssd1306_send_command(dev, 0x40);
    ssd1306_send_command(dev, 0xA4); // Retoma conteúdo da RAM
    ssd1306_send_command(dev, 0xA6); // Exibe no modo normal
    ssd1306_send_command(dev, 0xAF); // Liga o display

    // Atualiza o display com o buffer zerado
    ssd1306_show(dev);
}

void ssd1306_clear(ssd1306_t *dev) {
    for (int i = 0; i < (dev->width * dev->height / 8); i++) {
        dev->buffer[i] = 0x00;
    }
}

void ssd1306_show(ssd1306_t *dev) {
    // Define o endereço das colunas e páginas
    ssd1306_send_command(dev, 0x21); // Colunas
    ssd1306_send_command(dev, 0);    
    ssd1306_send_command(dev, dev->width - 1);

    ssd1306_send_command(dev, 0x22); // Páginas
    ssd1306_send_command(dev, 0);
    ssd1306_send_command(dev, (dev->height / 8) - 1);

    // Envia o buffer para o display
    for (int i = 0; i < (dev->width * dev->height / 8); i++) {
        uint8_t data[2] = { SSD1306_DATA, dev->buffer[i] };
        i2c_write_blocking(dev->i2c, dev->address, data, 2, false);
    }
}

void ssd1306_draw_pixel(ssd1306_t *dev, int x, int y, uint8_t color) {
    if (x < 0 || x >= dev->width || y < 0 || y >= dev->height)
        return;
    int byte_index = x + (y / 8) * dev->width;
    uint8_t bit_mask = 1 << (y % 8);
    if (color)
        dev->buffer[byte_index] |= bit_mask;
    else
        dev->buffer[byte_index] &= ~bit_mask;
}

void ssd1306_draw_rect(ssd1306_t *dev, int x, int y, int w, int h, uint8_t color) {
    // Linhas horizontais
    for (int i = x; i < x + w; i++) {
        ssd1306_draw_pixel(dev, i, y, color);
        ssd1306_draw_pixel(dev, i, y + h - 1, color);
    }
    // Linhas verticais
    for (int i = y; i < y + h; i++) {
        ssd1306_draw_pixel(dev, x, i, color);
        ssd1306_draw_pixel(dev, x + w - 1, i, color);
    }
}

void ssd1306_fill_rect(ssd1306_t *dev, int x, int y, int w, int h, uint8_t color) {
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            ssd1306_draw_pixel(dev, i, j, color);
        }
    }
}

// Função para desenhar um único caractere usando a fonte definida em fonte.h
void ssd1306_draw_char(ssd1306_t *dev, int x, int y, char ch, uint8_t color) {
    const uint8_t (*font)[5] = 0;
    uint8_t index = 0;
    
    if (ch >= 'A' && ch <= 'Z') {
        font = font_uppercase;
        index = ch - 'A';
    } else if (ch >= 'a' && ch <= 'z') {
        font = font_lowercase;
        index = ch - 'a';
    } else if (ch >= '0' && ch <= '9') {
        font = font_numbers;
        index = ch - '0';
    } else {
        // Caractere não suportado; não desenha nada (ou pode desenhar um espaço)
        return;
    }
    
    // Desenha cada uma das 5 colunas do caractere
    for (int col = 0; col < 5; col++) {
        uint8_t colData = font[index][col];
        // Desenha 8 linhas para cada coluna (a fonte é 5x7; usamos 8 linhas para simplificar)
        for (int row = 0; row < 8; row++) {
            if (colData & (1 << row)) {
                ssd1306_draw_pixel(dev, x + col, y + row, color);
            }
        }
    }
}

// Função para desenhar uma string de caracteres
void ssd1306_draw_string(ssd1306_t *dev, int x, int y, const char *str, uint8_t color) {
    while (*str) {
        ssd1306_draw_char(dev, x, y, *str, color);
        x += 6; // 5 pixels para o caractere + 1 pixel de espaçamento
        str++;
    }
}
