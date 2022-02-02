// Plateny de Brito Ponchet

#include <Arduino.h>
#include <WiFi.h>
#include "driver/i2s.h"
#include "webserver.h"
#include "arduinoFFT.h"

#define I2S_SAMPLE_RATE 44000
#define ADC_INPUT       ADC1_CHANNEL_4 //pin 32
#define OUTPUT_PIN      27
#define OUTPUT_VALUE    3800
#define READ_DELAY      9000 //microseconds
#define SAMPLES         512


const char *ssid = "Yagua-Config";
const char *passphrase = "yagua123";

IPAddress local_IP(10,200,200,1);
IPAddress gateway(10,200,200,1);
IPAddress subnet(255,255,255,0);


arduinoFFT FFT = arduinoFFT();

double fftResult[SAMPLES] = {0};

uint16_t buffer[SAMPLES] = {0};
double vReal[SAMPLES] = {0};

void i2sInit()
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate =  I2S_SAMPLE_RATE,              // The format of the signal using ADC_BUILT_IN
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    i2s_driver_install(
        I2S_NUM_0,
        &i2s_config,
        0,
        NULL
    );
    i2s_set_clk(
        I2S_NUM_0,
        I2S_SAMPLE_RATE,
        I2S_BITS_PER_SAMPLE_16BIT,
        I2S_CHANNEL_MONO
    );
    i2s_set_adc_mode(
        ADC_UNIT_1,
        ADC_INPUT
    );
    i2s_adc_enable(I2S_NUM_0);
}

void reader(void *pvParameters) {
    size_t bytes_read;

    while (true) {
        double vImag[SAMPLES] = {0};
        i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytes_read, 15);

        for (int i=0; i < SAMPLES; i++) {
            vReal[i] = (double)(buffer[i]&0x0FFF)/4095;
        }

        FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

        for (int i=0; i < SAMPLES; i++) {
            fftResult[i] = vReal[i];
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);

    Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

    Serial.print("Setting soft-AP ... ");
    Serial.println(WiFi.softAP(ssid,passphrase) ? "Ready" : "Failed!");

    Serial.print("Soft-AP IP address = ");
    Serial.println(WiFi.softAPIP());

    // Initialize the I2S peripheral
    i2sInit();
    // Create a task that will read the data
    xTaskCreatePinnedToCore(reader, "ADC_reader", 16384, NULL, 1, NULL, 1);

    webserverSetup(vReal);
}

void loop() {
    vTaskDelete(NULL);
}