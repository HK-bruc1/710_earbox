/*
 ******************************************************************
 *					      Audio Setup
 *
 * Discription: 音频模块初始化，配置，调试等
 *
 * Notes:
 ******************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "sdk_config.h"
#include "audio_adc.h"
#include "media/audio_energy_detect.h"
#include "adc_file.h"
#include "linein_file.h"
#include "asm/audio_common.h"
#include "gpio_config.h"
#include "audio_demo/audio_demo.h"
#include "gpadc.h"
#include "update.h"
#include "media/audio_general.h"
#include "media/audio_event_manager.h"
#include "asm/hw_eq.h"
#include "media/media_config.h"
#include "media_bank.h"

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#include "audio_dvol.h"
#endif

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice.h"
#endif /*TCFG_SMART_VOICE_ENABLE*/

struct audio_dac_hdl dac_hdl;
struct audio_adc_hdl adc_hdl;

typedef struct {
    u8 audio_inited;
    atomic_t ref;
} audio_setup_t;
audio_setup_t audio_setup = {0};
#define __this      (&audio_setup)

#if TCFG_MC_BIAS_AUTO_ADJUST
u8 mic_bias_rsel_use_save[AUDIO_ADC_MIC_MAX_NUM] = {0};
u8 save_mic_bias_rsel[AUDIO_ADC_MIC_MAX_NUM]     = {0};
u8 mic_ldo_vsel_use_save = 0;
u8 save_mic_ldo_vsel     = 0;
#endif // #if TCFG_MC_BIAS_AUTO_ADJUST

struct dac_platform_data dac_data;

int get_dac_channel_num(void)
{
    return dac_hdl.channel;
}

static u8 audio_dac_hpvdd_check()
{
#if 0
    u8 res;
    u16 hpvdd = adc_get_voltage_blocking(AD_CH_AUDIO_HPVDD);
    /* printf("HPVDD: %d\n", hpvdd); */
    if (hpvdd > 1500) {
        res = DAC_HPVDD_18V;
        puts("DAC_HPVDD: 1.8V");
    } else {
        res = DAC_HPVDD_12V;
        puts("DAC_HPVDD: 1.2V");
    }
    SFR(JL_ADDA->ADDA_CON0,  0,  1,  0);					// AUDIO到SARADC的总测试通道使能
    SFR(JL_ADDA->ADDA_CON0,  3,  1,  0);					// DAC测试通道总使能
    SFR(JL_ADDA->ADDA_CON0,  4,  3,  0);					// DAC待测试信号选择位
    return res;
#endif
    return 0;
}

/*
 * DAC开关状态回调，需要的时候，"#if 0" 改成 "#if 1"
 */
#if 0
void audio_dac_power_state(u8 state)
{
    switch (state) {
    case DAC_ANALOG_OPEN_PREPARE:
        //DAC打开前，即准备打开
        printf("DAC power_on start\n");
        break;
    case DAC_ANALOG_OPEN_FINISH:
        //DAC打开后，即打开完成
        printf("DAC power_on end\n");
        break;
    case DAC_ANALOG_CLOSE_PREPARE:
        //DAC关闭前，即准备关闭
        printf("DAC power_off start\n");
        break;
    case DAC_ANALOG_CLOSE_FINISH:
        //DAC关闭后，即关闭完成
        printf("DAC power_off end\n");
        break;
    }
}
#endif

__AUDIO_INIT_BANK_CODE
static void audio_common_initcall()
{
    printf("audio_common_initcall\n");
    audio_common_param_t common_param = {0};
    common_param.ff_clk_div = 0;
    common_param.fb_clk_div = 3;
    common_param.sz_clk_div = 0;
    common_param.cic.en = 1;//TCFG_AUDIO_ANC_ENABLE;
    common_param.cic.scale = 0;
    common_param.cic.shift = 11;
    common_param.drc.bypass = 1;
    common_param.drc.threshold = 1023;
    common_param.drc.ratio = 64;
    common_param.drc.kneewidth = 16;
    common_param.drc.makeup_gain = 0;
    common_param.drc.attack_time = 3;
    common_param.drc.release_time = 10;

    common_param.vcm0d5_mode = 1;
    common_param.pmu_vbg_value = 4;
    common_param.audio_vbg_value = 4;

    /* common_param.clock_mode = AUDIO_COMMON_CLK_DIG_SINGLE; */
    common_param.clock_mode = AUDIO_COMMON_CLK_DIF_XOSC;
    /* common_param.clock_mode = AUDIO_COMMON_CLK_DIG_XOSC; //低功耗配置时钟 */
    common_param.vcm_cap_en = TCFG_AUDIO_VCM_CAP_EN;
    audio_common_init(&common_param);

    //音频时钟初始化
    audio_dac_clock_init();

#if ((defined TCFG_AUDIO_DAC_CLASSH_EN) && (TCFG_AUDIO_DAC_CLASSH_EN == 1))
    audio_common_classh_clock_open(2);
#endif
    // common power trim
    audio_common_param_t *common = audio_common_get_param();
    int len;
    audio_vbg_trim_t vbg_trim = {0};
    len = audio_event_notify(AUDIO_LIB_EVENT_VBG_TRIM_READ, (void *)&vbg_trim, sizeof(audio_vbg_trim_t));
    if ((len != sizeof(audio_vbg_trim_t)) || (vbg_trim.src_sel != common->vcm_cap_en)) {
        printf("VBG trim, vcm_cap_en: %d, vcm_cap_en(VM): %d\n", common->vcm_cap_en, vbg_trim.src_sel);
        vbg_trim.src_sel = common->vcm_cap_en;
        u8 ret = audio_common_power_trim(&vbg_trim, 0);
        if (ret == 0) {
            audio_event_notify(AUDIO_LIB_EVENT_VBG_TRIM_WRITE, (void *)&vbg_trim, sizeof(audio_vbg_trim_t));
        }
    }
    common->audio_vbg_value = vbg_trim.aud_vbg_value;
    common->pmu_vbg_value = vbg_trim.pmu_vbg_value;
    printf(">>VBG_TRIM: %d, %d\n", common->audio_vbg_value, common->pmu_vbg_value);
    const u8 power_mode[] = {20, 30, 50, 80, 100, 0xFF};
    printf("DAC power mode:%dmW", power_mode[dac_data.power_mode]);

    // dacldo trim
    audio_dacldo_trim_t dacldo_trim = {0};
    len = audio_event_notify(AUDIO_LIB_EVENT_DACLDO_TRIM_READ, (void *)&dacldo_trim, sizeof(audio_dacldo_trim_t));
    if ((len != sizeof(audio_dacldo_trim_t)) || (dacldo_trim.power_mode != dac_data.power_mode)) {
        printf("DACLDO trim, power mode:%dmW, power mode(VM):%dmW", power_mode[dac_data.power_mode], power_mode[dacldo_trim.power_mode]);
        dacldo_trim.power_mode = dac_data.power_mode;
        u8 ret = audio_dac_ldo_trim(&dacldo_trim.dacldo_vsel, dac_data.power_mode);
        if (ret == 0) {
            audio_event_notify(AUDIO_LIB_EVENT_DACLDO_TRIM_WRITE, (void *)&dacldo_trim, sizeof(audio_dacldo_trim_t));
        }
    }
    dac_data.dacldo_vsel = dacldo_trim.dacldo_vsel;
    printf(">>DACLDO_TRIM: %d\n", dac_data.dacldo_vsel);
}

__AUDIO_INIT_BANK_CODE
void audio_dac_initcall(void)
{
    printf("audio_dac_initcall\n");

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif
    dac_data.max_sample_rate    = AUDIO_DAC_MAX_SAMPLE_RATE;
    dac_data.hpvdd_sel = audio_dac_hpvdd_check();
    dac_data.bit_width = audio_general_out_dev_bit_width();
    dac_data.mute_delay_isel = 2;
    dac_data.mute_delay_time = 50;
    dac_data.miller_en = 1;
    if ((JL_SYSTEM->CHIP_VER >= 0xA2) && (JL_SYSTEM->CHIP_VER < 0xAC)) { //C版及C版以后支持工具配置电流档位
        dac_data.pa_isel0 = TCFG_AUDIO_DAC_PA_ISEL0;
        dac_data.pa_isel1 = TCFG_AUDIO_DAC_PA_ISEL1;
    } else {
        if (dac_data.performance_mode == DAC_MODE_HIGH_PERFORMANCE) {
            dac_data.pa_isel0 = 5;
            dac_data.pa_isel1 = 7;
        } else {
            dac_data.pa_isel0 = 3;
            dac_data.pa_isel1 = 7;
        }
    }
    audio_dac_init(&dac_hdl, &dac_data);
    //dac_hdl.ng.threshold = 4;			//DAC底噪优化阈值
    //dac_hdl.ng.detect_interval = 200;	//DAC底噪优化检测间隔ms

    //ANC & DAC_CIC时钟分配参数设置在audio_common_init & audio_dac_init之后
#if TCFG_AUDIO_ANC_ENABLE
    /*
       1、根据ANC参数生成CLOCK DIV以及ANC & DAC CIC配置
       2、读取ANC DAC DRC配置
    */
    audio_anc_common_param_init();
#endif/*TCFG_AUDIO_ANC_ENABLE*/

    audio_dac_set_analog_vol(&dac_hdl, 0);

#if AUD_DAC_TRIM_ENABLE
    if ((JL_SYSTEM->CHIP_VER >= 0xA2) && (JL_SYSTEM->CHIP_VER < 0xAC)) { //C版以后才做DAC TRIM
        struct audio_dac_trim dac_trim = {0};
        int len = syscfg_read(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
        if (len != sizeof(dac_trim)) {
            if (config_audio_dac_output_mode == DAC_MODE_SINGLE) {
                dac_trim.left = -1250;
                dac_trim.right = -1250;
            } else {
                struct trim_init_param_t trim_init = {0};
                trim_init.precision = 1; //DAC trim的收敛精度(-precision, +precision)
                trim_init.trim_speed = 0.7f; //DAC trim的收敛速度(不建议修改)
                int ret = audio_dac_do_trim(&dac_hdl, &dac_trim, &trim_init);
                int triml_offset = (config_audio_dac_output_mode == DAC_MODE_DIFF) ? (1250) : (2500);
                int trimr_offset = triml_offset;
                int trim_limit = (config_audio_dac_output_mode == DAC_MODE_DIFF) ? (100) : (300);
                if (config_audio_dac_output_channel == DAC_OUTPUT_MONO_L) {
                    trimr_offset = 0;
                }
                if (config_audio_dac_output_channel == DAC_OUTPUT_MONO_R) {
                    triml_offset = 0;
                }
                if ((ret == 0) && (__builtin_abs(dac_trim.left + triml_offset) < trim_limit) && (__builtin_abs(dac_trim.right + trimr_offset) < trim_limit)) {
                    /* puts("dac_trim_succ"); */
                    syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(struct audio_dac_trim));
                } else {
                    dac_trim.left = -1250;
                    dac_trim.right = -1250;
                }
                audio_dac_close(&dac_hdl);
            }
        }
        audio_dac_set_trim_value(&dac_hdl, &dac_trim);
    }
#endif

    audio_dac_set_fade_handler(&dac_hdl, NULL, audio_fade_in_fade_out);

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    /* audio_src_base_filt_init(audio_src_hw_filt, sizeof(audio_src_hw_filt)); */

#if AUDIO_OUTPUT_AUTOMUTE
    mix_out_automute_open();
#endif  //#if AUDIO_OUTPUT_AUTOMUTE
}

static u8 audio_init_complete()
{
    if (!__this->audio_inited) {
        return 0;
    }
    return 1;
}

REGISTER_LP_TARGET(audio_init_lp_target) = {
    .name = "audio_init",
    .is_idle = audio_init_complete,
};

extern void dac_analog_power_control(u8 en);
void audio_fast_mode_test()
{
    audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    /* dac_analog_power_control(1);////将关闭基带，不开可发现，不可连接 */
    audio_dac_start(&dac_hdl);
    audio_adc_mic_demo_open(AUDIO_ADC_MIC_CH, 10, 16000, 1);

}

struct audio_adc_private_param adc_private_param = {
    .performance_mode = TCFG_ADC_PERFORMANCE_MODE,
    .mic_ldo_vsel   = TCFG_AUDIO_MIC_LDO_VSEL,
    /* .mic_ldo_isel   = TCFG_AUDIO_MIC_LDO_ISEL, */
    .adca_reserved0 = 0,
    .adcb_reserved0 = 0,
    .lowpower_lvl = 0,
};

#if TCFG_AUDIO_ADC_ENABLE
struct adc_platform_cfg adc_platform_cfg_table[AUDIO_ADC_MAX_NUM] = {
#if TCFG_ADC0_ENABLE
    [0] = {
        .mic_mode           = TCFG_ADC0_MODE,
        .mic_ain_sel        = TCFG_ADC0_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC0_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC0_BIAS_RSEL,
        .power_io           = TCFG_ADC0_POWER_IO,
        .mic_dcc_en         = TCFG_ADC0_DCC_EN,
        .mic_dcc            = TCFG_ADC0_DCC_LEVEL,
    },
#endif
#if TCFG_ADC1_ENABLE
    [1] = {
        .mic_mode           = TCFG_ADC1_MODE,
        .mic_ain_sel        = TCFG_ADC1_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC1_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC1_BIAS_RSEL,
        .power_io           = TCFG_ADC1_POWER_IO,
        .mic_dcc_en         = TCFG_ADC1_DCC_EN,
        .mic_dcc            = TCFG_ADC1_DCC_LEVEL,
    },
#endif
};
#endif

__AUDIO_INIT_BANK_CODE
void audio_input_initcall(void)
{
    printf("audio_input_initcall\n");
#if TCFG_MC_BIAS_AUTO_ADJUST
    /* extern u8 mic_ldo_vsel_use_save; */
    /* extern u8 save_mic_ldo_vsel; */
    if (mic_ldo_vsel_use_save) {
        adc_private_param.mic_ldo_vsel = save_mic_ldo_vsel;
    }
#endif

    u16 dvol_441k = (u16)(50 * eq_db2mag(TCFG_ADC_DIGITAL_GAIN));
    u16 dvol_48k  = (u16)(35 * eq_db2mag(TCFG_ADC_DIGITAL_GAIN));
    adc_private_param.dvol_441k = (dvol_441k >= AUDIO_ADC_DVOL_LIMIT) ? AUDIO_ADC_DVOL_LIMIT : dvol_441k;
    adc_private_param.dvol_48k = (dvol_48k >= AUDIO_ADC_DVOL_LIMIT) ? AUDIO_ADC_DVOL_LIMIT : dvol_48k;
    audio_adc_init(&adc_hdl, &adc_private_param);
    /* adc_hdl.bit_width = audio_general_in_dev_bit_width(); */

#if TCFG_SUPPORT_MIC_CAPLESS
    adc_hdl.capless_trim.bias_rsel0 = TCFG_ADC0_BIAS_RSEL;
    adc_hdl.capless_trim.bias_rsel1 = TCFG_ADC1_BIAS_RSEL;
    adc_hdl.capless_param.trigger_threshold = 50; //电压差校准触发阈值(单位:mV)
    /*
     * 以下两个delay需要根据
     * const_mic_capless_open_delay_debug、const_mic_capless_trim_delay_debug 的结果配置
     */
    adc_hdl.capless_param.open_delay_ms = 150; //adc上电等待稳定延时
    adc_hdl.capless_param.trim_delay_ms = 50; //偏置调整等待稳定延时
    int ret = 0;
#if (TCFG_ADC0_ENABLE && (TCFG_ADC0_MODE == AUDIO_MIC_CAPLESS_MODE))
    adc_hdl.capless_param.mic_trim_ch |= AUDIO_ADC_MIC_0;
#endif
#if (TCFG_ADC1_ENABLE && (TCFG_ADC1_MODE == AUDIO_MIC_CAPLESS_MODE))
    adc_hdl.capless_param.mic_trim_ch |= AUDIO_ADC_MIC_1;
#endif
#if (TCFG_MC_BIAS_AUTO_ADJUST == MC_BIAS_ADJUST_ONE)
    int len = syscfg_read(CFG_MC_BIAS, &adc_hdl.capless_trim, sizeof(struct mic_capless_trim_result));
    if (len != sizeof(struct mic_capless_trim_result)) {
        ret = audio_mic_bias_adjust(&adc_hdl.capless_trim, &adc_hdl.capless_param);
        if (ret == 0) {
            syscfg_write(CFG_MC_BIAS, &adc_hdl.capless_trim, sizeof(struct mic_capless_trim_result));
        }
    }
#elif (TCFG_MC_BIAS_AUTO_ADJUST == MC_BIAS_ADJUST_ALWAYS)
    ret = audio_mic_bias_adjust(&adc_hdl.capless_trim, &adc_hdl.capless_param);
    if (ret == 0) {
        syscfg_write(CFG_MC_BIAS, &adc_hdl.capless_trim, sizeof(struct mic_capless_trim_result));
    }
#endif
    adc_platform_cfg_table[0].mic_bias_rsel = adc_hdl.capless_trim.bias_rsel0;
    adc_platform_cfg_table[1].mic_bias_rsel = adc_hdl.capless_trim.bias_rsel1;
    printf("BIAS_RSEL: %d, %d\n", adc_hdl.capless_trim.bias_rsel0, adc_hdl.capless_trim.bias_rsel1);
#endif
    audio_adc_file_init();

#if TCFG_AUDIO_DUT_ENABLE
    audio_dut_init();
#endif /*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_AUDIO_LINEIN_ENABLE
    audio_linein_file_init();
#endif/*TCFG_AUDIO_LINEIN_ENABLE*/
}

struct dac_platform_data dac_data = {//临时处理
    /* .power_on_mode  = TCFG_AUDIO_DAC_POWER_ON_MODE, */
    .dma_buf_time_ms = TCFG_AUDIO_DAC_BUFFER_TIME_MS,
    .performance_mode = TCFG_DAC_PERFORMANCE_MODE,
    .power_mode     = TCFG_DAC_POWER_MODE,
    .l_ana_gain     = TCFG_AUDIO_L_CHANNEL_GAIN,
    .r_ana_gain     = TCFG_AUDIO_R_CHANNEL_GAIN,
    .dcc_level      = 14,
    .bit_width      = DAC_BIT_WIDTH_16,
    // TODO
    .dacldo_vsel    = 3,
    .classh_en      = TCFG_AUDIO_DAC_CLASSH_EN,
    .classh_mode    = 0,
    .classh_down_step = 3000000,
};

static void wl_audio_clk_on(void)
{
    //TODO：临时处理，初始化蓝牙时钟，防止sync寄存器访问异常
    SFR(JL_LSBCLK->PRP_CON2,  0,  2,  1);    //wl_aud_clk sel std48m
    SFR(JL_LSBCLK->PRP_CON2, 11,  1,  1);    //wl_aud_rst
    udelay(100);
    JL_WL_AUD->CON0 = 1;
}

__INITCALL_BANK_CODE
static int audio_init()
{
    struct volume_mixer vol_mixer = {
        .hw_dvol_max = dac_dvol_max_query,
    };
    audio_volume_mixer_init(&vol_mixer);
    audio_common_initcall();
#ifndef CONFIG_FPGA_ENABLE
    wl_audio_clk_on();
#endif
    //通用初始化
    audio_general_init();
    //输入初始化
    audio_input_initcall();
#if TCFG_AUDIO_ANC_ENABLE
    anc_init();
#endif
    //DAC输出初始化
    audio_dac_initcall();

#if TCFG_SMART_VOICE_ENABLE
    audio_smart_voice_detect_init(NULL);
#endif /* #if TCFG_SMART_VOICE_ENABLE */
    __this->audio_inited = 1;
    return 0;
}
platform_initcall(audio_init);

static void audio_uninit()
{
    dac_power_off();
}
platform_uninitcall(audio_uninit);

/*关闭audio相关模块使能*/
static void audio_disable_all(void)
{
    printf("audio_disable_all\n");
    //DAC:DACEN
    JL_AUD->AUD_CON0 &= ~BIT(2);
    //ADC:ADCEN
    JL_AUD->AUD_CON0 &= ~(BIT(1) | BIT(4) | BIT(9));
    //EQ:
    JL_EQ->CON0 &= ~BIT(1);
    //FFT:
    //JL_FFT->CON = BIT(1);//置1强制关闭模块，不管是否已经运算完成
    //SRC:
    JL_SRC0->CON1 |= BIT(22);
    /* JL_SRC1->CON0 |= BIT(10); */

    //ANC:anc_en anc_start
#if 0//build
    JL_ANC->CON0 &= ~(BIT(1) | BIT(29));
#endif

}
void dac_power_on(void)
{
    /* log_info(">>>dac_power_on:%d", __this->ref.counter); */
    if (atomic_inc_return(&__this->ref) == 1) {
        audio_dac_open(&dac_hdl);
    }
}

void dac_power_off(void)
{
    /*log_info(">>>dac_power_off:%d", __this->ref.counter);*/
    if (atomic_read(&__this->ref) != 0 && atomic_dec_return(&__this->ref)) {
        return;
    }
    audio_dac_close(&dac_hdl);
}

#define TRIM_VALUE_LR_ERR_MAX           (600)   // 距离参考值的差值限制
#define abs(x) ((x)>0?(x):-(x))
int audio_dac_trim_value_check(struct audio_dac_trim *dac_trim)
{
#if 0
    s16 reference = 0;
    printf("audio_dac_trim_value_check %d %d\n", dac_trim->left, dac_trim->right);

#if TCFG_AUDIO_DAC_CONNECT_MODE != DAC_OUTPUT_MONO_R
    if (abs(dac_trim->left - reference) > TRIM_VALUE_LR_ERR_MAX) {
        printf("dac trim channel l err\n");
        return -1;
    }
#endif
#if TCFG_AUDIO_DAC_CONNECT_MODE != DAC_OUTPUT_MONO_L
    if (abs(dac_trim->right - reference) > TRIM_VALUE_LR_ERR_MAX) {
        printf("dac trim channel r err\n");
        return -1;
    }
#endif
#endif

    return 0;
}

REGISTER_UPDATE_TARGET(audio_update_target) = {
    .name = "audio",
    .driver_close = audio_disable_all,
};

/*音频模块寄存器跟踪*/
void audio_adda_dump(void) //打印所有的dac,adc寄存器
{
    printf("JL_WL_AUD CON0:%x", JL_WL_AUD->CON0);
    printf("DAC_CON0:%x", JL_AUDDAC->DAC_CON0);
    printf("DAC_CON1:%x", JL_AUDDAC->DAC_CON1);
    printf("DAC_CON2:%x", JL_AUDDAC->DAC_CON2);
    printf("DAC_CON3:%x", JL_AUDDAC->DAC_CON3);
    printf("DAC_CON4:%x", JL_AUDDAC->DAC_CON4);
    printf("DAC_CON5:%x", JL_AUDDAC->DAC_CON5);
    printf("DAC_CON6:%x", JL_AUDDAC->DAC_CON6);
    printf("DAC_CON7:%x", JL_AUDDAC->DAC_CON7);
    printf("DAC_CON8:%x", JL_AUDDAC->DAC_CON8);
    printf("DAC_CON9:%x", JL_AUDDAC->DAC_CON9);
    printf("DAC_CON10:%x", JL_AUDDAC->DAC_CON10);
    printf("DAC_CON11:%x", JL_AUDDAC->DAC_CON11);
    printf("DAC_CON12:%x", JL_AUDDAC->DAC_CON12);
    printf("DAC_CON13:%x", JL_AUDDAC->DAC_CON13);
    printf("DAC_CON14:%x", JL_AUDDAC->DAC_CON14);
    printf("DAC_CON15:%x", JL_AUDDAC->DAC_CON15);
    printf("ADC_CON0:%x", JL_AUDADC->ADC_CON0);
    printf("ADC_CON1:%x", JL_AUDADC->ADC_CON1);
    printf("ADC_CON2:%x", JL_AUDADC->ADC_CON2);
    printf("DAC_VL0:%x", JL_AUDDAC->DAC_VL0);
    printf("DAC_TM0:%x", JL_AUDDAC->DAC_TM0);
    printf("ADDA_CON0:%x", JL_ADDA->ADDA_CON0);
    printf("ADDA_CON1:%x", JL_ADDA->ADDA_CON1);
    printf("ADDA_CON0:%x    1:%x\n", JL_ADDA->ADDA_CON0, JL_ADDA->ADDA_CON1);
    printf("DAA_CON 0:%x 	1:%x,	2:%x,	7:%x\n", JL_ADDA->DAA_CON0, JL_ADDA->DAA_CON1, JL_ADDA->DAA_CON2, JL_ADDA->DAA_CON7);
    printf("ADA_CON 0:%x	1:%x	2:%x	3:%x	4:%x	5:%x	6:%x\n", JL_ADDA->ADA_CON0, JL_ADDA->ADA_CON1, JL_ADDA->ADA_CON2, JL_ADDA->ADA_CON3, JL_ADDA->ADA_CON4, JL_ADDA->ADA_CON5, JL_ADDA->ADA_CON6);
    printf("AUD_CON 0:%x	1:%x	2:%x	3:%x	4:%x	5:%x	6:%x\n", JL_AUD->AUD_CON0, JL_AUD->AUD_CON1, JL_AUD->AUD_CON2, JL_AUD->AUD_CON3, JL_AUD->AUD_CON4, JL_AUD->AUD_CON5, JL_AUD->AUD_CON6);
}

/*音频模块配置跟踪*/
void audio_config_dump()
{
    u8 dac_bit_width = ((JL_AUDDAC->DAC_CON0 & BIT(20)) ? 24 : 16);
    u8 adc_bit_width = ((JL_AUDADC->ADC_CON0 & BIT(20)) ? 24 : 16);
    int dac_dgain_max = 16384;
    int dac_again_max = 7;
    int mic_gain_max = 5;
    u8 dac_dcc = (JL_AUDDAC->DAC_CON0 >> 12) & 0xF;
    u8 mic0_dcc = (JL_AUDADC->ADC_CON1 >> 12) & 0xF;
    u8 mic1_dcc = (JL_AUDADC->ADC_CON1 >> 16) & 0xF;

    u8 dac_again_l = (JL_ADDA->DAA_CON1 >> 1) & 0x7;
    u8 dac_again_r = (JL_ADDA->DAA_CON2 >> 1) & 0x7;
    u32 dac_dgain_l = JL_AUDDAC->DAC_VL0 & 0xFFFF;
    u32 dac_dgain_r = (JL_AUDDAC->DAC_VL0 >> 16) & 0xFFFF;
    u8 mic0_0_6 = (JL_ADDA->ADA_CON2 >> 14) & 0x1;
    u8 mic1_0_6 = (JL_ADDA->ADA_CON3 >> 14) & 0x1;
    u8 mic0_0_3 = (JL_ADDA->ADA_CON2 >> 13) & 0x1;
    u8 mic1_0_3 = (JL_ADDA->ADA_CON3 >> 13) & 0x1;
    u8 mic0_gain = (JL_ADDA->ADA_CON2 >> 10) & 0x7;
    u8 mic1_gain = (JL_ADDA->ADA_CON3 >> 10) & 0x7;
    int dac_sr = audio_dac_get_sample_rate_base_reg();
    int adc_sr = audio_adc_mic_get_sample_rate();

    printf("[ADC]BitWidth:%d,DCC:%d,%d,SR:%d\n", adc_bit_width, mic0_dcc, mic1_dcc, adc_sr);
    printf("[ADC]Gain(Max:%d):%d,%d,N3dB:%d,%d,N6dB:%d,%d\n", mic_gain_max, mic0_gain, mic1_gain, \
           mic0_0_3, mic1_0_3, mic0_0_6, mic1_0_6);

    printf("[DAC]BitWidth:%d,DCC:%d,SR:%d\n", dac_bit_width, dac_dcc, dac_sr);
    printf("[DAC]AGain(Max:%d):%d,%d,DGain(Max:%d):%d,%d\n", dac_again_max, dac_again_l, dac_again_r, \
           dac_dgain_max, dac_dgain_l, dac_dgain_r);
}

