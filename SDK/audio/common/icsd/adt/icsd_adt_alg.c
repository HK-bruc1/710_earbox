#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_adt.data.bss")
#pragma data_seg(".icsd_adt.data")
#pragma const_seg(".icsd_adt.text.const")
#pragma code_seg(".icsd_adt.text")
#endif

#include "app_config.h"
#if ((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN && \
	 TCFG_AUDIO_ANC_ENABLE)
#include "icsd_adt.h"

#if ICSD_VDT_LIB
#include "icsd_vdt.h"
#else
const u16 ADT_DEBUG_INF = 0;
u8    ADT_PATH_CONFIG = 0;
#endif

#if ICSD_WAT_LIB
#include "icsd_wat.h"
#endif

#if ICSD_EIN_LIB
#include "icsd_ein.h"
#endif

//===========EIN============================================
int icsd_adt_ein_get_libfmt()
{
    int add_size = 0;
#if ICSD_EIN_LIB
    struct icsd_ein_libfmt libfmt;
    icsd_ein_get_libfmt(&libfmt);
    add_size = libfmt.lib_alloc_size;
#else
    printf("ADT need to include EIN LIB ! ! !\n");
    while (1);
#endif
    return add_size;
}

int icsd_adt_ein_set_infmt(int _ram_addr)
{
    int set_size = 0;
#if ICSD_EIN_LIB
    struct icsd_ein_infmt  fmt;
    fmt.alloc_ptr = (void *)_ram_addr;
    icsd_ein_set_infmt(&fmt);
    set_size = fmt.lib_alloc_size;
#endif
    return set_size;
}

void icsd_adt_alg_ein_run(__adt_ein_run_parm *_run_parm, __adt_ein_output *_output)
{
#if ICSD_EIN_LIB
    __icsd_ein_run_parm run_parm;
    __icsd_ein_output output;
    run_parm.adc_ref_buf = _run_parm->adc_ref_buf;
    run_parm.adc_err_buf = _run_parm->adc_err_buf;
    run_parm.inear_sz_spko_buf = _run_parm->inear_sz_spko_buf;
    run_parm.inear_sz_emic_buf = _run_parm->inear_sz_emic_buf;
    run_parm.inear_sz_dac_buf = _run_parm->inear_sz_dac_buf;
    run_parm.resume_mode = _run_parm->resume_mode;
    run_parm.anc_onoff = _run_parm->anc_onoff;
    run_parm.run_mode = _run_parm->run_mode;

    extern void icsd_alg_ein_run(__icsd_ein_run_parm * run_parm, __icsd_ein_output * output);
    icsd_alg_ein_run(&run_parm, &output);
    _output->ein_output = output.ein_output;
#endif
}
//===========WAT============================================
int icsd_adt_wat_get_libfmt()
{
    int add_size = 0;
#if ICSD_WAT_LIB
    struct icsd_wat_libfmt libfmt;
    icsd_wat_get_libfmt(&libfmt);
    add_size = libfmt.lib_alloc_size;
#else
    printf("ADT need to include WAT LIB ! ! !\n");
    while (1);
#endif
    return add_size;
}

int icsd_adt_wat_set_infmt(int _ram_addr)
{
    int set_size = 0;
#if ICSD_WAT_LIB
    struct icsd_wat_infmt  fmt;
    fmt.alloc_ptr = (void *)_ram_addr;
    icsd_wat_set_infmt(&fmt);
    set_size = fmt.lib_alloc_size;
#endif
    return set_size;
}

void icsd_adt_alg_wat_run(__adt_wat_run_parm *_run_parm, __adt_wat_output *_output)
{
#if ICSD_WAT_LIB
    __icsd_wat_run_parm run_parm;
    __icsd_wat_output output;
    run_parm.data_1_ptr = _run_parm->data_1_ptr;
    run_parm.err_gain = _run_parm->err_gain;
    run_parm.resume_mode = _run_parm->resume_mode;
    run_parm.anc_onoff = _run_parm->anc_onoff;
    icsd_alg_wat_run(&run_parm, &output);
    _output->wat_output = output.wat_output;
    _output->get_max_range = output.get_max_range;
    _output->max_range = output.max_range;
#endif
}

void icsd_adt_alg_wat_ram_clean()
{
#if ICSD_WAT_LIB
    icsd_wat_ram_clean();
#endif
}


//===========WIND===========================================
#if ICSD_WIND_LIB
const u8 ICSD_ADT_WIND_MIC_TYPE   = SDK_WIND_MIC_TYPE;
const u8 ICSD_ADT_WIND_PHONE_TYPE = SDK_WIND_PHONE_TYPE;
#else
const u8 ICSD_ADT_WIND_MIC_TYPE   = 0;
const u8 ICSD_ADT_WIND_PHONE_TYPE = 0;
#endif

int icsd_adt_wind_get_libfmt()
{
    int add_size = 0;
#if ICSD_WIND_LIB
    struct icsd_win_libfmt libfmt;
    icsd_wind_get_libfmt(&libfmt);
    add_size = libfmt.lib_alloc_size;
#else
    printf("ADT need to include WIND LIB ! ! !\n");
    while (1);
#endif
    return add_size;
}

int icsd_adt_wind_set_infmt(int _ram_addr)
{
    int set_size = 0;
#if ICSD_WIND_LIB
    struct icsd_win_infmt  fmt;
    fmt.alloc_ptr = (void *)_ram_addr;
    icsd_wind_set_infmt(&fmt);
    set_size = fmt.lib_alloc_size;
#endif
    return set_size;
}

void icsd_adt_alg_wind_run(__adt_win_run_parm *_run_parm, __adt_win_output *_output)
{
#if ICSD_WIND_LIB
    __icsd_win_run_parm run_parm;
    __icsd_win_output output;
    run_parm.data_1_ptr = _run_parm->data_1_ptr;
    run_parm.data_2_ptr = _run_parm->data_2_ptr;
    icsd_alg_wind_run(&run_parm, &output);
    _output->wind_lvl = output.wind_lvl;
    _output->wind_alg_bt_inf = output.wind_alg_bt_inf;
    _output->wind_alg_bt_len = output.wind_alg_bt_len;
#endif
}

void icsd_adt_alg_wind_run_part1(__adt_win_run_parm *_run_parm, __adt_win_output *_output)
{
#if ICSD_WIND_LIB
    __icsd_win_run_parm run_parm;
    __icsd_win_output output;
    run_parm.data_1_ptr = _run_parm->data_1_ptr;
    run_parm.part1_out = (__wind_part1_out *)_run_parm->part1_out;
    run_parm.part1_out->idx = _run_parm->part1_out->idx;
    run_parm.part1_out->wind_id = _run_parm->part1_out->wind_id;
    icsd_alg_wind_run_part1(&run_parm, &output);
#endif
}

void icsd_adt_alg_wind_run_part2(__adt_win_run_parm *_run_parm, __adt_win_output *_output)
{
#if ICSD_WIND_LIB
    __icsd_win_run_parm run_parm;
    __icsd_win_output output;
    run_parm.part1_out = (__wind_part1_out *)_run_parm->part1_out;
    run_parm.part1_out_rx = (__wind_part1_out_rx *)_run_parm->part1_out_rx;
    icsd_alg_wind_run_part2(&run_parm, &output);
    _output->wind_lvl = output.wind_lvl;
    _output->wind_alg_bt_inf = output.wind_alg_bt_inf;
    _output->wind_alg_bt_len = output.wind_alg_bt_len;
#endif
}

int icsd_adt_alg_wind_ssync(__win_part1_out *_part1_out_tx)
{
    int ret = 1;
#if ICSD_WIND_LIB
    ret = alg_wind_ssync((__wind_part1_out *)_part1_out_tx);
#endif
    return ret;
}

void icsd_adt_wind_master_tx_data_sucess(void *_data)
{
#if ICSD_WIND_LIB
    icsd_wind_master_tx_data_sucess(_data);
#endif
}

void icsd_adt_wind_master_rx_data(void *_data)
{
#if ICSD_WIND_LIB
    icsd_wind_master_rx_data(_data);
#endif
}

void icsd_adt_wind_slave_tx_data_sucess(void *_data)
{
#if ICSD_WIND_LIB
    icsd_wind_slave_tx_data_sucess(_data);
#endif
}

void icsd_adt_wind_slave_rx_data(void *_data)
{
#if ICSD_WIND_LIB
    icsd_wind_slave_rx_data(_data);
#endif
}

u8 icsd_adt_wind_data_sync_en()
{
    u8 en = 0;
#if ICSD_WIND_LIB
    if ((ICSD_WIND_PHONE_TYPE == ICSD_WIND_TWS) && (ICSD_WIND_MIC_TYPE == ICSD_WIND_LFF_RFF)) {
        en = 1;
    }
#endif
    return en;
}



//===========VDT============================================
#if ICSD_VDT_LIB
const u8 ADT_VDT_USE_ANCDMAL = VDT_USE_ANCDMAL_DATA;
#else
const u8 ADT_VDT_USE_ANCDMAL = 0;
#endif
int icsd_adt_vdt_get_libfmt()
{
    int add_size = 0;
#if ICSD_VDT_LIB
    struct icsd_vdt_libfmt libfmt;
    icsd_vdt_get_libfmt(&libfmt);
    add_size = libfmt.lib_alloc_size;
#else
    printf("ADT need to include VDT LIB ! ! !\n");
    while (1);
#endif
    return add_size;
}

int icsd_adt_vdt_set_infmt(int _ram_addr)
{
    int set_size = 0;
#if ICSD_VDT_LIB
    struct icsd_vdt_infmt  fmt;
    fmt.alloc_ptr = (void *)_ram_addr;
    icsd_vdt_set_infmt(&fmt);
    set_size = fmt.lib_alloc_size;
#endif
    return set_size;
}

void icsd_adt_alg_vdt_run(__adt_vdt_run_parm *_run_parm, __adt_vdt_output *_output)
{
#if ICSD_VDT_LIB
    __icsd_vdt_run_parm run_parm;
    __icsd_vdt_output output;
    run_parm.refmic   = _run_parm->refmic;
    run_parm.errmic   = _run_parm->errmic;
    run_parm.tlkmic   = _run_parm->tlkmic;
    run_parm.dmah     = _run_parm->dmah;
    run_parm.dmal     = _run_parm->dmal;
    run_parm.dac_data = _run_parm->dac_data;
    run_parm.fbgain   = _run_parm->fbgain;
    run_parm.tfbgain  = _run_parm->tfbgain;
    run_parm.mic_num  = _run_parm->mic_num;
    run_parm.anc_mode_ind = _run_parm->anc_mode_ind;
    icsd_alg_vdt_run(&run_parm, &output);
    _output->voice_state = output.voice_state;
#endif
}

void icsd_adt_vdt_data_init(u8 _anc_mode_ind, float ref_mgain, float err_mgain, float tlk_mgain)
{
#if ICSD_VDT_LIB
    icsd_vdt_data_init(_anc_mode_ind, ref_mgain, err_mgain, tlk_mgain);
#endif
}

#endif/*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
