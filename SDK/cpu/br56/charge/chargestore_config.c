#include "app_config.h"
#include "chargestore/chargestore.h"
#include "asm/charge.h"
#include "gpio_config.h"
#include "system/init.h"
#include "asm/power_interface.h"
#include "gpio.h"

#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE
static void chargestore_wakeup_callback(P33_IO_WKUP_EDGE edge)
{
    chargestore_ldo5v_fall_deal();
}

static const struct _p33_io_wakeup_config port1 = {
    .pullup_down_mode = PORT_INPUT_FLOATING,          	//配置I/O 内部上下拉是否使能
    .edge               = FALLING_EDGE, //唤醒方式选择,可选：上升沿\下降沿
    .filter      		= PORT_FLT_1ms,
    .gpio               = TCFG_CHARGESTORE_PORT,//唤醒口选择
    .callback			= chargestore_wakeup_callback,
};

static const struct chargestore_platform_data chargestore_data = {
    .io_port                = TCFG_CHARGESTORE_PORT,
    .baudrate               = 9600,
    .init                   = chargestore_init,
    .open                   = chargestore_open,
    .close                  = chargestore_close,
    .write                  = chargestore_write,
};

int board_chargestore_config()
{
    p33_io_wakeup_port_init(&port1);
    p33_io_wakeup_enable(TCFG_CHARGESTORE_PORT, 1);

    chargestore_api_init(&chargestore_data);
    return 0;
}
static void board_chargestore_not_config()
{
    p33_io_wakeup_enable(TCFG_CHARGESTORE_PORT, 0);
}

platform_initcall(board_chargestore_config);

platform_uninitcall(board_chargestore_not_config);

#endif
