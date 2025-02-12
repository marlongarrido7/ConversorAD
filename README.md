
---

# Uso de Conversor Analógico / Digital em Microcontrolador Raspberry Pico W 2040  
**Display SSD1306, Joystick, Lâmpadas RGB e Acessórios**

---

## Autor do Projeto  
**Dr. Marlon da Silva Garrido**  
Professor Associado IV (CENAMB - PPGEA)  
Universidade Federal do Vale do São Francisco (UNIVASF)

---

## Contexto e Objetivos

Este projeto foi desenvolvido no âmbito das atividades do **EMBARCATECH 2024/25**, com o objetivo de consolidar os conceitos de utilização de conversores analógico-digitais (ADC) no **RP2040**, assim como explorar as funcionalidades da placa de desenvolvimento **BitDogLab**.

### Objetivos Específicos:
- Compreender o funcionamento do conversor analógico-digital (ADC) no RP2040.
- Utilizar PWM para controlar a intensidade de dois LEDs RGB com base nos valores do joystick.
- Representar a posição do joystick no display SSD1306 por meio de um quadrado móvel.
- Aplicar o protocolo de comunicação **I2C** para integração com o display.

---

## Descrição do Projeto

### Funcionalidades Principais:
1. **Controle de Brilho dos LEDs RGB via PWM**  
   - **LED Azul**: Controlado pelo eixo Y do joystick. O brilho aumenta gradualmente ao mover o joystick para cima ou para baixo.  
   - **LED Vermelho**: Controlado pelo eixo X do joystick, seguindo a mesma lógica do LED Azul.

2. **Exibição no Display SSD1306**  
   - Um quadrado de 8x8 pixels se move proporcionalmente aos valores do joystick, sendo inicialmente centralizado no display.

3. **Botões e Interrupções**  
   - O **Botão do Joystick** alterna o estado do LED Verde e modifica o estilo da borda do display.  
   - O **Botão A** ativa ou desativa os LEDs PWM a cada acionamento.

---

## Diagrama Simplificado de Conexões

```
+--------------------------------------------------------+
|                Placa BitDogLab / Pico                  |
|                                                        |
| GPIO 11  -----------------------> LED RGB (Verde)      |
| GPIO 12  -----------------------> LED RGB (Azul)       |
| GPIO 13  -----------------------> LED RGB (Vermelho)   |
| GPIO 22  -----------------------> Botão do Joystick    |
| GPIO 26  -----------------------> Eixo Y do Joystick   |
| GPIO 27  -----------------------> Eixo X do Joystick   |
| GPIO 5   -----------------------> Botão A              |
| GPIO 14  -----------------------> SDA (Display OLED)   |
| GPIO 15  -----------------------> SCL (Display OLED)   |
+--------------------------------------------------------+
```

---

## Como Executar o Projeto

### Passos:
1. **Clonar o Repositório**:
   ```bash
   git clone https://github.com/seu-usuario/ProjetoConversorAD.git
   cd ProjetoConversorAD
   ```

2. **Configurar o Ambiente de Desenvolvimento**:
   - Instale o Raspberry Pico SDK.
   - Verifique as dependências necessárias (bibliotecas para I2C, ADC e PWM).

3. **Compilar o Projeto**:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

4. **Transferir o Firmware**:
   - Conecte a placa ao computador.
   - Copie o arquivo `.uf2` para o volume USB do dispositivo.

---

## Detalhamento do Código

O código principal integra múltiplas funções, bibliotecas e conceitos, como ADC, PWM, I2C, e tratamento de interrupções.

### Bibliotecas Utilizadas:
```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "ssd1306.h"
```
- **pico/stdlib.h** → Funções básicas do Raspberry Pi Pico (GPIO, temporização).  
- **hardware/adc.h** → Configuração e leitura do conversor analógico-digital (ADC).  
- **hardware/pwm.h** → Controle PWM para ajuste da intensidade dos LEDs RGB.  
- **hardware/i2c.h** → Comunicação com o display SSD1306 via protocolo I2C.  
- **ssd1306.h** → Funções para manipulação gráfica do display OLED.

---

### Variáveis e Definições Importantes:
```c
#define PWM_WRAP_VALUE  4095
#define DEBOUNCE_DELAY_US 200000
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  64
#define SQUARE_SIZE     8
```
- **PWM_WRAP_VALUE**: Valor máximo do contador PWM, definindo a resolução de 12 bits.  
- **DEBOUNCE_DELAY_US**: Define o tempo mínimo entre leituras consecutivas para evitar bouncing nos botões.  
- **DISPLAY_WIDTH** e **DISPLAY_HEIGHT**: Dimensões do display SSD1306 (128x64 pixels).  
- **SQUARE_SIZE**: Tamanho do quadrado que representa a posição do joystick no display.

---

### Funções Explicadas:

1. **Inicialização dos Periféricos**
   - Configuração de GPIOs para LEDs, botões e funções PWM.
   - Inicialização do ADC para leitura dos eixos X e Y do joystick.
   - Configuração do I2C para comunicação com o display SSD1306.

   ```c
   adc_init();
   adc_gpio_init(27);  // Joystick X
   adc_gpio_init(26);  // Joystick Y
   i2c_init(I2C_PORT, 100 * 1000);  // Velocidade de 100 kHz para o I2C
   ```

2. **Função `gpio_callback(uint gpio, uint32_t events)`**  
   Trata as interrupções dos botões, alternando o estado do LED Verde e ativando/desativando o PWM.

   ```c
   if (gpio == BUTTON_A) {
       pwm_enabled = !pwm_enabled;
       if (!pwm_enabled) {
           pwm_set_gpio_level(LED_AZUL, 0);
           pwm_set_gpio_level(LED_VERMELHA, 0);
       }
       printf("Botão A pressionado. PWM %s\n", pwm_enabled ? "Ativado" : "Desativado");
   }
   ```

3. **Leitura do ADC e Controle de LEDs PWM**  
   O valor lido pelo ADC é usado para calcular o nível PWM aplicado aos LEDs Azul e Vermelho. Se o valor estiver dentro da zona morta, o LED correspondente é apagado.

   ```c
   uint32_t pwm_level_blue = ((uint32_t)abs((int)adc_y - 2048) * PWM_WRAP_VALUE) / 2048;
   pwm_set_gpio_level(LED_AZUL, pwm_level_blue > PWM_WRAP_VALUE ? PWM_WRAP_VALUE : pwm_level_blue);
   ```

4. **Função `ssd1306_fill_rect()` para Atualização do Display**  
   Desenha um quadrado no display em posição proporcional aos valores lidos do joystick.

   ```c
   uint8_t pos_x = (uint32_t)adc_x * (DISPLAY_WIDTH - SQUARE_SIZE) / 4095;
   uint8_t pos_y = (uint32_t)(4095 - adc_y) * (DISPLAY_HEIGHT - SQUARE_SIZE) / 4095;
   ssd1306_fill_rect(&display, pos_x, pos_y, SQUARE_SIZE, SQUARE_SIZE, 1);
   ```

5. **Função Principal (`main()`)**  
   Laço principal que:  
   - Lê os valores do joystick.  
   - Atualiza o display OLED.  
   - Controla os LEDs RGB de acordo com o movimento do joystick.  

   ```c
   while (true) {
       adc_select_input(JOYSTICK_X_ADC_CH);
       uint16_t adc_x = adc_read();
       adc_select_input(JOYSTICK_Y_ADC_CH);
       uint16_t adc_y = adc_read();
       // Atualização do display e controle PWM
       sleep_ms(50);
   }
   ```

---

## Referências

- [Raspberry Pi Pico SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf)  
- Documentação oficial do RP2040  

---

## Demonstração em Vídeo

Assista à demonstração completa do projeto:  
[Link do Vídeo](#) *(Em breve)*  

---

## Contato  
📧 **Email**: marlon.garrido@univasf.edu.br  
🌐 **Site**: [UNIVASF](https://www.univasf.edu.br/)

---