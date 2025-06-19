/*
 ******************************************************************************************
 *							Audio Config
 *
 * Discription: 音频模块与芯片系列相关配置
 *
 * Notes:
 ******************************************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_config.h"
#include "clock_manager/clock_manager.h"

const u16 dac_digital_gain_tab_version_c[7] = {
    16384,  // 20mW
    16384,  // 30mW
    16766,  // 50mW
    17156,  // 80mW
    17156,  // 100mW
    16766,  // 2Vrms
    17156   // 2Vrms
};
const int config_audio_dac_mute_timeout = 70;   //单位：ms

/*
 *******************************************************************
 *						Audio Dac Config
 *******************************************************************
 */
const int config_audio_dac_trim_fade_enable = AUD_DAC_TRIM_FADE_ENABLE;

/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
const int config_audio_dac_mix_enable = 1;

//***********************
//*		AAC Codec       *
//***********************
const int AAC_DEC_MP4A_LATM_ANALYSIS = 1;
const int AAC_DEC_LIB_SUPPORT_24BIT_OUTPUT = 0;
const int WTS_DEC_LIB_SUPPORT_24BIT_OUTPUT = 1;
//***********************
//*		MP3 Codec       *
//***********************
const int MP3_DEC_LIB_SUPPORT_24BIT_OUTPUT = 0;

//***********************
//*		Audio Params    *
//***********************
void audio_adc_param_fill(struct mic_open_param *mic_param, struct adc_platform_cfg *platform_cfg)
{
    mic_param->mic_mode      = platform_cfg->mic_mode;
    mic_param->mic_ain_sel   = platform_cfg->mic_ain_sel;
    mic_param->mic_bias_sel  = platform_cfg->mic_bias_sel;
    mic_param->mic_bias_rsel = platform_cfg->mic_bias_rsel;
    mic_param->mic_dcc       = platform_cfg->mic_dcc;
    mic_param->mic_dcc_en    = platform_cfg->mic_dcc_en;
}
void audio_linein_param_fill(struct linein_open_param *linein_param, const struct adc_platform_cfg *platform_cfg)
{
    linein_param->linein_mode    = platform_cfg->mic_mode;
    linein_param->linein_ain_sel = platform_cfg->mic_ain_sel;
    linein_param->linein_dcc     = platform_cfg->mic_dcc;
}

/*param:dvol*/
u16 dac_dvol_max_query(void)
{
    u16 DAC_0dB = 16100;
    if ((JL_SYSTEM->CHIP_VER >= 0xA2) && (JL_SYSTEM->CHIP_VER < 0xAC)) { //C版以后才做DAC TRIM
        DAC_0dB = dac_digital_gain_tab_version_c[TCFG_DAC_POWER_MODE];
    }
    //printf("dac_dvol_max_query:%d",DAC_0dB);
    return DAC_0dB;
}

//***********************
//*		Audio Clock     *
//***********************
int aud_clock_alloc(const char *name, u32 clk)
{
    y_printf("aud_clock_alloc:%s(%d)\n", name, clk);
    return clock_alloc(name, clk);
}

int aud_clock_free(char *name)
{
    y_printf("aud_clock_alloc:%s\n", name);
    return clock_free(name);

}
