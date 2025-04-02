
#include "sbox_user_app.h"
#include "classic/tws_api.h"
#include "audio_anc.h"
#include "btstack/bluetooth.h"
#include "audio_config.h"
#include "btstack/avctp_user.h"
#include "event.h"
#include "app_main.h"
// #include "le_multi_trans.h"
#include "system/includes.h"
#include "app_msg.h"
#include "app_tone.h"
#include "sbox_core_config.h"
#include "battery_manager.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & JL_SBOX_EN)

#if 1
#define log_info(x, ...)  printf("[sbox_app]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

struct user_time phone_time;
struct sbox_state_info user_info;

// u16 check_handle=0;

struct s_box_app_cb sbox_cb_func = {
    .sbox_sync_all_info = custom_sync_all_info_to_box,
    .sbox_sync_volume_info = custom_sync_volume_state,
    .sbox_sync_time_info = custom_sync_time_info,
    .sbox_sync_bt_con_state = custom_sync_bt_connect_state,
    .sbox_sync_ble_con_state = custom_sync_ble_connect_state,
    .sbox_sync_battery_info = custom_sync_vbat_percent_state,
    .sbox_sync_eq_info = custom_sync_eq_info_to_box,
    .sbox_sync_anc_info = custom_sync_anc_info_to_box,
    .sbox_sync_call_state = NULL,//custom_sync_call_state,
    .sbox_sync_key_info =custom_sync_key_setting,
    .sbox_sync_edr_info =custom_sync_edr_info,
    .sbox_sync_phone_call_info = custom_sync_phone_call_info,
    // .sbox_sync_lyric_info =  custom_sync_lyric_info,

    .sbox_control_volume = custom_control_volume,
    .sbox_control_music_state =custom_control_music_state,
    .sbox_control_anc= custom_control_anc_mode,
    .sbox_control_eq= custom_control_eq_mode,
    .sbox_control_key = NULL,//custom_control_key_setting,
    .sbox_control_tiktok = NULL,//custom_control_tiltok,
    .sbox_control_photo = NULL,//custom_control_photo,
    .sbox_control_playmode = custom_control_play_mode,
    .sbox_control_language = NULL,//custom_control_tone_language,
    .sbox_control_find_phone =  NULL,//custom_set_find_phone,
    .sbox_control_bt_name = NULL,//custom_control_bt_name,
    .sbox_control_key = NULL,//custom_control_key_setting,
    .sbox_control_phone_out = NULL,//custom_control_phone_out,
    .sbox_control_phone_call = custom_control_phone_call,
    .sbox_control_ble_lantacy= NULL,//custom_ble_into_no_latency,
    .sbox_control_alarm_tone_play = NULL,//custom_control_alarm_toneplay,
    .sbox_control_edr_conn= NULL,//custom_control_edr_conn,
};

void sbox_app_init(void)
{
    sbox_tr_cbuf_init();
}


#if BLE_PROTECT_DEAL

bool user_ctrl_tws_role_switch;
int user_ctrl_tws_role_switch_timer=0;
void user_check_ble_role_switch_succ(void)
{
    tws_api_auto_role_switch_enable();
    user_ctrl_tws_role_switch=0;
}

#endif


//同步耳机所有的协议信息
__attribute__((weak))
void custom_sync_all_info_to_box(void)
{
    u8 all_info[8];
    u8 channel = tws_api_get_local_channel();
    
    all_info[0]=bt_get_total_connect_dev();
    all_info[1]=0;//get_eq_index();
    all_info[2]=0;//anc_mode_get();
    all_info[3]=(a2dp_player_runing()?1:2);//user_info.music_states;
    all_info[4]=app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    if ('L' == channel) {
         all_info[5] = 10*(battery_value_to_phone_level()+1);//get_vbat_percent();
        all_info[6] = get_tws_sibling_bat_persent();
    } else if ('R' == channel) {
        all_info[5] = get_tws_sibling_bat_persent();
        all_info[6] = 10*(battery_value_to_phone_level()+1);//get_vbat_percent();
    } else {
         all_info[5] = 10*(battery_value_to_phone_level()+1);//get_vbat_percent();
         all_info[6] = 10*(battery_value_to_phone_level()+1);
    }
    all_info[7] = user_info.game_mode;
    log_info("custom_sync_all_info_to_box %c L :%d R:%d\n",channel,all_info[5],all_info[6]);
    sbox_ble_att_send_data(CUSTOM_ALL_INFO_CMD, all_info, sizeof(all_info));
}

//同步耳机播歌音量给仓
__attribute__((weak))
void custom_sync_volume_state(void)
{
    log_info("%s \n",__func__);
    u8 cur_vol = 0; 
    cur_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    sbox_ble_att_send_data(CUSTOM_BLE_VOLUMEN_CMD, &cur_vol, sizeof(cur_vol)); //将当前音量发给手表
}
//同步手机时间信息给仓
__attribute__((weak))
void custom_sync_time_info(struct user_time my_time)
{
    struct user_time *my_times = &my_time;
    sbox_ble_att_send_data(CUSTOM_BLE_TIME_DATE_CMD, (u8 *)(my_times), sizeof(my_time));
}

//同步经典蓝牙状态给仓
__attribute__((weak))
void custom_sync_bt_connect_state(u8 value)
{
    log_info("custom_sync_bt_connect_state%d\n",value);
    u8 bt_conn_state = value;
    sbox_ble_att_send_data(CUSTOM_BT_CONNECT_STATE_CMD, &bt_conn_state, sizeof(bt_conn_state)); //将当前蓝牙状态回复给手表
}
//同步BLE状态给仓
__attribute__((weak))
void custom_sync_ble_connect_state(u8 value)
{
    log_info("%s \n",__func__);
    u8 btl_conn_state = value;
    sbox_ble_att_send_data(CUSTOM_BLE_CONNECT_STATE_CMD, &btl_conn_state, sizeof(btl_conn_state)); //将当前BLE状态回复给手表
}
//同步EQ信息给仓
__attribute__((weak))
void custom_sync_eq_info_to_box(void)
{
    log_info("%s \n",__func__);
    u8 eq_info =get_eq_index();
    sbox_ble_att_send_data(CUSTOM_EQ_DATE_CMD, &eq_info, sizeof(eq_info));
}

//同步anc信息给仓
__attribute__((weak))
void custom_sync_anc_info_to_box(void)
{
    log_info("%s \n",__func__);
    u8 anc_info=0;//anc_mode_get();
    sbox_ble_att_send_data(CUSTOM_ANC_DATE_CMD, &anc_info, sizeof(anc_info));
}

//同步耳机电量状态给仓
__attribute__((weak))
void custom_sync_vbat_percent_state(void)
{
    u8 bat_state[3];
    u8 channel = tws_api_get_local_channel();

    // byte0:L耳机电量---- byte1：R耳机电量
    if ('L' == channel) {
        bat_state[0] = 10*(battery_value_to_phone_level()+1);//get_vbat_percent();
        bat_state[1] = get_tws_sibling_bat_persent();
    } else if ('R' == channel) {
        bat_state[0] = get_tws_sibling_bat_persent();
        bat_state[1] = 10*(battery_value_to_phone_level()+1);//get_vbat_percent();
    }else {
        bat_state[0] = 10*(battery_value_to_phone_level()+1);//get_vbat_percent();
        bat_state[1] = 0xff;
    }
    bat_state[2]=bt_get_total_connect_dev(); 
    
    log_info("sscustom_sync_vbat_percent_state bat_states:%x, vbat_percent:%d, tws_vbat_percent:%d", bat_state, get_vbat_percent(), get_tws_sibling_bat_persent());
    sbox_ble_att_send_data(CUSTOM_BLE_BATTERY_STATE_CMD, bat_state, sizeof(bat_state)); //将当前电量状态回复给手表
}

//同步耳机播歌状态给仓
__attribute__((weak))
void custom_sync_music_state(void)
{
    log_info("%s \n",__func__);
    u8 music_state = user_info.music_states;
    sbox_ble_att_send_data(CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD, &music_state, sizeof(music_state)); //将当前音乐状态回复给手表
}

//同步耳机自定义按键状态给仓
__attribute__((weak))
void custom_sync_key_setting(void)
{
    log_info("%s \n",__func__);
}  

//同步手机连接耳机后的通话信息给仓
__attribute__((weak))
void custom_sync_phone_call_info(void)
{
    log_info("%s \n",__func__);
}

//同步耳机信息给给发射器
__attribute__((weak))
void custom_sync_edr_info(void)
{
    log_info("%s \n",__func__);
} 




/*---------------------------------------  功能控制类 -----------------------------------*/

//仓设置耳机播歌的EQ模式,   len:1byte
__attribute__((weak))
void custom_control_eq_mode(void *data)
{
    log_info("%s \n",__func__);
    // custom_eq_param_index_switch(*data);
}

//仓设置耳机anc模式         len:1byte
__attribute__((weak))
void custom_control_anc_mode(void *data)
{
    u8 *datas =(u8*)data;
    log_info("%s \n",__func__);
    if(get_tws_sibling_connect_state()){
       if (*datas == 0x01) { 
            tws_api_sync_call_by_uuid(0xA2122623, SYNC_CMD_ANC_OFF, 200);
        } else if (*datas == 0x02) { 
            tws_api_sync_call_by_uuid(0xA2122623, SYNC_CMD_ANC_ON, 200);
        } else if (*datas == 0x03) {
            tws_api_sync_call_by_uuid(0xA2122623, SYNC_CMD_ANC_TRANS, 200);
        }
    }else{
        if (*datas == 0x01) {
            anc_mode_switch(ANC_OFF, 1); 
        } else if (*datas == 0x02) { // 
            anc_mode_switch(ANC_ON, 1);
        } else if (*datas == 0x03) {
            anc_mode_switch(ANC_TRANSPARENCY, 1);
        }
    }
}

//仓设置耳机播歌音量
__attribute__((weak))
void custom_control_volume(void *datas)
{
    log_info("%s \n",__func__);
    u8 *data = (u8 *)datas;
    if(get_tws_sibling_connect_state())
    {
        if(*data==1){
            tws_api_sync_call_by_uuid(0xA2122623, SYNC_CMD_VOLUME_UP , 100);
        }else{
            tws_api_sync_call_by_uuid(0xA2122623, SYNC_CMD_VOLUME_DOWN , 100);
        }
    }else{
        if(*data==1){
            bt_volume_up(1);
        }else{
            bt_volume_down(1);
        }
        sbox_cb_func.sbox_sync_volume_info();
        // set_lanugae_ver(data);
    
    }
}

//仓设置播歌状态，播放暂停、上下曲
__attribute__((weak))
void custom_control_music_state(void *data)
{
    u8 *bt_addr = NULL;
    u8 *datas = (u8*)data;
    #if TCFG_BT_DUAL_CONN_ENABLE
            
            u8 addr[6];
            if(a2dp_player_get_btaddr(addr)){
                bt_addr = addr;
            }else{
                bt_addr = bt_get_current_remote_addr();
            }
            if (data == 0x01) { // 播放bt_cmd_prepare
                bt_cmd_prepare_for_addr(bt_addr,USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            } else if (data == 0x02) { // 暂停
                bt_cmd_prepare_for_addr(bt_addr,USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            } else if (data == 0x03) { // 上一曲
                bt_cmd_prepare_for_addr(bt_addr,USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
            } else if (data == 0x04) { // 下一曲
                bt_cmd_prepare_for_addr(bt_addr,USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
            } else {
                log_info("CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD volue:%x valid!!!!\n", data);
            }
    #else
        if (*datas == 0x01) { // 播放bt_cmd_prepare
            // bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        } else if (*datas == 0x02) { // 暂停
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        } else if (*datas == 0x03) { // 上一曲
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
        } else if (*datas == 0x04) { // 下一曲
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
        } else {
            log_info("CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD volue:%x valid!!!!\n", *datas);
        }
    #endif
}


//仓设置耳机自定义按键设置
__attribute__((weak))
void custom_control_key_setting(void *data)
{
    log_info("%s \n",__func__);
}

//设置手机的抖音点赞
__attribute__((weak))
void custom_control_tiltok(void *data)
{
    log_info("%s \n",__func__);
    // sbox_ctrl_tiktok(*data);//抖音点赞操作接口
}

//仓设置手机是否拍照
__attribute__((weak))
void custom_control_photo(void *data)
{
    log_info("%s \n",__func__);
    // extern void key_take_photo();
    // key_take_photo();
}

//仓设置耳机进出低延时模式 
__attribute__((weak))
void custom_control_play_mode(void *datas)
{
    log_info("%s \n",__func__);
    u8 *data =(u8 *)datas;
     if(*data == 1){
        bt_set_low_latency_mode(1, 1, 300);
    }else if(*data==2) {
        bt_set_low_latency_mode(0, 1, 300);
    }
    custom_sync_game_mode_state();
}

//仓设置耳机默认提示音语种
__attribute__((weak))
void custom_control_tone_language(void *data)
{
    log_info("%s \n",__func__);
    //to do
}
int smartbox_tone_app_message_deal(u8 opcode);
//仓设置查找耳机
__attribute__((weak))
void custom_control_find_phone(void **data)
{
    log_info("%s \n",__func__);
    if(get_tws_sibling_connect_state()){
        tws_api_sync_call_by_uuid(0x23091217, sbox_tr_rbuf[0], 300);
    }else{
        smartbox_tone_app_message_deal(sbox_tr_rbuf[0]);
    }
}

//仓设置耳机蓝牙名字
__attribute__((weak))
void custom_control_bt_name(void *data)
{
    log_info("%s \n",__func__);
    // sbox_set_bt_name_setting(*data);//todo
}

//仓设置耳机呼出指定电话
__attribute__((weak))
void custom_control_phone_out(void *data)
{
    log_info("%s \n",__func__);
    // u16 len = 
    // if(len<3 || len >30){
    //     log_info("custom_phone_out len error %d\n",len);
    //     return;
    // }
    // put_buf(number,strlen(number));
    // user_send_cmd_prepare(USER_CTRL_DIAL_NUMBER, len, number);
}

//仓设置耳机播放闹钟提示音
__attribute__((weak))
void custom_control_alarm_toneplay(void *data)
{
    log_info("%s \n",__func__);
}

//仓设置断开手机连接，回连仓播歌
__attribute__((weak))
void custom_control_edr_conn(void *data)
{
    log_info("%s \n",__func__);
}



//仓控制耳机通话接听挂断
__attribute__((weak))
void custom_control_phone_call(void *datas)
{
    log_info("%s \n",__func__);
    u8 *data = (u8*)datas;
    u8 state = bt_get_call_status();
    if(state >=BT_CALL_INCOMING && state < BT_CALL_HANGUP){
        switch(*data){
            case 1:
                bt_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
                break;
            case 2:
                bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
                break;
            case 3:
                if(get_tws_sibling_connect_state())
                {
                    tws_api_sync_call_by_uuid(0xA2122623, SYNC_CMD_CALL_MUTE, 200);
                }else{
                    user_info.phone_call_mute=(!user_info.phone_call_mute);
                }
                break;
            case 4: 
                if(get_tws_sibling_connect_state())
                {
                    tws_api_sync_call_by_uuid(0xA2122623, SYNC_CMD_CALL_MUTE, 200);
                }else{
                    user_info.phone_call_mute=(!user_info.phone_call_mute);
                }
                break;
        default:
                break;
        }
    }
}
//设置BLE不进入低功耗模式，从而提高响应速度，但会增加设备的功耗。
__attribute__((weak))
void custom_ble_into_no_latency(u8 enable)
{
    log_info("%s \n",__func__);
    // if(get_ble_conn_handle_state()){
    //     log_info("custom_ble_into_no_latency %d\n",enable);
        if(enable){
            ble_op_latency_skip(0x50,0xffff);
        }else{
            ble_op_latency_skip(0x50,0);
        }
    // }
}


/*-------------------------------------------查找耳机------------------------------------------------------*/

void find_ear_tone_play(u8 tws)
{
    // todo 播查找耳机提示音
    log_info("find_ear_tone_play\n");
    u8 i = 0;
    ring_player_stop();
    if(tws){
        // tws_play_ring_file_alone(get_tone_files()->find_ear,400);
    }else{
        // play_ring_file_alone(get_tone_files()->find_ear);
    }

}
void real_alarm_tone_play(u8 tws)
{
    // todo 播闹钟提示音
    log_info("alarm_tone_play\n");
    u8 i = 0;
    ring_player_stop();
    if(tws){
        // tws_play_ring_file_alone(get_tone_files()->alarm,400);
    }else{
        // play_ring_file_alone(get_tone_files()->alarm);
    }
}
void smartbox_ring_set(u8 en, u8 ch)
{
    log_info("%s en: %d ch: %d tws_state %d role %d\n",__func__,en,ch,get_tws_sibling_connect_state(),tws_api_get_role()); 
    if (!en) {  
        if (((ch & RING_CH_L) && tws_api_get_local_channel() == 'L')
            || ((ch & RING_CH_R) && tws_api_get_local_channel() == 'R')) {
            // if (!tone_name_compare(TONE_RING)) {
                ring_player_stop();
            // }
        }
        return;
    }

     if (ch == RING_CH_LR_ALARM) {
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state()) {
            if (tws_api_get_role() == 0) { //从机发起同步
                tws_api_sync_call_by_uuid(0x23081717, 2, 300);
            }
        } else
#endif
        {
            real_alarm_tone_play(0);
        }
        return;
    }

    if (ch == RING_CH_LR) {
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state()) {
            if (tws_api_get_role() == 0) { //从机发起同步
                tws_api_sync_call_by_uuid(0x23081717, 1, 300);
               
            }
        } else
#endif
        {
            find_ear_tone_play(0);

        }
        return;
    }
    if (((ch & RING_CH_L) && tws_api_get_local_channel() == 'L')
        || ((ch & RING_CH_R) && tws_api_get_local_channel() == 'R')) {
        find_ear_tone_play(0);

    }
}

int smartbox_tone_app_message_deal(u8 opcode)
{
    switch (opcode) {
    case RING_CH_LR_ALARM:
        log_info("SMARTBOX_RING_ALL");
        smartbox_ring_set(1, RING_CH_LR_ALARM);
        break;
    case RING_CH_LR:
        log_info("SMARTBOX_RING_ALL");
        smartbox_ring_set(1, RING_CH_LR);
        break;
    case RING_CH_R:
        log_info("SMARTBOX_RING_RIGHT");
        smartbox_ring_set(1, RING_CH_R);
        smartbox_ring_set(0, RING_CH_L);
        break;                 
    case RING_CH_L:
        log_info("SMARTBOX_RING_LEFT");
        smartbox_ring_set(1, RING_CH_L);
        smartbox_ring_set(0, RING_CH_R);
        break;
    case RING_CH_LR_ALL_OFF:
        printf("GFPS_RING_STOP_ALL");
        smartbox_ring_set(0, RING_CH_LR);
        break;
    }
    return 1;
}

TWS_SYNC_CALL_REGISTER(tws_smartbox_sync_tone_play) = {
    .uuid = 0x23091217,
    .task_name = "app_core",
    .func = smartbox_tone_app_message_deal,
};

/**
 * @brief 响铃函数
 *
 * @param en 1为响铃，0为不响铃
 * @param ch 设置的通道号
 */
static void smartbox_tone_play_at_same_time(int event, int err)
{
    log_info("%s\n",__func__);
    if (!get_tws_sibling_connect_state()) {
        log_info("%s tws_disconn, ignore", __func__);
        return;
    }
    if (tws_api_get_role() == TWS_ROLE_MASTER && err != TWS_SYNC_CALL_TX) { //主机发送失败，重新发送
        tws_api_sync_call_by_uuid(0x23081717, event, 300);
        return;
    }
    if(event==2){
        real_alarm_tone_play(1);
        return;
    }
    //  real_alarm_tone_play(1);
    find_ear_tone_play(event);
}

TWS_SYNC_CALL_REGISTER(tws_smartbox_tone_play) = {
    .uuid = 0x23081717,
    .task_name = "app_core",
    .func = smartbox_tone_play_at_same_time,
};
/*-------------------------------------------查找耳机------------------------------------------------------*/


//同步执行对应功能
static void sbox_tws_app_info_sync(u8 cmd)
{
    log_info("%s cmd:%d\n",__func__,cmd);
    switch (cmd)
    {
        case SYNC_CMD_EQ_SWITCH_0:
            // custom_eq_param_index_switch(0);
            break;
        case SYNC_CMD_EQ_SWITCH_1:
            // custom_eq_param_index_switch(1);
            break;
        case SYNC_CMD_EQ_SWITCH_2:
            // custom_eq_param_index_switch(2);
            break;
        case SYNC_CMD_EQ_SWITCH_3:
            // custom_eq_param_index_switch(3);
            break;
        case SYNC_CMD_EQ_SWITCH_4:
            // custom_eq_param_index_switch(4);
            break;
        case SYNC_CMD_ANC_ON:
            // anc_mode_switch(ANC_ON, 1);
            break;
        case SYNC_CMD_ANC_OFF:
            // anc_mode_switch(ANC_OFF, 1);
            break;
        case SYNC_CMD_ANC_TRANS:
            // anc_mode_switch(ANC_TRANSPARENCY, 1); 
            break;
        case SYNC_CMD_VOLUME_UP:
            bt_volume_up(1);
            if(tws_api_get_role()==0){
                sbox_cb_func.sbox_sync_volume_info();
                // custom_sync_volume_state();
            }
            break;
        case SYNC_CMD_VOLUME_DOWN:
            bt_volume_down(1);
            if(tws_api_get_role()==0){
                sbox_cb_func.sbox_sync_volume_info();
            }
            break;
        case SYNC_CMD_CALL_MUTE:

            // user_info.phone_call_mute=(!user_info.phone_call_mute);
            break;
        case SYNC_CMD_SBOX_POWER_OFF_TOGETHER:
            extern void sys_enter_soft_poweroff(enum poweroff_reason reason);
            sys_enter_soft_poweroff(POWEROFF_NORMAL_TWS);
            break;
    default:

        break;
    }
}

TWS_SYNC_CALL_REGISTER(sbox_app_info_sync) = {
    .uuid = 0xA2122623,
    .task_name = "app_core",
    .func = sbox_tws_app_info_sync,
};



/*----------------------------------------------------功能应用相关------------------------------------------------------------------------*/



/*-------------------------------------------------状态同步相关------------------------------------------------------------------------*/

static int bt_call_status_event_handler(int *msg)
{
     u8 *phone_number = NULL;
    struct bt_event *bt = (struct bt_event *)msg;

    if (tws_api_get_role_async() == TWS_ROLE_SLAVE) {
        return 0;
    }

    switch (bt->event) {
    case BT_STATUS_PHONE_INCOME:
        sbox_cb_func.sbox_sync_call_state(SBOX_CALL_INCOME);
        // custom_sync_call_state(SBOX_CALL_INCOME);
        break;
    case BT_STATUS_PHONE_HANGUP:
        sbox_cb_func.sbox_sync_call_state(SBOX_CALL_HANDUP);
        // custom_sync_call_state(SBOX_CALL_HANDUP);
        break;
    case BT_STATUS_PHONE_ACTIVE:
        sbox_cb_func.sbox_sync_call_state(SBOX_CALL_ACTIVE);
        // custom_sync_call_state(SBOX_CALL_ACTIVE);
        break;
    default:
        break;
    }
}

APP_MSG_HANDLER(bt_call_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = bt_call_status_event_handler,
};


//蓝牙事件回调  用于同步一些状态信息
static int sbox_btstack_event_handler(int *_event)
{
    struct bt_event *event = (struct bt_event *)_event;
    log_info("sbox_btstack_event_handler:%d\n",event->event);
    put_buf(event->args,6);
    int a2dp_addr_is_same=0;
    static u8 last_addr[6] ={0xff};
    put_buf(last_addr,6);
    u8 tmpbuf[6] = {0xff};
    switch (event->event) {
    case BT_STATUS_INIT_OK:
        // sbox_cfg_init();
        break;
    case BT_STATUS_FIRST_CONNECTED:
    case BT_STATUS_SECOND_CONNECTED:

        break;
    case BT_STATUS_CONN_A2DP_CH:
    case BT_STATUS_CONN_HFP_CH:
        sbox_cb_func.sbox_sync_all_info();
        break;
    case BT_STATUS_FIRST_DISCONNECT:
        sys_timeout_add(NULL, sbox_cb_func.sbox_sync_all_info,1000);
    case BT_STATUS_SECOND_DISCONNECT:

        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
        u8 connect_num = bt_get_total_connect_dev();
        log_info("sbox_btstack_event_handler:%d connect_num%d value%d user_info.music_states%d\n",event->event,connect_num,event->value,user_info.music_states);
    #if TCFG_BT_DUAL_CONN_ENABLE
        if(connect_num ==1 ){
            if (event->value == AVC_PLAY) {
                user_info.music_states=1;
                custom_sync_music_state_pro(1);
            } else if (event->value == AVC_PAUSE) {
                user_info.music_states=0;
                custom_sync_music_state_pro(2);
            }
        }else if(connect_num ==2){
            if(memcmp(last_addr,tmpbuf,6) == 0){
                if (event->value == AVC_PLAY) {
                    memcpy(last_addr,event->args,6);
                    user_info.music_states=1;
                    custom_sync_music_state_pro(1);
                } else if (event->value == AVC_PAUSE) {
                    user_info.music_states=0;
                    memset(last_addr,0,6);
                    custom_sync_music_state_pro(2);
                }
            }else{
                if(memcmp(last_addr,event->args,6) == 0){
                    a2dp_addr_is_same =1;
                }else{
                    a2dp_addr_is_same =0;
                }

                if ((event->value == AVC_PLAY) && user_info.music_states && (a2dp_addr_is_same==0)) {//一拖二播歌抢断的情况
                    putchar('F');
                    memcpy(last_addr,event->args,6);
                } else if ((event->value == AVC_PLAY) && (user_info.music_states == 0)) {
                    putchar('G');
                    user_info.music_states=1;
                    memcpy(last_addr,event->args,6);
                    custom_sync_music_state_pro(1);
                }else if ((event->value == AVC_PAUSE) &&  user_info.music_states && (a2dp_addr_is_same==0)) {
                    putchar('H');
                    // user_info.music_states=0;
                    // custom_sync_music_state_pro(2);
                    // memset(last_addr,0,6);
                }else if ((event->value == AVC_PAUSE) &&  user_info.music_states && (a2dp_addr_is_same==1)) {
                    putchar('J');
                    user_info.music_states=0;
                    custom_sync_music_state_pro(2);
                    memset(last_addr,0,6);
                }
            }
        }
    
       
        // memcpy(last_addr,event->args,6);
    #else
        if (event->value == AVC_PLAY) {
            user_info.music_states=1;
            custom_sync_music_state();
        } else if (event->value == AVC_PAUSE) {
            user_info.music_states=2;
            custom_sync_music_state();
        }
    #endif
        break;
    }
}

APP_MSG_HANDLER(sbox_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = sbox_btstack_event_handler,
};

#define TWS_FUNC_ID(a, b, c, d)     (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#define TWS_FUNC_ID_GAME_MODE_SYNC     TWS_FUNC_ID('S', 'G', 'B', 'E')
extern void bt_set_low_latency_mode(int enable, u8 tone_play_enable, int delay_ms);
void tws_sync_game_mode(void *_data, u16 len, bool rx)
{
    if(rx){
        u8 data = (u8) _data;
        if((data || user_info.game_mode) && (tws_api_get_role()==0)){
            bt_set_low_latency_mode(1,1,300);
        }
    }
}

void tws_sync_game_info(void)
{   
    u8 data = user_info.game_mode;
    r_printf("tws_sync_game_info: %d\n",data);
    tws_api_send_data_to_sibling(data,1,TWS_FUNC_ID_GAME_MODE_SYNC);
}

REGISTER_TWS_FUNC_STUB(game_mode_sync) = {
    .func_id = TWS_FUNC_ID_GAME_MODE_SYNC,
    .func    = tws_sync_game_mode,
};



// #define TWS_FUNC_ID_BLE_ADDR_SYNC     TWS_FUNC_ID('B', 'A', 'R', 'S')

// void tws_sync_box_addr(void *_data, u16 len, bool rx)
// {
//     r_printf("tws_sync_box_addr:\n");
//     if(rx){
//         u8 *data = (u8*) _data;
//         put_buf(_data,6);
//         memcpy(user_info.box_addr,data,6);
//         set_box_mac(user_info.box_addr);
//     }
// }

// void tws_sync_box_addr_info(u8 *addr)
// {   
//     printf("tws_sync_box_addr_info\n");
//     put_buf(addr,6);
//     u8 ble_mac[6] = {0};
//     memcpy(ble_mac,addr,6);
//     memcpy(user_info.box_addr,addr,6);
//     set_box_mac(user_info.box_addr);
//     tws_api_send_data_to_sibling(ble_mac,6,TWS_FUNC_ID_BLE_ADDR_SYNC);
// }

// REGISTER_TWS_FUNC_STUB(ble_mac_sync) = {
//     .func_id = TWS_FUNC_ID_BLE_ADDR_SYNC,
//     .func    = tws_sync_box_addr,
// };

void tws_conn_snyc_info(void);
//对耳事件回调  用于同步一些状态信息

void custom_set_find_phone(u8* data)
{
    if(get_tws_sibling_connect_state())
    {
        tws_api_sync_call_by_uuid(0x23091217, *data, 300);
    }else{
        smartbox_tone_app_message_deal(*data);
    }
}


static void custom_ctrle_emitter_info(u8 *data, u16 len);
void custom_ctrle_edr_conn(u8 cmd);

//仓协议事件具体执行
void sys_smartstore_event_handle(struct box_info *boxinfo)
{
    u8 addr[6];
    u32 len = 0;
    u8 cmd = 0;
    int ret = sbox_tr_cbuf_read(sbox_tr_rbuf, &len, &cmd);//栈变量取值可能会死机
    if(ret < 0){
        log_info("%s[line:%d] read cbuf error!!!\n",__func__,__LINE__);
        return;
    }
__recmd:
    log_info("sys_smartstore_event_handle %x %d\n",cmd,len);
    switch (cmd)
    {
        case CUSTOM_ALL_INFO_CMD: 
            log_info("case CUSTOM_ALL_INFO_CMD\n");

            break;
        case CUSTOM_BLE_PLAY_MODE_CONTROL_CMD:
            log_info("CUSTOM_BLE_PLAY_MODE_CONTROL_CMD \n");

            break;
        case CUSTOM_BLE_FINE_EARPHONE_CMD://0x38
            log_info("CUSTOM_BLE_FINE_EARPHONE_CMD \n");
        //    sbox_cb_func.sbox_control_find_phone(&sbox_tr_rbuf[0]);
            break;
        case CUSTOM_BLE_VOL_CONTROL_CMD:
            log_info("CUSTOM_BLE_VOL_CONTROL_CMD %d\n",sbox_tr_rbuf[0] );
            sbox_cb_func.sbox_control_volume(&sbox_tr_rbuf[0]); 
            break;
        case CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD:
            log_info("CUSTOM_BLE_MUSIC_STATE_CONTROL_CMD\n");
            sbox_cb_func.sbox_control_music_state(&sbox_tr_rbuf[0]);
            break;
        case CUSTOM_BLE_ANC_MODE_CONTROL_CMD:
            log_info("CUSTOM_BLE_ANC_MODE_CONTROL_CMD\n");
            sbox_cb_func.sbox_control_anc(&sbox_tr_rbuf[0]);
            break;
        case CUSTOM_BLE_EQ_MODE_CONTROL_CMD:
            log_info("CUSTOM_BLE_EQ_MODE_CONTROL_CMD\n");
            sbox_cb_func.sbox_control_eq(&sbox_tr_rbuf[0]);
            break;
        case CUSTOM_BLE_CONTRAL_DOUYIN:
            log_info("CUSTOM_BLE_CONTRAL_DOUYIN\n");
            // sbox_ctrl_douyin(sbox_tr_rbuf[0]);
            break;
        case CUSTOM_BLE_CONTRAL_PHOTO:
            log_info("CUSTOM_BLE_CONTRAL_PHOTO\n");
            // custom_ctrl_phone_photo();
            break;
        case CUSTOM_EDR_CONTRAL_CONN:
            log_info("CUSTOM_EDR_CONTRAL_CONN\n");
            // custom_ctrle_edr_conn(sbox_tr_rbuf[0]);
            break;
        case CUSTOM_EDR_SYNC_INFO:
            log_info("CUSTOM_EDR_CONTRAL_CONN\n");
            sbox_cb_func.sbox_sync_all_info();// custom_sync_all_info_to_box();//仓同步过来手机地址的时候也同步下耳机的所有信息
            // custom_ctrle_emitter_info(data,boxinfo.lens);
            break;
        case CUSTOM_BLE_CONTRAL_CALL:
            sbox_cb_func.sbox_control_phone_call(&sbox_tr_rbuf[0]);
            break;
        default:
            break;
    }
    ret =0;
    ret = sbox_tr_cbuf_read(sbox_tr_rbuf, &len, &cmd);//栈变量取值可能会死机
    if(ret < 0){
        log_info("%s[line:%d] read cbuf return\n",__func__,__LINE__);
        return;
    }else{
        log_info("%s[line:%d] read cbuf recmd\n",__func__,__LINE__);
        goto __recmd;
    }
}

static int sbox_app_power_event_handler(int *msg)
{
     switch (msg[0]) {
    case POWER_EVENT_SYNC_TWS_VBAT_LEVEL:
        printf("update sbox bat");
        sbox_cb_func.sbox_sync_battery_info();
        break;
    case POWER_EVENT_POWER_CHANGE:
        printf("update sbox bat");
        sbox_cb_func.sbox_sync_battery_info();
        break;
    }
    return 0;
}

APP_MSG_HANDLER(sbox_level_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = sbox_app_power_event_handler,
};

#endif

