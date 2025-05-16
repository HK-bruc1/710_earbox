#include "app_config.h"
#include "cpu/includes.h"
#include "asm/charge.h"
#include "system/init.h"
#include "asm/power_interface.h"
#include "gpio.h"

#if TCFG_CHARGE_ENABLE
static void vpwr_wakeup_callback(P33_IO_WKUP_EDGE edge)
{
    ldoin_wakeup_isr();
}

static const struct _p33_io_wakeup_config vbat_port = {
    .edge               = BOTH_EDGE,        //唤醒方式选择,可选：上升沿\下降沿\双边沿
    .filter      		= PORT_FLT_16ms,
    .gpio               = IO_VBTCH_DET,     //唤醒口选择
    .callback			= vpwr_wakeup_callback,
};

static const struct _p33_io_wakeup_config ldoin_port = {
    .edge               = BOTH_EDGE,        //唤醒方式选择,可选：上升沿\下降沿\双边沿
    .filter      		= PORT_FLT_16ms,
    .gpio               = IO_LDOIN_DET,     //唤醒口选择
    .callback			= vpwr_wakeup_callback,
};

__INITCALL_BANK_CODE
void charge_wakeup_init()
{
    p33_io_wakeup_port_init(&vbat_port);
    p33_io_wakeup_enable(IO_VBTCH_DET, 1);

    p33_io_wakeup_port_init(&ldoin_port);
    p33_io_wakeup_enable(IO_LDOIN_DET, 1);
}

static const struct charge_platform_data charge_data  = {
    .charge_en              = TCFG_CHARGE_ENABLE,                       //内置充电使能
    .charge_poweron_en      = TCFG_CHARGE_POWERON_ENABLE,               //是否支持充电开机
    .charge_full_V          = TCFG_CHARGE_FULL_V,                       //充电截止电压
    .charge_full_mA			= TCFG_CHARGE_FULL_MA,                      //充电截止电流
    .charge_mA				= TCFG_CHARGE_MA | TCFG_CHARGE_DIV,         //充电电流
    .charge_trickle_mA		= TCFG_CHARGE_TRICKLE_MA | TCFG_CHARGE_TRICKLE_DIV,//涓流电流

    /* ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,
     * ldoin < 0.6V且时间大于过滤时间才认为拔出
     * 对于充满直接从5V掉到0V的充电仓，该值必须设置成0
     * 对于充满由5V先掉到0V之后再升压到xV的 充电仓，需要根据实际情况设置该值大小
     * */
    .ldo5v_off_filter		= TCFG_LDOIN_OFF_FILTER_TIME / 2,
    .ldo5v_on_filter        = TCFG_LDOIN_ON_FILTER_TIME / 2,
    .ldo5v_keep_filter      = TCFG_LDOIN_KEEP_FILTER_TIME / 2,
    .ldo5v_pulldown_lvl     = TCFG_LDOIN_PULLDOWN_LEV,
    .ldo5v_pulldown_keep    = TCFG_LDOIN_PULLDOWN_KEEP,

    /*
     * 1. 对于自动升压充电舱,若充电舱需要更大的负载才能检测到插入时，请将该变量置1
          并且根据需求配置下拉电阻档位
     * 2. 对于按键升压,并且是通过上拉电阻去提供维持电压的舱,请将该变量设置1,
          并且根据舱的上拉配置下拉需要的电阻挡位
     * 3. 对于常5V的舱,可将改变量设为0,省功耗
     */
    .ldo5v_pulldown_en		= TCFG_LDOIN_PULLDOWN_EN,
};

__INITCALL_BANK_CODE
int board_charge_init(void)
{
    charge_wakeup_init();

    charge_init(&charge_data);

    if (get_charge_online_flag()) {
        power_set_mode(PWR_LDO15);
    } else {
        power_set_mode(TCFG_LOWPOWER_POWER_SEL);
    }
    return 0;
}
static void board_charge_uninit()
{
    p33_io_wakeup_enable(IO_CHGFL_DET, 0);
}

platform_initcall(board_charge_init);

platform_uninitcall(board_charge_uninit);
#else

//没开启充电时,关闭漏电寄存器(约2uA)
int board_charge_init()
{
    CHG_VILOOP_EN(0);
    CHG_VILOOP2_EN(0);
    return 0;
}
platform_initcall(board_charge_init);

#endif

