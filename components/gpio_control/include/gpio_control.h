int gpio_init(void);
void led_control(void);

struct button {
    const int   gpio;
    bool        is_pressed;
    int         time_pressed;
};

#define GPIO_TASK_DELAY     200

#define BUTTON_COUNT        2

#define TIMEOUT_RESET       1000
#define TIMEOUT_DEFAULTS    5000
#define TIMEOUT_CH_SWITCH   1000