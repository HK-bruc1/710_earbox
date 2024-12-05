#ifndef _ICSD_ADT_ALG_H
#define _ICSD_ADT_ALG_H


//==========EIN========================
typedef struct {
    s16 *adc_ref_buf;
    s16 *adc_err_buf;
    s16 *inear_sz_spko_buf;
    s16 *inear_sz_emic_buf;
    s16 *inear_sz_dac_buf;
    u8 resume_mode;
    u8 anc_onoff;
    u8 run_mode;
} __adt_ein_run_parm;

typedef struct {
    u8 ein_output;
} __adt_ein_output;

int  icsd_adt_ein_get_libfmt();
int  icsd_adt_ein_set_infmt(int _ram_addr);
void icsd_adt_alg_ein_run(__adt_ein_run_parm *_run_parm, __adt_ein_output *_output);
//==========WAT========================
typedef struct {
    s16 *data_1_ptr;
    u8 resume_mode;
    u8 anc_onoff;
    float err_gain;
} __adt_wat_run_parm;

typedef struct {
    u8 wat_output;
    u8 get_max_range;
    int max_range;
} __adt_wat_output;

int  icsd_adt_wat_get_libfmt();
int  icsd_adt_wat_set_infmt(int _ram_addr);
void icsd_adt_alg_wat_run(__adt_wat_run_parm *_run_parm, __adt_wat_output *_output);
void icsd_adt_alg_wat_ram_clean();
//==========WIND========================
#if ICSD_WIND_LIB
#else//没有风噪库时使用该配置
#define ICSD_WIND_HEADSET           1
#define ICSD_WIND_TWS		        2
#define ICSD_WIND_LFF_TALK          1
#define ICSD_WIND_LFF_RFF           2
#define ICSD_WIND_LFF_LFB           3
#define ICSD_WIND_LFF_LFB_TALK      4
#define ICSD_WIND_RFF_TALK      	5
#endif

typedef struct {
    u8  f2wind;
    float f2pwr;
    float f2curin[48];
    int idx;
    u8  wind_id;
} __win_part1_out;

typedef struct {
    u8  f2wind;
    float f2pwr;
    float f2in_cur3t[48];
    int idx;
    u8  wind_id;
} __win_part1_out_rx;

typedef struct {
    s16 *data_1_ptr;
    s16 *data_2_ptr;
    s16 *data_3_ptr;
    u8 anc_mode;
    u8 wind_ft;
    __win_part1_out *part1_out;
    __win_part1_out_rx *part1_out_rx;
} __adt_win_run_parm;

typedef struct {
    u8 wind_lvl;
    void *wind_alg_bt_inf;
    u16 wind_alg_bt_len;
} __adt_win_output;

int  icsd_adt_wind_get_libfmt();
int  icsd_adt_wind_set_infmt(int _ram_addr);
void icsd_adt_alg_wind_run(__adt_win_run_parm *_run_parm, __adt_win_output *_output);
void icsd_adt_alg_wind_run_part1(__adt_win_run_parm *_run_parm, __adt_win_output *_output);
void icsd_adt_alg_wind_run_part2(__adt_win_run_parm *_run_parm, __adt_win_output *_output);
int  icsd_adt_alg_wind_ssync(__win_part1_out *_part1_out_tx);
void icsd_adt_wind_master_tx_data_sucess(void *_data);
void icsd_adt_wind_master_rx_data(void *_data);
void icsd_adt_wind_slave_tx_data_sucess(void *_data);
void icsd_adt_wind_slave_rx_data(void *_data);
u8   icsd_adt_wind_data_sync_en();
void icsd_wind_run_part2_cmd();
void icsd_wind_data_sync_master_cmd();
void *icsd_adt_wind_part1_rx();
//==========VDT========================
typedef struct {
    s16 *refmic;
    s16 *errmic;
    s16 *tlkmic;
    s16 *dmah;
    s16 *dmal;
    s16 *dac_data;
    float fbgain;
    float tfbgain;
    u8 mic_num;
    u8 anc_mode_ind;
} __adt_vdt_run_parm;

typedef struct {
    u8 voice_state;
} __adt_vdt_output;

int  icsd_adt_vdt_get_libfmt();
int  icsd_adt_vdt_set_infmt(int _ram_addr);
void icsd_adt_alg_vdt_run(__adt_vdt_run_parm *_run_parm, __adt_vdt_output *_output);
void icsd_adt_vdt_data_init(u8 _anc_mode_ind, float ref_mgain, float err_mgain, float tlk_mgain);
#endif
