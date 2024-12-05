#include "asm/power_interface.h"
#include "app_config.h"
#include "gpio.h"

static struct _p33_io_wakeup_config port0 = {
    .pullup_down_mode = PORT_INPUT_PULLUP_10K,
    .filter      		= PORT_FLT_DISABLE,
    .edge               = FALLING_EDGE,
    .gpio              = IO_PORTB_01,
};

void key_wakeup_init()
{
#if (!TCFG_LP_TOUCH_KEY_ENABLE)
    p33_io_wakeup_port_init(&port0);
    p33_io_wakeup_enable(IO_PORTB_01, 1);

#endif
}
