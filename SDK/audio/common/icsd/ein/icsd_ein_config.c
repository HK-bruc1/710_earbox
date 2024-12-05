#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_adt.data.bss")
#pragma data_seg(".icsd_adt.data")
#pragma const_seg(".icsd_adt.text.const")
#pragma code_seg(".icsd_adt.text")
#endif

#include "app_config.h"
#if ((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN && \
	 TCFG_AUDIO_ANC_ENABLE)

#include "icsd_ein.h"
#include "icsd_adt.h"

struct ein_function *EIN_FUNC;
int (*ein_printf)(const char *format, ...) = _ein_printf;

void ein_config_init(__ein_config *_ein_config)
{
    ein_printf("ein_config_init\n");
}


void ein_function_init()
{
    ein_printf("ein_function_init");
    EIN_FUNC->ein_config_init = ein_config_init;
    EIN_FUNC->HanningWin_pwr_s1 = icsd_HanningWin_pwr_s1;
    EIN_FUNC->FFT_radix64 = icsd_FFT_radix64;
    EIN_FUNC->FFT_radix256 = icsd_FFT_radix256;
    EIN_FUNC->complex_mul  = icsd_complex_mul_v2;
    EIN_FUNC->complex_div  = icsd_complex_div_v2;
    EIN_FUNC->log10_float  = icsd_log10_anc;
}

#endif/*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
