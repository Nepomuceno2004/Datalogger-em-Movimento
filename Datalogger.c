#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/mpu6050.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "pico/bootrom.h"
#include <math.h>

#define BOTAO_A 5           // pino do botão A
#define BOTAO_B 6           // pino do botão B
#define LED_PIN_GREEN 11    // verde
#define LED_PIN_BLUE 12     // azul
#define LED_PIN_RED 13      // vermelho
#define BUZZER_PIN 21       // pino do buzzer
#define JOYSTICK_BTN_PIN 22 // pino do botão do joystick

#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1

// semáforos utilizados
SemaphoreHandle_t xSemBotaoA;
SemaphoreHandle_t xSemBotaoB;

volatile uint32_t last_time;        // armazena o tempo do último clique nos botões
volatile bool sensor_state = false; // estado do sensor, inicia desligado
volatile bool ready = true;         // estado de prontidão do sistema
volatile bool capture = false;
volatile bool error = false;
volatile int cont = 0;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time > 200)
    {
        if (gpio == BOTAO_A) // Se o botão estiver pressionado
        {
            sensor_state = !sensor_state; // Alterna o estado do sensor
            ready = !ready;               // Alterna o estado de prontidão
            capture = !capture;           // Alterna o estado de captura
        }

        else if (gpio == BOTAO_B) // Se o botão estiver pressionado
        {
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
    // Inicializa I2C
    i2c_init(I2C_PORT, 400 * 1000); // 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa MPU6050
    mpu6050_init(I2C_PORT, MPU6050_DEFAULT_ADDR);
    mpu6050_reset(I2C_PORT, MPU6050_DEFAULT_ADDR);

    int16_t acel[3], giro[3], temp;
    // Variáveis de limiar e last_value para giroscópio
    float last_gx = 0.0f, last_gy = 0.0f, last_gz = 0.0f;
    const float gyro_limiar = 100.0f; // Zona morta para giroscópio (em LSB)
    float last_roll = 0.0f, last_pitch = 0.0f;
    const float accel_limiar = 1.0f; // Zona morta para Pitch/Roll (em graus)

    while (true)
    {
        // Espera até o semáforo de reset ser liberado
        if (sensor_state)
        {
            mpu6050_read_raw(I2C_PORT, MPU6050_DEFAULT_ADDR, acel, giro, &temp);

            // Incrementa o contador de leituras
            cont++;

            // --- Cálculos para Giroscópio ---
            float ax = acel[0] / 16384.0f;
            float ay = acel[1] / 16384.0f;
            float az = acel[2] / 16384.0f;

            // --- Cálculos para Acelerômetro (Pitch/Roll) ---
            float gx = giro[0] / 131.0f;
            float gy = giro[1] / 131.0f;
            float gz = giro[2] / 131.0f;

            float roll = atan2(ay, az) * 180.0f / M_PI;
            float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / M_PI;

            // Atualiza o display apenas se houver mudança significativa em qualquer sensor
            if (fabs(gx - last_gx) > gyro_limiar || fabs(gy - last_gy) > gyro_limiar || fabs(gz - last_gz) > gyro_limiar ||
                fabs(roll - last_roll) > accel_limiar || fabs(pitch - last_pitch) > accel_limiar)
            {

                last_gx = gx;
                last_gy = gy;
                last_gz = gz;
                last_roll = roll;
                last_pitch = pitch;

                float temp_c = (temp / 340.0f) + 15.0f;

                printf("Acel: X=%.2fg Y=%.2fg Z=%.2fg\n", ax, ay, az);
                printf("Giro: X=%.2f°/s Y=%.2f°/s Z=%.2f°/s\n", gx, gy, gz);
                printf("Temp: %.2f °C\n", temp_c);
                printf("---------------------------\n");
                vTaskDelay(pdMS_TO_TICKS(200)); // Delay to avoid flooding the output
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay to avoid flooding the output
    }
}

void vLedsTask(void *params)
{

    while (true)
    {
        gpio_put(LED_PIN_GREEN, false);
        gpio_put(LED_PIN_BLUE, false);
        gpio_put(LED_PIN_RED, false);

        if (ready)
        {
            gpio_put(LED_PIN_GREEN, true); // Liga o LED verde
        }
        else if (capture)
        {
            gpio_put(LED_PIN_RED, true); // Liga o LED vermelho
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay para evitar flooding
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

    // --- Inicializa os LEDs como saída ---

    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);

    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

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

    // Criação dos semáforos
    xSemBotaoA = xSemaphoreCreateBinary();
    xSemBotaoB = xSemaphoreCreateBinary();

    xTaskCreate(vCapturaTask, "Captura Task", 256, NULL, 1, NULL);
    xTaskCreate(vLedsTask, "Leds Task", 256, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
