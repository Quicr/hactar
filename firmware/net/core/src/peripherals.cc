#include "peripherals.hh"


#include "net.hh"

extern QueueHandle_t uart_queue;

void InitializeGPIO()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_STAT_MASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(NET_STAT, 0);

    // Debug init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_DEBUG_MASK;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);
}

std::unique_ptr<Serial> InitializeQueuedUART(const uart_config_t& uart_config,
    const uart_port_t& uart_port,
    QueueHandle_t& uart_queue,
    const int rx_buff_size,
    const int tx_buff_size,
    const int event_queue_size,
    const int tx_pin,
    const int rx_pin,
    const int rts_pin,
    const int cts_pin,
    const long int intr_priority,
    const uint16_t serial_tx_task_sz,
    const uint16_t serial_rx_task_sz,
    const uint16_t serial_ring_tx_num,
    const uint16_t serial_ring_rx_num
)
{

    esp_err_t res;
    // res = uart_driver_install(uart_port, rx_buff_size, tx_buff_size, event_queue_size, &uart_queue, intr_priority);
    // ESP_LOGI("Initialize UART", "install res=%d\n", res);
    res = uart_driver_install(uart_port, rx_buff_size, tx_buff_size, 0, NULL, intr_priority);
    ESP_LOGI("Initialize UART", "install res=%d\n", res);

    res = uart_set_pin(uart_port, tx_pin, rx_pin, rts_pin, cts_pin);
    ESP_LOGI("Initialize UART", "uart set pin res=%d\n", res);

    res = uart_param_config(uart_port, &uart_config);
    ESP_LOGI("Initialize UART", "install res=%d\n", res);

    return std::make_unique<Serial>(uart_port, uart_queue,
        serial_tx_task_sz, serial_rx_task_sz,
        serial_ring_tx_num, serial_ring_rx_num);
}

void IntitializeLEDs()
{
    gpio_config_t io_conf = {};
    // LED init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = LED_MASK;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);
}

void IntitializePWM()
{
    constexpr ledc_timer_t ledc_timer = LEDC_TIMER_0;
    constexpr ledc_mode_t ledc_mode = LEDC_LOW_SPEED_MODE;
    constexpr int ledc_io = NET_LED_R;
    constexpr ledc_channel_t ledc_channel = LEDC_CHANNEL_0;
    constexpr ledc_timer_bit_t ledc_duty_res = LEDC_TIMER_8_BIT;
    constexpr int ledc_duty = 255;
    constexpr int ledc_freq = 8000;

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer_c = {
        .speed_mode = ledc_mode,
        .duty_resolution = ledc_duty_res,
        .timer_num = ledc_timer,
        .freq_hz = ledc_freq,  // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_c));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel_c = {
        .gpio_num = ledc_io,
        .speed_mode = ledc_mode,
        .channel = ledc_channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = ledc_timer,
        .duty = 0, // Set duty to 0%
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_c));
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(ledc_mode, ledc_channel, ledc_duty));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(ledc_mode, ledc_channel));
}