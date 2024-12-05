#include "audio_dac.h"
#include "media_config.h"
#include "power/power_manage.h"

extern struct audio_dac_hdl dac_hdl;

static u8 audio_dac_idle_query()
{
    if (((dac_hdl.state == DAC_STATE_STOP) && (dac_hdl.anc_dac_open == 1)) ||
        (dac_hdl.state == DAC_STATE_INIT)	||
        (dac_hdl.state == DAC_STATE_POWER_OFF)) {
        return 1;
    }
    return 0;
}

static enum LOW_POWER_LEVEL audio_dac_level_query(void)
{
    /*根据dac的状态选择sleep等级*/
    if (config_audio_dac_power_off_lite) {
        /*anc打开，进入轻量级低功耗*/
        return LOW_POWER_MODE_LIGHT_SLEEP;
    } else {
        /*进入最优低功耗*/
        return LOW_POWER_MODE_SLEEP;
    }
}

REGISTER_LP_TARGET(audio_dac_lp_target) = {
    .name    = "audio_dac",
    .level   = audio_dac_level_query,
    .is_idle = audio_dac_idle_query,
};
