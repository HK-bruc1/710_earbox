#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".key_msg_handler.data.bss")
#pragma data_seg(".key_msg_handler.data")
#pragma const_seg(".key_msg_handler.text.const")
#pragma code_seg(".key_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "app_default_msg_handler.h"
#include "a2dp_player.h"
#include "tws_dual_share.h"
#include "low_latency.h"
#include "poweroff.h"
#include "bt_key_func.h"
#include "low_latency.h"
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
#include "app_le_connected.h"
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)
#include "app_le_auracast.h"
#endif


#define LOG_TAG             "[EARPHONE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#include "app_tone.h"



#if TCFG_ADKEY_ENABLE
const int adkey_msg_table[10][KEY_ACTION_MAX] = {
    //短按, 长按, hold, 长按抬起,
    //双击, 3击, 4击, 5击,
    //按住3s, 按住5s
    [0] = {
        APP_MSG_MUSIC_PP,   APP_MSG_CALL_HANGUP,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_LOW_LANTECY,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_POWER_OFF,
    },
    [1] = {
        APP_MSG_MUSIC_NEXT, APP_MSG_VOL_UP,   APP_MSG_VOL_UP,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
    [2] = {
        APP_MSG_MUSIC_PREV, APP_MSG_VOL_DOWN,   APP_MSG_VOL_DOWN,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
    [3] = {
        APP_MSG_GOTO_NEXT_MODE,   APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
    [4] = {
        APP_MSG_ANC_SWITCH, APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
    [5] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
    [6] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
    [7] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
    [8] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
    [9] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,
    },
};
#endif

//三方通话
typedef enum{
    BT_THREE_CALL_ACTIVE   = 0,//当前正在和第一个号码通话中，没有来电
    BT_THREE_CALL_INCOMING = 2,//当前正在和第一个号码通话中，第二个电话进来
    BT_THREE_CALL_COMING   = 8,//当前已经保留了一个电话，正在和另外一个手机通话
}BT_THREE_CALL_IND_STA;
int bt_get_phone_state(void *device)
{
    int state = btstack_bt_get_call_status(device);
    if (state == BT_SIRI_STATE &&
        btstack_get_call_esco_status(device) != BT_ESCO_STATUS_OPEN) {
        state = BT_CALL_HANGUP;
    }
    return state;
}

extern int get_second_call_status(void);
int bt_key_power_msg_remap(int *msg)
{
    char channel;
    int app_msg = APP_MSG_NULL;
    u8 key_action = APP_MSG_KEY_ACTION(msg[0]);

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        //按键消息从机都不执行？下面的操作主机都能同步操作？
        return APP_MSG_NULL;
    }

    void *devices[2];
    void *active_device = NULL;
    void *incoming_device = NULL;
    void *outgoing_device = NULL;
    void *siri_device = NULL;
    //获取本地声道，只能用于配对方式选择了固定左右耳宏的?
    channel = tws_api_get_local_channel();

    int tws_state = tws_api_get_tws_state();
    int num = btstack_get_conn_devices(devices, 2);
    for (int i = 0; i < num; i++) {
        int state = bt_get_phone_state(devices[i]);
        if (state == BT_CALL_ACTIVE) {
            active_device = devices[i];
        } else if (state == BT_CALL_INCOMING) {
            incoming_device = devices[i];
        } else if (state == BT_CALL_OUTGOING || state == BT_CALL_ALERT) {
            outgoing_device = devices[i];
        } else if (state == BT_SIRI_STATE) {
            siri_device = devices[i];
        }
    }
    /* 通话相关场景下按键流程-------------------------------------------------------------------------------- */
    if (active_device) {
        //通话状态的通话场景
        switch (key_action) {
        case KEY_ACTION_CLICK:
// #if _THREE_CALL_EN
//         if(get_second_call_status() == BT_THREE_CALL_ACTIVE){
//             //当前正在和第一个号码通话中，没有来电
//             app_msg = _ACTIVE_DEVICE_KEY_ACTION_CLICKBT_BT_THREE_CALL_ACTIVE;
//             log_info("active_device-----KEY_ACTION_CLICK-----BT_THREE_CALL_ACTIVE-------------------------------------------------------   \n");
//             break;
//         }else if(get_second_call_status() == BT_THREE_CALL_INCOMING){
//             //当前正在和第一个号码通话中，第二个电话进来
//             app_msg = _ACTIVE_DEVICE_KEY_ACTION_CLICKBT_BT_THREE_CALL_INCOMING;
//             log_info("active_device-----KEY_ACTION_CLICK-----BT_THREE_CALL_INCOMING-------------------------------------------------------   \n");
//             break;
//         }else if(get_second_call_status() == BT_THREE_CALL_COMING){
//             //当前已经保留了一个电话，正在和另外一个手机通话
//             app_msg = _ACTIVE_DEVICE_KEY_ACTION_CLICKBT_BT_THREE_CALL_COMING;
//             log_info("active_device-----KEY_ACTION_CLICK-----BT_THREE_CALL_COMING-------------------------------------------------------   \n");
//             break;
//         }
// #endif
            //先判断是否有三方通话再处理普通单击
            app_msg = APP_MSG_CALL_HANGUP;
            break;

//         case KEY_ACTION_DOUBLE_CLICK:
// #if _TONE_KEY_ACTION_DOUBLE_CLICK_ACTIVE_DEVICE_ENABLE
//         play_tone_file(get_tone_files()->_TONE_KEY_ACTION_DOUBLE_CLICK_ACTIVE_DEVICE_NAME);
// #endif
// #if _THREE_CALL_EN
//             if(get_second_call_status() == BT_THREE_CALL_ACTIVE){
//                 //当前正在和第一个号码通话中，没有来电
//                 app_msg = _ACTIVE_DEVICE_KEY_ACTION_DOUBLE_CLICK_BT_THREE_CALL_ACTIVE;
//                 log_info("active_device-----KEY_ACTION_DOUBLE_CLICK-----BT_THREE_CALL_ACTIVE-------------------------------------------------------   \n");
//                 break;
//             }else if(get_second_call_status() == BT_THREE_CALL_INCOMING){
//                 //当前正在和第一个号码通话中，第二个电话进来
//                 app_msg = _ACTIVE_DEVICE_KEY_ACTION_DOUBLE_CLICK_BT_THREE_CALL_INCOMING;
//                 log_info("active_device-----KEY_ACTION_DOUBLE_CLICK-----BT_THREE_CALL_INCOMING-------------------------------------------------------   \n");
//                 break;
//             }else if(get_second_call_status() == BT_THREE_CALL_COMING){
//                 //当前已经保留了一个电话，正在和另外一个手机通话
//                 app_msg = _ACTIVE_DEVICE_KEY_ACTION_DOUBLE_CLICK_BT_THREE_CALL_COMING;
//                 log_info("active_device-----KEY_ACTION_DOUBLE_CLICK-----BT_THREE_CALL_COMING-------------------------------------------------------   \n");
//                 break;
//             }
// #endif
//             //先判断是否有三方通话再处理普通双击
//             app_msg = _ACTIVE_DEVICE_KEY_ACTION_DOUBLE_CLICK;
//             break;

//         case KEY_ACTION_LONG:
// #if _THREE_CALL_EN
//             if(get_second_call_status() == BT_THREE_CALL_ACTIVE){
//                 //当前正在和第一个号码通话中，没有来电
//                 app_msg = _ACTIVE_DEVICE_KEY_ACTION_LONG_BT_THREE_CALL_ACTIVE;
//                 log_info("active_device-----KEY_ACTION_LONG-----BT_THREE_CALL_ACTIVE-------------------------------------------------------   \n");
//                 break;
//             }else if(get_second_call_status() == BT_THREE_CALL_INCOMING){
//                 //当前正在和第一个号码通话中，第二个电话进来
//                 app_msg = _ACTIVE_DEVICE_KEY_ACTION_LONG_BT_THREE_CALL_INCOMING;
//                 log_info("active_device-----KEY_ACTION_LONG-----BT_THREE_CALL_INCOMING-------------------------------------------------------   \n");
//                 break;
//             }else if(get_second_call_status() == BT_THREE_CALL_COMING){
//                 //当前已经保留了一个电话，正在和另外一个手机通话
//                 app_msg = _ACTIVE_DEVICE_KEY_ACTION_LONG_BT_THREE_CALL_COMING;
//                 log_info("active_device-----KEY_ACTION_LONG-----BT_THREE_CALL_COMING-------------------------------------------------------   \n");
//                 break;
//             }
// #endif
//             //先判断是否有三方通话再处理普通长按
//             app_msg = _ACTIVE_DEVICE_KEY_ACTION_LONG;
//             break;

        default:
            break;
        }
    } else if (incoming_device) {
        //通话状态的来电场景
        switch (key_action) {
        case KEY_ACTION_CLICK:
#if _TONE_KEY_ACTION_CLICK_INCOMING_DEVICE_ENABLE
                play_tone_file(get_tone_files()->_TONE_KEY_ACTION_CLICK_INCOMING_DEVICE_NAME);
#endif
            app_msg = APP_MSG_CALL_ANSWER;    
            break;

//         case KEY_ACTION_DOUBLE_CLICK: 
// #if _TONE_KEY_ACTION_DOUBLE_CLICK_INCOMING_DEVICE_ENABLE
//                 play_tone_file(get_tone_files()->_TONE_KEY_ACTION_DOUBLE_CLICK_INCOMING_DEVICE_NAME);
// #endif
//             app_msg = _INCOMING_DEVICE_KEY_ACTION_DOUBLE_CLICK;    
//             break;

//         case KEY_ACTION_TRIPLE_CLICK: 
//             app_msg = _INCOMING_DEVICE_KEY_ACTION_TRIPLE_CLICK;      
//             break;

        case KEY_ACTION_LONG:  
            app_msg = APP_MSG_CALL_HANGUP;     
            break;

        default:
            break;
        }
    } else if (outgoing_device) {
        switch (key_action) {
        case KEY_ACTION_CLICK:   
            //app_msg = _OUTGOING_DEVICE_KEY_ACTION_CLICK;       
            break;

        default:
            break;
        }
    } else if (siri_device) {
        switch (key_action) {
        case KEY_ACTION_CLICK:
            //app_msg = _SIRI_DEVICE_KEY_ACTION_CLICK;    
            break;
        case KEY_ACTION_DOUBLE_CLICK:
            //通话中的语音助手，比如小爱同学接听
            //app_msg = _SIRI_DEVICE_KEY_ACTION_DOUBLE_CLICK;       
            break;

        default:
            break;
        }
    } else {
        /* 非通话相关场景下按键流程------------------------------------------------------------------------------- */
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
        int tws_cig_state = is_cig_phone_conn();
        if ((tws_state & TWS_STA_PHONE_CONNECTED) || tws_cig_state) { //已连接手机经典蓝牙或者cig
#elif (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)
        if ((tws_state & TWS_STA_PHONE_CONNECTED) && (le_auracast_status_get() != BROADCAST_STATUS_START)) { //已连接手机经典蓝牙&&耳机没有auracast播歌
#else
        if (tws_state & TWS_STA_PHONE_CONNECTED) { //已连接手机经典蓝牙
#endif
            switch (key_action) {
            case KEY_ACTION_CLICK:
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        app_msg = APP_MSG_MUSIC_PP;
                        break;
                    }else {
                        app_msg = APP_MSG_MUSIC_PP;
                        break;
                    }
                }
                // else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                //     if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                //         (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                //         app_msg = APP_MSG_MUSIC_PP;
                //         break;
                //     }else {
                //         app_msg = APP_MSG_MUSIC_PP;
                //         break;
                //     }
                // }
                break;

            case KEY_ACTION_DOUBLE_CLICK:
#if _TONE_KEY_ACTION_DOUBLE_CLICK_ENABLE
                play_tone_file(get_tone_files()->_TONE_KEY_ACTION_DOUBLE_CLICK_NAME);
#endif
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //定义左耳，功能却是反的。。。
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_DOUBLE_CLICK_L;
                        break;
                    }else {
                        app_msg = APP_MSG_LOW_LANTECY;
                        break;
                    }
                }
                // else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                //     if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                //         (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                //         app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_DOUBLE_CLICK_L;
                //         break;
                //     }else {
                //         app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_DOUBLE_CLICK_R;
                //         break;
                //     }
                // }
                break;

            case KEY_ACTION_TRIPLE_CLICK:
#if _TONE_KEY_ACTION_TRIPLE_CLICK_ENABLE
                play_tone_file(get_tone_files()->_TONE_KEY_ACTION_TRIPLE_CLICK_NAME);
#endif
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_TRIPLE_CLICK_L;
                        break;
                    }else {
                        app_msg = APP_MSG_OPEN_SIRI; 
                        break;
                    }
                }
                // else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                //     if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                //         (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                //         app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_TRIPLE_CLICK_L;
                //         break;
                //     }else {
                //         app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_TRIPLE_CLICK_R;
                //         break;
                //     }
                // } 
                break; 
            
            case KEY_ACTION_FOURTH_CLICK:
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        app_msg = APP_MSG_VOL_DOWN;
                        break;
                    }else {
                        app_msg = APP_MSG_VOL_UP; 
                        break;
                    }
                }
                // else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                //     if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                //         (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                //         app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_FOURTH_CLICK_L;
                //         break;
                //     }else {
                //         app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_FOURTH_CLICK_R;
                //         break;
                //     }
                // } 
                break;

            // case KEY_ACTION_FIFTH_CLICK:
            //     if (tws_state & TWS_STA_SIBLING_CONNECTED) {
            //         if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            //             (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_FIFTH_CLICK_L;
            //             break;
            //         }else {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_FIFTH_CLICK_R; 
            //             break;
            //         }
            //     }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
            //         if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            //             (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_FIFTH_CLICK_L;
            //             break;
            //         }else {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_FIFTH_CLICK_R;
            //             break;
            //         }
            //     } 
            //     break;
            
            // case KEY_ACTION_SEXTUPLE_CLICK:
            //     if (tws_state & TWS_STA_SIBLING_CONNECTED) {
            //         if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            //             (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_SEXTUPLE_CLICK_L;
            //             break;
            //         }else {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_SEXTUPLE_CLICK_R; 
            //             break;
            //         }
            //     }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
            //         if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            //             (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_SEXTUPLE_CLICK_L;
            //             break;
            //         }else {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_SEXTUPLE_CLICK_R;
            //             break;
            //         }
            //     } 
            //     break;
            
            // case KEY_ACTION_SEPTUPLE_CLICK:
            //     if (tws_state & TWS_STA_SIBLING_CONNECTED) {
            //         if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            //             (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_SEPTUPLE_CLICK_L;
            //             break;
            //         }else {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_SEPTUPLE_CLICK_R; 
            //             break;
            //         }
            //     }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
            //         if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            //             (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_SEPTUPLE_CLICK_L;
            //             break;
            //         }else {
            //             app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_SEPTUPLE_CLICK_R;
            //             break;
            //         }
            //     } 
            //     break;

            case KEY_ACTION_LONG:
#if _TONE_KEY_ACTION_LONG_ENABLE
                play_tone_file(get_tone_files()->_TONE_KEY_ACTION_LONG_NAME);
#endif
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_LONG_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_LONG_R; 
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_LONG_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_LONG_R;
                        break;
                    }
                } 
                break;

            case KEY_ACTION_HOLD:
                //const char *fname = get_tone_files()->normal;
                //tws_play_tone_file_alone(fname,0);
                //留在这里做个参考，代提示音的按键处理
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_HOLD_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_CONNECTED_KEY_ACTION_HOLD_R; 
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_HOLD_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_CONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_HOLD_R;
                        break;
                    }
                } 
                break;

            default:
                break;
            }
        } else if (tws_state & TWS_STA_PHONE_DISCONNECTED) {//手机未连接蓝牙
            switch (key_action) {
            case KEY_ACTION_CLICK:
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_CLICK_R;
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_CLICK_R;
                        break;
                    }
                }
                break;

            case KEY_ACTION_DOUBLE_CLICK:
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_DOUBLE_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_DOUBLE_CLICK_R;
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_DOUBLE_CLICK_L;
                        app_msg = KEY_HEART_ONCE;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_DOUBLE_CLICK_R;
                        app_msg = KEY_HEART_ONCE;
                        break;
                    }
                }
                break;

            case KEY_ACTION_TRIPLE_CLICK:
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_TRIPLE_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_TRIPLE_CLICK_R; 
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_TRIPLE_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_TRIPLE_CLICK_R;
                        break;
                    }
                } 
                break; 
            
            case KEY_ACTION_FOURTH_CLICK:
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_FOURTH_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_FOURTH_CLICK_R; 
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_FOURTH_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_FOURTH_CLICK_R;
                        break;
                    }
                } 
                break;

            // case KEY_ACTION_FIFTH_CLICK:
            //     if (tws_state & TWS_STA_SIBLING_CONNECTED) {
            //         if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            //             (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            //             //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_FIFTH_CLICK_L;
            //             break;
            //         }else {
            //             //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_FIFTH_CLICK_R; 
            //             break;
            //         }
            //     }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
            //         if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            //             (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            //             //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_FIFTH_CLICK_L;
            //             break;
            //         }else {
            //             //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_FIFTH_CLICK_R;
            //             break;
            //         }
            //     } 
            //     break;
            
            case KEY_ACTION_SEXTUPLE_CLICK:
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_SEXTUPLE_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_SEXTUPLE_CLICK_R; 
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_SEXTUPLE_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_SEXTUPLE_CLICK_R;
                        break;
                    }
                } 
                break;
            
            case KEY_ACTION_SEPTUPLE_CLICK:
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_SEPTUPLE_CLICK_L;
                        break;
                    }else {
                        // = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_SEPTUPLE_CLICK_R; 
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_SEPTUPLE_CLICK_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_SEPTUPLE_CLICK_R;
                        break;
                    }
                } 
                break;

            case KEY_ACTION_LONG:
                //const char *fname = get_tone_files()->normal;
                //tws_play_tone_file_alone(fname,0);
                //留在这里做个参考，代提示音的按键处理
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_LONG_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_LONG_R;
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_LONG_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_LONG_R;
                        break;
                    }
                } 
                break;

            case KEY_ACTION_HOLD:
                //const char *fname = get_tone_files()->normal;
                //tws_play_tone_file_alone(fname,0);
                //留在这里做个参考，代提示音的按键处理
                if (tws_state & TWS_STA_SIBLING_CONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_HOLD_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_CONNECTED_KEY_ACTION_HOLD_R; 
                        break;
                    }
                }else if(tws_state & TWS_STA_SIBLING_DISCONNECTED) {
                    if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
                        (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_HOLD_L;
                        break;
                    }else {
                        //app_msg = _TWS_STA_PHONE_DISCONNECTED_SIBLING_DISCONNECTED_KEY_ACTION_HOLD_R;
                        break;
                    }
                } 
                break;
                
            default:
                break;
            }
        }
#if (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)
        if (le_auracast_status_get() == BROADCAST_STATUS_START) {
            switch (key_action) {
            case KEY_ACTION_DOUBLE_CLICK:
                app_msg = APP_MSG_VOL_UP;
                break;
            case KEY_ACTION_TRIPLE_CLICK:
                app_msg = APP_MSG_VOL_DOWN;
                break;
            default:
                break;
            }
        }
#endif
    }
    /* 所有场景下按键流程 */
    switch (key_action) {
//     case KEY_ACTION_CLICK_PLUS_LONG:
//         // 单击+长按
//         app_msg = _KEY_ACTION_CLICK_PLUS_LONG;
//         break;

//     case KEY_ACTION_DOUBLE_CLICK_PLUS_LONG:
//         // 双击+长按
//         app_msg = _KEY_ACTION_DOUBLE_CLICK_PLUS_LONG;
//         break;

//     case KEY_ACTION_TRIPLE_CLICK_PLUS_LONG:
//         // 三击+长按
// #if _TONE_KEY_ACTION_TRIPLE_CLICK_PLUS_LONG_ENABLE
//         play_tone_file(get_tone_files()->_TONE_KEY_ACTION_TRIPLE_CLICK_PLUS_LONG_NAME);
// #endif
//         app_msg = _KEY_ACTION_TRIPLE_CLICK_PLUS_LONG;
//         break;

    case KEY_ACTION_HOLD_3SEC: 
        //长按5s
        app_msg = APP_MSG_TWS_POWER_OFF; 
        break; 
    case KEY_ACTION_HOLD_1SEC: 
        //长按3s
        //app_msg = _KEY_ACTION_HOLD_1SEC; 
        break;
    case KEY_ACTION_TWS_FIRTH_CLICK:
        //TWS同时五击(工具中需要使能)
#if _TONE_KEY_ACTION_TWS_FIRTH_CLICK_ENABLE
        play_tone_file(get_tone_files()->_TONE_KEY_ACTION_TWS_FIRTH_CLICK_NAME);
#endif
        //app_msg = _KEY_ACTION_TWS_FIRTH_CLICK;
        break;

#if (TCFG_USER_TWS_ENABLE && (CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_CLICK))
    case KEY_ACTION_FOURTH_CLICK:
        if (tws_state & TWS_STA_TWS_UNPAIRED) { // TWS未配对，按键配对
            app_msg = APP_MSG_TWS_START_PAIR;
            break;
        }
#endif
    default:
        break;
    }
#if 0 //anc功能测试
    app_msg = APP_MSG_NULL;
    switch (key_action) {
    case KEY_ACTION_CLICK:
        y_printf("APP_MSG_MUSIC_PP 95 >>>>>>>>>>>>>>KEY_ACTION_CLICK \n");
        app_msg = APP_MSG_MUSIC_PP;
        break;
    case KEY_ACTION_HOLD_1SEC:   //长按1s 切换anc模式
        y_printf("APP_MSG_ANC_SWITCH 66 >>>>>>>>>>>>>>KEY_ACTION_HOLD_1SEC \n");
        app_msg = APP_MSG_ANC_SWITCH;
        break;
    case KEY_ACTION_DOUBLE_CLICK:    //双击切换智能免摘
        y_printf("audio_speak_to_chat_demo 72 >>>>>>>>>>>>>>KEY_ACTION_DOUBLE_CLICK \n");
        app_msg = APP_MSG_SPEAK_TO_CHAT_SWITCH;
        break;
    case KEY_ACTION_TRIPLE_CLICK:   //三击切换广域点击
        y_printf("audio_wat_click_demo 73  >>>>>>>>>>>>>>KEY_ACTION_TRIPLE_CLICK \n");
        app_msg = APP_MSG_WAT_CLICK_SWITCH;
        break;
    case KEY_ACTION_FOURTH_CLICK:   //四击切换风噪检测
        y_printf("audio_icsd_wind_detect_demo 74 >>>>>>>>>>>>>>KEY_ACTION_FOURTH_CLICK \n");
        app_msg = APP_MSG_WIND_DETECT_SWITCH;
        break;
    case KEY_ACTION_FIFTH_CLICK:   //五击切换自适应 自适应开启一段时间会自己关
        y_printf("audio_anc_ear_adaptive_open 70  >>>>>>>>>>>>>>KEY_ACTION_FIFTH_CLICK \n");
        app_msg = APP_MSG_EAR_ADAPTIVE_OPEN;
        break;
    default:
        break;
    }
#endif
    printf("bt_key_msg_remap, key_action: %d, app_msg: %d\n", key_action, app_msg);
    return app_msg;
}

int bt_key_next_msg_remap(int *msg)
{
    int app_msg = APP_MSG_NULL;
    u8 key_action  = APP_MSG_KEY_ACTION(msg[0]);

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return APP_MSG_NULL;
    }
    if (tws_api_get_tws_state() & TWS_STA_PHONE_DISCONNECTED) {
        return APP_MSG_NULL;
    }

    int state = bt_get_call_status();

    switch (key_action) {
    case KEY_ACTION_CLICK:
        if (state == BT_CALL_HANGUP) {
            app_msg = APP_MSG_MUSIC_NEXT;
        }
        break;
    case KEY_ACTION_LONG:
    case KEY_ACTION_HOLD:
        app_msg = APP_MSG_VOL_UP;
        break;
    }

    return app_msg;
}

int bt_key_prev_msg_remap(int *msg)
{
    int app_msg = APP_MSG_NULL;
    u8 key_action  = APP_MSG_KEY_ACTION(msg[0]);

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return APP_MSG_NULL;
    }
    if (tws_api_get_tws_state() & TWS_STA_PHONE_DISCONNECTED) {
        return APP_MSG_NULL;
    }

    int state = bt_get_call_status();

    switch (key_action) {
    case KEY_ACTION_CLICK:
        if (state == BT_CALL_HANGUP) {
            app_msg = APP_MSG_MUSIC_PREV;
        }
        break;
    case KEY_ACTION_LONG:
    case KEY_ACTION_HOLD:
        app_msg = APP_MSG_VOL_DOWN;
        break;
    }

    return app_msg;
}

int bt_key_mode_msg_remap(int *msg)
{
    int app_msg = APP_MSG_NULL;
    u8 key_action  = APP_MSG_KEY_ACTION(msg[0]);

    switch (key_action) {
    case KEY_ACTION_CLICK:
        app_msg = APP_MSG_GOTO_NEXT_MODE;
        break;
    }

    return app_msg;
}

int bt_key_slider_msg_remap(int *msg)
{
    int app_msg = APP_MSG_NULL;
    u8 key_action  = APP_MSG_KEY_ACTION(msg[0]);

    switch (key_action) {
    case KEY_SLIDER_UP:
        app_msg = APP_MSG_VOL_UP;
        break;
    case KEY_SLIDER_DOWN:
        app_msg = APP_MSG_VOL_DOWN;
        break;
    }
    return app_msg;
}




const struct key_remap_table bt_mode_key_table[] = {
#if TCFG_IOKEY_ENABLE
    { .key_value = KEY_POWER,   .remap_func = bt_key_power_msg_remap },
    //{ .key_value = KEY_NEXT,    .remap_func = bt_key_next_msg_remap },
    //{ .key_value = KEY_PREV,    .remap_func = bt_key_prev_msg_remap },
#endif
#if TCFG_LP_TOUCH_KEY_ENABLE
    { .key_value = KEY_POWER,   .remap_func = bt_key_power_msg_remap },
    { .key_value = KEY_SLIDER,  .remap_func = bt_key_slider_msg_remap },
#endif

#if TCFG_ADKEY_ENABLE
    { .key_value = KEY_AD_NUM0, .remap_table = adkey_msg_table[0] },
    { .key_value = KEY_AD_NUM1, .remap_table = adkey_msg_table[1] },
    { .key_value = KEY_AD_NUM2, .remap_table = adkey_msg_table[2] },
    { .key_value = KEY_AD_NUM3, .remap_table = adkey_msg_table[3] },
    { .key_value = KEY_AD_NUM4, .remap_table = adkey_msg_table[4] },
    { .key_value = KEY_AD_NUM5, .remap_table = adkey_msg_table[5] },
    { .key_value = KEY_AD_NUM6, .remap_table = adkey_msg_table[6] },
    { .key_value = KEY_AD_NUM7, .remap_table = adkey_msg_table[7] },
    { .key_value = KEY_AD_NUM8, .remap_table = adkey_msg_table[8] },
    { .key_value = KEY_AD_NUM9, .remap_table = adkey_msg_table[9] },
#endif

    { .key_value = 0xff }
};

