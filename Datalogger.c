#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "pico/bootrom.h"

#define BOTAO_A 5           // pino do botão A
#define BOTAO_B 6           // pino do botão B
#define LED_PIN_GREEN 11    // verde
#define LED_PIN_BLUE 12     // azul
#define LED_PIN_RED 13      // vermelho
#define BUZZER_PIN 21       // pino do buzzer
#define JOYSTICK_BTN_PIN 22 // pino do botão do joystick

// semáforos utilizados
SemaphoreHandle_t xSemBotaoA;
SemaphoreHandle_t xSemBotaoB;

uint32_t last_time; // armazena o tempo do último clique nos botões

// Interrupt handler para o pino A (CLK)
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time > 200)
    {
        if (gpio == BOTAO_A) // Se o botão estiver pressionado
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xSemBotaoA, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        else if (gpio == BOTAO_B) // Se o botão estiver pressionado
        {
            // reset_usb_boot(0, 0);
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xSemBotaoB, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else if (gpio == JOYSTICK_BTN_PIN) // Se o botão estiver pressionado
        {
            reset_usb_boot(0, 0);
        }
        last_time = current_time; // Atualiza o tempo do último clique
    }
}
void vCapturaTask(void *params)
{
    while (true)
    {
        // Espera até o semáforo de reset ser liberado
        if (xSemaphoreTake(xSemBotaoA, portMAX_DELAY) == pdTRUE)
        {
            // Ação a ser executada quando o botão A for pressionado
            printf("Botão A pressionado!\n");
            gpio_put(LED_PIN_BLUE, 1);       // Liga o LED verde
            vTaskDelay(pdMS_TO_TICKS(2000)); // Espera 1 segundo
            gpio_put(LED_PIN_BLUE, 0);       // Desliga o LED verde
        }
    }
}
void vMontagemTask(void *params)
{
    while (true)
    {
        // Espera até o semáforo de reset ser liberado
        if (xSemaphoreTake(xSemBotaoB, portMAX_DELAY) == pdTRUE)
        {
            // Ação a ser executada quando o botão A for pressionado
            printf("Botão B pressionado!\n");
            gpio_put(LED_PIN_RED, 1);        // Liga o LED verde
            vTaskDelay(pdMS_TO_TICKS(2000)); // Espera 1 segundo
            gpio_put(LED_PIN_RED, 0);        // Desliga o LED verde
        }
    }
}

int main()
{
    stdio_init_all();

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    gpio_init(JOYSTICK_BTN_PIN);
    gpio_set_dir(JOYSTICK_BTN_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN_PIN);

    // --- Configura interrupções para os botões na borda de descida ---
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // --- Inicializa os LEDs como saída ---
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);

    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    // --- Garante que todos os LEDs comecem apagados ---
    gpio_put(LED_PIN_GREEN, false);
    gpio_put(LED_PIN_BLUE, false);
    gpio_put(LED_PIN_RED, false);

    // Criação dos semáforos
    xSemBotaoA = xSemaphoreCreateBinary();
    xSemBotaoB = xSemaphoreCreateBinary();

    xTaskCreate(vCapturaTask, "Captura Task", 256, NULL, 1, NULL);
    xTaskCreate(vMontagemTask, "Montagem Task", 256, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
