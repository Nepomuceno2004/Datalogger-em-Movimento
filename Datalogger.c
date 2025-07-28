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

#include "ff.h"
#include "diskio.h"
#include "hw_config.h"
#include "sd_card.h"
#include "f_util.h"
#include "my_debug.h"

static const char *nome_arquivo = "dados.csv";

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
SemaphoreHandle_t xSemBotaoB;

volatile uint32_t last_time;        // armazena o tempo do último clique nos botões
volatile bool sensor_state = false; // estado do sensor, inicia desligado
volatile bool ready = true;         // estado de prontidão do sistema
volatile bool capture = false;
volatile bool error = false;
volatile bool sd_mount = false;
volatile bool sd_mounting = false;
volatile bool sd_writing = false;
volatile int numero_amostra = 0;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time > 200)
    {
        if (gpio == BOTAO_A) // Se o botão estiver pressionado
        {
            sensor_state = !sensor_state; // Alterna o estado do sensor
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

static sd_card_t *sd_get_by_name(const char *const name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
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
    while (true)
    {

        if (sensor_state && sd_mount && ready)
        {
            ready = false;  // Indica que o sistema não está pronto para capturar dados
            capture = true; // Alterna o estado de captura

            mpu6050_read_raw(I2C_PORT, MPU6050_DEFAULT_ADDR, acel, giro, &temp);

            // --- Cálculos para Acelerômetro ---
            float ax = acel[0] / 16384.0f;
            float ay = acel[1] / 16384.0f;
            float az = acel[2] / 16384.0f;

            // --- Cálculos para Giroscópio ---
            float gx = giro[0] / 131.0f;
            float gy = giro[1] / 131.0f;
            float gz = giro[2] / 131.0f;

            float temp_c = (temp / 340.0f) + 15.0f;

            printf("Acel: X=%.2fg Y=%.2fg Z=%.2fg\n", ax, ay, az);
            printf("Giro: X=%.2f°/s Y=%.2f°/s Z=%.2f°/s\n", gx, gy, gz);
            printf("Temp: %.2f °C\n", temp_c);
            printf("---------------------------\n");

            vTaskDelay(pdMS_TO_TICKS(800)); // Delay to avoid flooding the output

            capture = false; // Alterna o estado de captura

            sd_writing = true; // Indica que o sistema está escrevendo no SD

            // --- Escreve os dados no SD ---
            FIL file;
            FRESULT fr;

            fr = f_open(&file, nome_arquivo, FA_WRITE | FA_OPEN_APPEND);
            if (fr == FR_OK)
            {
                char linha[128];
                snprintf(linha, sizeof(linha), "%d;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
                         numero_amostra, ax, ay, az, gx, gy, gz);

                UINT bw;
                fr = f_write(&file, linha, strlen(linha), &bw);

                if (fr == FR_OK)
                {
                    // Incrementa o numero_amostra ador de leituras
                    numero_amostra++;
                }
                else
                {
                    printf("[ERRO] Falha ao escrever no arquivo: %d\n", fr);
                }

                f_close(&file);
            }
            else
            {
                printf("[ERRO] Falha ao abrir o arquivo: %d\n", fr);
            }

            // --- Finaliza a escrita no SD ---
            printf("[SD] Dados gravados no arquivo %s\n", nome_arquivo);
            vTaskDelay(pdMS_TO_TICKS(800)); // Delay to avoid flooding the output

            sd_writing = false; // Indica que o sistema terminou de escrever no SD
            ready = true;       // Indica que o sistema está pronto para capturar dados

            vTaskDelay(pdMS_TO_TICKS(2000)); // Delay to avoid flooding the output
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
        else if (error)
        {
            gpio_put(LED_PIN_BLUE, true);   // Liga o LED azul
            gpio_put(LED_PIN_RED, true);    // Liga o LED vermelho
            vTaskDelay(pdMS_TO_TICKS(200)); // Delay para evitar flooding
            gpio_put(LED_PIN_BLUE, false);  // Desliga o LED azul
            gpio_put(LED_PIN_RED, false);   // Desliga o LED vermelho
        }
        else if (sd_mounting)
        {
            gpio_put(LED_PIN_RED, true);   // Liga o LED vermelho
            gpio_put(LED_PIN_GREEN, true); // Liga o LED verde
        }
        else if (sd_writing)
        {
            gpio_put(LED_PIN_BLUE, true);   // Liga o LED azul
            vTaskDelay(pdMS_TO_TICKS(200)); // Delay para evitar flooding
            gpio_put(LED_PIN_BLUE, false);  // Desliga o LED azul
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay para evitar flooding
    }
}

void vDisplayTask(void *params)
{
    ssd1306_t ssd;
    init_Display(&ssd);

    while (true)
    {
        ssd1306_fill(&ssd, false); // Limpa a tela

        if (!sd_mount)
        {
            ssd1306_draw_string(&ssd, "Monte o SD", 5, 30);
        }
        else if (sd_mounting)
        {
            if (sd_mount)
            {
                ssd1306_draw_string(&ssd, "Montando SD...", 5, 30);
            }
            else
            {
                ssd1306_draw_string(&ssd, "Desmontando SD...", 5, 30);
            }
        }
        else if (ready && sensor_state)
        {
            ssd1306_draw_string(&ssd, "Sistema", 5, 20);
            ssd1306_draw_string(&ssd, "Ligado", 5, 30);
        }
        else if (ready)
        {
            ssd1306_draw_string(&ssd, "Sistema", 5, 20);
            ssd1306_draw_string(&ssd, "Pronto", 5, 30);
        }
        else if (capture)
        {
            ssd1306_draw_string(&ssd, "Sensor", 5, 20);
            ssd1306_draw_string(&ssd, "Capturando...", 5, 30);
        }
        else if (error)
        {
            ssd1306_draw_string(&ssd, "Erro no SD Card", 5, 30);
        }
        else if (sd_writing)
        {
            ssd1306_draw_string(&ssd, "Gravando SD...", 5, 30);
        }
        else
        {
            ssd1306_draw_string(&ssd, "Estado desconhecido", 5, 30);
        }
        ssd1306_send_data(&ssd);        // Envia os dados para o display
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay para evitar flooding
    }
}

int criar_cabecalho_csv()
{
    FIL file;
    FRESULT fr;

    // Tenta criar o arquivo NOVO com cabeçalho
    fr = f_open(&file, nome_arquivo, FA_WRITE | FA_CREATE_NEW);
    if (fr == FR_OK)
    {
        const char *cabecalho = "numero_amostra;accel_x;accel_y;accel_z;giro_x;giro_y;giro_z\n";
        UINT bw;
        f_write(&file, cabecalho, strlen(cabecalho), &bw);
        f_close(&file);
        printf("[INFO] Arquivo criado com cabeçalho.\n");
        return 1; // começa do zero
    }
    else if (fr == FR_EXIST)
    {
        printf("[INFO] Arquivo %s já existe. Lendo última amostra...\n", nome_arquivo);

        // Abre para leitura
        fr = f_open(&file, nome_arquivo, FA_READ);
        if (fr != FR_OK)
        {
            printf("[ERRO] Falha ao abrir o arquivo existente para leitura: %d\n", fr);
            return numero_amostra;
        }

        // Move o ponteiro para o fim do arquivo
        f_lseek(&file, f_size(&file));

        // Começa a voltar até ennumero_amostra rar a última quebra de linha
        char c;
        DWORD pos = f_tell(&file);
        int found_newline = 0;

        while (pos > 0)
        {
            pos--;
            f_lseek(&file, pos);
            f_read(&file, &c, 1, NULL);
            if (c == '\n' && found_newline)
                break;
            if (c == '\n')
                found_newline = 1;
        }

        // Agora lê a última linha
        char ultima_linha[128] = {0};
        UINT br;
        f_read(&file, ultima_linha, sizeof(ultima_linha) - 1, &br);
        f_close(&file);

        // A linha deve estar no formato: numero_amostra;...
        int ultimo_numero = 0;
        sscanf(ultima_linha, "%d", &ultimo_numero);
        printf("[INFO] Último número de amostra: %d\n", ultimo_numero);

        return ultimo_numero + 1;
    }
    else
    {
        printf("[ERRO] Falha ao criar ou abrir o arquivo: %d\n", fr);
        return numero_amostra;
    }
}

void vMontagemTask(void *params)
{
    while (true)
    {
        if (xSemaphoreTake(xSemBotaoB, portMAX_DELAY) == pdTRUE)
        {
            const char *drive = sd_get_by_num(0)->pcName;
            FATFS *p_fs = &sd_get_by_num(0)->fatfs;
            sd_card_t *pSD = sd_get_by_name(drive);

            sensor_state = false; // Desliga o sensor durante a montagem/desmontagem
            ready = false;        // Indica que o sistema não está pronto para capturar dados
            capture = false;      // Desativa a captura de dados
            sd_mounting = true;   // Indica que o sistema está montando/desmontando o SD

            if (!sd_mount)
            {
                FRESULT fr = f_mount(p_fs, drive, 1);
                if (fr == FR_OK)
                {
                    error = false;
                    pSD->mounted = true;
                    sd_mount = true;

                    printf("[MONTAGEM] Cartão SD montado com sucesso.\n");

                    vTaskDelay(pdMS_TO_TICKS(800));         // Delay para evitar flooding
                    sd_mounting = false;                    // Indica que o sistema terminou de montar/desmontar o SD
                    ready = true;                           // Indica que o sistema está pronto para capturar dados
                    numero_amostra = criar_cabecalho_csv(); // Cria o cabeçalho do CSV se não existir
                }
                else
                {
                    printf("[ERRO] Falha ao montar o cartão: %s (%d)\n", FRESULT_str(fr), fr);
                    error = true;
                }
            }
            else
            {
                FRESULT fr = f_unmount(drive);
                if (fr == FR_OK)
                {
                    error = false;
                    pSD->mounted = false;
                    pSD->m_Status |= STA_NOINIT;
                    sd_mount = false;
                    printf("[DESMONTAGEM] Cartão SD desmontado com sucesso.\n");

                    vTaskDelay(pdMS_TO_TICKS(800)); // Delay para evitar flooding

                    sd_mounting = false; // Indica que o sistema terminou de montar/desmontar o SD
                    ready = true;        // Indica que o sistema está pronto para capturar dados
                }
                else
                {
                    error = true;
                    printf("[ERRO] Falha ao desmontar o cartão: %s (%d)\n", FRESULT_str(fr), fr);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay para evitar flooding
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
    xSemBotaoB = xSemaphoreCreateBinary();

    xTaskCreate(vCapturaTask, "Captura Task", 512, NULL, 1, NULL);
    xTaskCreate(vLedsTask, "Leds Task", 256, NULL, 1, NULL);
    xTaskCreate(vMontagemTask, "Montagem Task", 512, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 1, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
