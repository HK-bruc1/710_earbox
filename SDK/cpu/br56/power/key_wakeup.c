#include "asm/power_interface.h"
#include "app_config.h"
#include "gpio.h"
#include "iokey.h"
#include "adkey.h"

static struct _p33_io_wakeup_config port0 = {
    .pullup_down_mode = PORT_INPUT_PULLUP_10K,
    .filter      		= PORT_FLT_DISABLE,
    .edge               = FALLING_EDGE,
    .gpio              = IO_PORTB_04,
};

void key_wakeup_init()
{
#if (!TCFG_LP_TOUCH_KEY_ENABLE)

#if TCFG_ADKEY_ENABLE
    const struct adkey_platform_data *data = get_adkey_platform_data();
    port0.gpio = data->adkey_pin;
#endif

#if TCFG_IOKEY_ENABLE
    const struct iokey_platform_data *data = get_iokey_platform_data();
    u8 wakeup_key_num = 0;      //默认0号按键做唤醒，需要可以自行修改按键号
    if (data->port[wakeup_key_num].connect_way == ONE_PORT_TO_LOW) {
        port0.edge  = FALLING_EDGE;
        port0.gpio  = data->port[wakeup_key_num].key_type.one_io.port;
    } else if (data->port[wakeup_key_num].connect_way == ONE_PORT_TO_HIGH) {
        port0.edge  = RISING_EDGE;
        port0.gpio  = data->port[wakeup_key_num].key_type.one_io.port;
    } else if (data->port[wakeup_key_num].connect_way == DOUBLE_PORT_TO_IO) {
        port0.edge  = FALLING_EDGE;
        port0.gpio  =  data->port[wakeup_key_num].key_type.two_io.in_port;
    }
#endif

    p33_io_wakeup_port_init(&port0);
    p33_io_wakeup_enable(port0.gpio, 1);
#endif
}
