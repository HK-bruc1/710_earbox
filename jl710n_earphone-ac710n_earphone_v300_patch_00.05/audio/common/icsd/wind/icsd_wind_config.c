#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_adt.data.bss")
#pragma data_seg(".icsd_adt.data")
#pragma const_seg(".icsd_adt.text.const")
#pragma code_seg(".icsd_adt.text")
#endif

#include "app_config.h"
#if ((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN && \
	 TCFG_AUDIO_ANC_ENABLE)
#include "icsd_wind.h"
#include "icsd_adt.h"



//====================风噪检测配置=====================
const u8 ICSD_WIND_PHONE_TYPE = SDK_WIND_PHONE_TYPE;
const u8 ICSD_WIND_MIC_TYPE   = SDK_WIND_MIC_TYPE;
const u8 ICSD_WIND_ALG_BT_INF = 0;

struct wind_function *WIND_FUNC;

//==============================================//
//    风噪检测参数配置
//==============================================//
const u8  ICSD_WIND_3MIC = 0; //当前不支持3mic
const u16 correrr_thr = 300;
const int wind_sat_thr = 10000;
const u8 wind_lowfreq_point = 10;
const u8 wind_pwr_cnt_thr = 0;
const float wind_margin_dB = -20;
const float wind_pwr_ref_thr = 15;
const float wind_pwr_err_thr = 20;
const float wind_iir_alpha = 16;
const float wind_lpf_alpha = 0.0625;
const float wind_hpf_alpha = 0.001;
const float wind_max_min_diff_thr = 32;
const float wind_timepwr_diff_thr0 = 50;
const float wind_timepwr_diff_thr1 = 400;
const float wind_ref2err_diff_thr = 50;//50;
const float wind_ref2err_diffmin_thr = 15;
const u8 wind_num_thr = 4;
const float wind_tlk_corr = 0.35;//0.3 ~ 1
const u8 wind_tlk_corr_num = 6;//7
const float wind_tlk_corr_avr = 0.35;//0.3;
const u8 wind_corr_select = 0;
const u8 wind_fcorr_fpoint = 10;
const float wind_fcorr_min_thr = 0.6;
const int wind_ref2err_cnt = 13;
const u8 wind_stable_cnt_thr = 1;
const u8 wind_lvl_scale = 4;
const u8 icsd_wind_num_thr2 = 4;
const u8 icsd_wind_num_thr1 = 2;
const u8 wind_out_mode = 1;//1:滤波输出(滤波模式) 0:连续3包无风输出0(快速模式)
const float wind_ref_cali_table[25] = {0};
const float wind_pwr_bw_thr0 = -4;
const float wind_pwr_bw_thr1 = -40;
const float wind_tppwr_bw_thr0 = -4;
const float wind_tppwr_bw_thr1 = 1;
const u8 wind_mic_sel = 0;
const float wind_errpwr_100hz_thr = 0;
const float wind_refpwr_100hz_thr = 20;
const u8 wind_cxy_cnt_thr1 = 18;
const u8 wind_cxy_cnt_thr2 = 1;
const float pwr_div_thr = 1.6;
const u8 cepst_en = 1;
const u8 cepst_debug = 0;
const float icsd_cepst_thr = -15;
const float cepst_1p_thr = 8;


int win_printf_off(const char *format, ...)
{
    return 0;
}

int (*win_printf)(const char *format, ...) = _win_printf;


void wind_config_init(__wind_config *_wind_config)
{
    __wind_config *wind_config = _wind_config;
    wind_config->msc_lp_thr    = 2.4;
    wind_config->msc_mp_thr    = 2.4;
    wind_config->corr_thr      = 0.4;//tws 不需要
    wind_config->cepst_1p_thr  = 20;
    wind_config->ref_pwr_thr   = 30;
    wind_config->wind_iir_alpha = 16;
    wind_config->wind_lvl_scale = 2;
    wind_config->icsd_wind_num_thr2 = 3;
    wind_config->icsd_wind_num_thr1 = 1;
}

void wind_function_init()
{
    WIND_FUNC->wind_config_init = wind_config_init;
    WIND_FUNC->HanningWin_pwr_float = icsd_HanningWin_pwr_float;
    WIND_FUNC->HanningWin_pwr_s1 = icsd_HanningWin_pwr_s1;
    WIND_FUNC->FFT_radix256 = icsd_FFT_radix256;
    WIND_FUNC->FFT_radix128 = icsd_FFT_radix128;
    WIND_FUNC->complex_mul  = icsd_complex_mul_v2;
    WIND_FUNC->complex_div  = icsd_complex_div_v2;
    WIND_FUNC->pxydivpxxpyy = icsd_pxydivpxxpyy;
    WIND_FUNC->log10_float  = icsd_log10_anc;
    WIND_FUNC->cal_score    = icsd_cal_score;
    WIND_FUNC->icsd_adt_tws_msync = icsd_adt_tws_msync;
    WIND_FUNC->icsd_adt_tws_ssync = icsd_adt_tws_ssync;
    WIND_FUNC->icsd_wind_run_part2_cmd = icsd_wind_run_part2_cmd;
    WIND_FUNC->icsd_adt_wind_part1_rx = icsd_adt_wind_part1_rx;
}

void icsd_wind_demo()
{
    static int *alloc_addr = NULL;
    struct icsd_win_libfmt libfmt;
    struct icsd_win_infmt  fmt;
    icsd_wind_get_libfmt(&libfmt);
    win_printf("WIND RAM SIZE:%d\n", libfmt.lib_alloc_size);
    if (alloc_addr == NULL) {
        alloc_addr = zalloc(libfmt.lib_alloc_size);
    }
    fmt.alloc_ptr = alloc_addr;
    icsd_wind_set_infmt(&fmt);
}

#endif/*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
