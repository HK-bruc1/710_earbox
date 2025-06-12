
#include "sbox_user_app.h"
#include "classic/tws_api.h"
#include "btstack/avctp_user.h"
#include "app_main.h"


static u8 default_key_set[8] = {
    SBOX_VOL_UP,        SBOX_PLAY_PAUSE,           SBOX_MUSIC_PREV,                SBOX_ANC_MODE,\
        SBOX_VOL_DOWN,        SBOX_PLAY_PAUSE,           SBOX_MUSIC_PREV,                SBOX_ANC_MODE
        };

void set_sbox_key(u8 *my_setting)
{
    if(my_setting == NULL){
        return;
    }
    printf("set_sbox_key\n");
    memcpy(&sbox_user_c_info.default_key_set,my_setting,8);

}

void get_sbox_key(u8 *my_setting)
{
    if(my_setting == NULL){
        return;
    }

    memcpy(my_setting,&sbox_user_c_info.default_key_set,8);
}

static void update_key_setting_vm_value(u8 *my_setting)
{

    int ret=syscfg_write(CFG_USER_KEYSETTING, my_setting, 8);
    if(ret == 8){
        printf("update_key_setting_vm_value success\n");
    }else{
        printf("update_key_setting_vm_value error\n");
    }
}

static void key_setting_sync(void);



void deal_sbox_key_setting(u8 *key_setting_info, u8 write_vm, u8 tws_sync)
{
    putchar('s');
    u8 key_setting[8];
    if (!key_setting_info) {
        get_sbox_key(&key_setting);
    } else {
        putchar('a');
        memcpy(&key_setting, key_setting_info, 8);
        set_sbox_key(key_setting_info);

    }
    // 1、写入VM
    if (write_vm) {
        putchar('g');
        update_key_setting_vm_value(&key_setting);
    }
    // 2、同步对端
    if (tws_sync) {
        putchar('l');
        key_setting_sync();
    }
}

void sbox_keysetting_test()
{
    u8 key_setting[8] = {7,2,5,6,7,2,5,6};
    if(tws_api_get_role()==0){
        deal_sbox_key_setting(key_setting,1,1);
    }
    
}

static void key_setting_sync(void)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_sbox_setting(SBOX_TYPE_KEY_SETTING);
    }
#endif
}

int sbox_custom_key_event_handler(u8 func)
{
    printf("sbox_custom_key_event_handler %d \n",func);
    switch (func) {
        case SBOX_PLAY_PAUSE:
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
            break;
        case SBOX_MUSIC_PREV:
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
            break;
        case SBOX_MUSIC_NEXT:
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
            break;
        case SBOX_VOICE_ASSISTANT:
            if (app_var.siri_stu || bt_phone_dec_is_running()) {
                user_send_cmd_prepare(USER_CTRL_HFP_GET_SIRI_CLOSE, 0, NULL);
            } else {
                user_send_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
            }
            break;
        case SBOX_LOW_LATENCY:
            bt_set_low_latency_mode(!bt_get_low_latency_mode());
            break;
        case SBOX_ANC_MODE:
            anc_mode_next();
            break;
        case SBOX_VOL_UP:
            volume_up();
            break;
        case SBOX_VOL_DOWN:
            volume_down();
            break;
        case SBOX_EQ_SWITCH:
            eq_mode_sw();
            sbox_user_c_info.eq_inidex++;
            if(sbox_user_c_info.eq_inidex == EQ_MODE_MAX){
                sbox_user_c_info.eq_inidex=0;
            }
            break;
        case SBOX_NO_FUNTION: 
            
            break;
        default:

            break;
    }
}

void set_key_event_by_sbox_info(struct sys_event *event, u8 *key_event)
{
    u8 key_action = 0;
    struct key_event *key = &event->u.key;
    int ret;
    u8 func;
#if (TCFG_USER_TWS_ENABLE)
    u8 channel = tws_api_get_local_channel();
    if (get_bt_tws_connect_status()) {
        channel = tws_api_get_local_channel();
    }
#endif

    switch (channel) {
    case 'U':
    case 'L':
        channel = (u32) event->arg == KEY_EVENT_FROM_TWS ? 1 : 0;
        break;
    case 'R':
        channel = (u32) event->arg == KEY_EVENT_FROM_TWS ? 0 : 1;
        break;
    default:
        channel = 0;
        return;
    }

    switch (key->event) {
    case KEY_EVENT_CLICK:
        key_action = SBOX_ONE_CLICK;
        break;
    case KEY_EVENT_DOUBLE_CLICK:
        key_action = SBOX_DOUBLE_CLICK;
        break;
    case KEY_EVENT_TRIPLE_CLICK:
        key_action = SBOX_THIRD_CLICK;
        break;
    case KEY_EVENT_LONG:
        key_action = SBOX_LONG_PRESS;
        break;
    default:
        key_action = SBOX_OP_NULL;
        break;;
    }
    if (key_action != 0xff) {
        func = sbox_user_c_info.default_key_set[channel*4 + key_action];
        ret = sbox_custom_key_event_handler(func);
    }
    printf("ch:%d, act:%d, func:%d", channel, key_action, func);
}

void sbox_key_init(void)
{
    uint8_t tmp_key[8] = {0}; // 确保数组大小正确
    int ret = syscfg_read(CFG_USER_KEYSETTING, tmp_key, sizeof(tmp_key));
    if (ret == sizeof(tmp_key)) { // 检查返回值是否等于期望的长度
        printf("%s key_setting_info ok ", __func__);
        for (int i = 0; i < ret; i++) { 
            printf("%02X ", tmp_key[i]);
        }
        printf("\n");
        put_buf(tmp_key, ret);
        set_sbox_key((u8 * *)tmp_key); // 
    } else {
        printf("%s key_setting_info error %d\n", __func__, ret);
        set_sbox_key(&sbox_default_info.default_key_set);
        put_buf(sbox_default_info.default_key_set, 8);
    }
}

void sbox_key_reset(void)
{
    memcpy(&sbox_user_c_info.default_key_set,&sbox_default_info.default_key_set,sizeof(sbox_user_c_info.default_key_set));
    deal_sbox_key_setting(NULL,1,1);
}



