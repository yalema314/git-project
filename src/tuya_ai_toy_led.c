#include "tuya_ai_toy_led.h"
#include "tuya_led.h"
#include "tkl_gpio.h"
#include "tal_log.h"

// led handle
STATIC LED_HANDLE s_ai_toy_led = NULL;

void tuya_ai_toy_led_on(void)
{
    if (s_ai_toy_led) {
        tuya_set_led_light_type(s_ai_toy_led, OL_HIGH, 200, 0xFFFF);
    }
}

void tuya_ai_toy_led_off(void)
{
    if (s_ai_toy_led) {
        tuya_set_led_light_type(s_ai_toy_led, OL_LOW, 200, 0xFFFF);
    }
}

void tuya_ai_toy_led_flash(int time)
{
    if (s_ai_toy_led) {
        tuya_set_led_light_type(s_ai_toy_led, OL_FLASH_LOW, time, 0xFFFF);
    }
}

OPERATE_RET tuya_ai_toy_led_init(TUYA_GPIO_NUM_E led_pin)
{
    OPERATE_RET rt = OPRT_OK;

    //! led init
    TAL_PR_DEBUG("ai toy -> init led %d", led_pin);
    if (led_pin != TUYA_GPIO_NUM_MAX) {
        TUYA_CALL_ERR_RETURN(tuya_create_led_handle(led_pin, OL_HIGH, &s_ai_toy_led));
    }

    return rt;
}