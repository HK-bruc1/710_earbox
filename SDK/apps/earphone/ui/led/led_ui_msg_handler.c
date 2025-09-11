#include "app_msg.h"
#include "app_main.h"
#include "bt_tws.h"
#include "asm/charge.h"
#include "battery_manager.h"
#include "pwm_led/led_ui_api.h"
#include "a2dp_player.h"
#include "esco_player.h"
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
#include "cig.h"
#include "app_le_connected.h"
#endif


#define LOG_TAG             "[LED_UI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if (TCFG_PWMLED_ENABLE)

u8 app_msg_power_on_flag = 0;//开机灯效标志
static int ui_battery_msg_handler(int *msg)
{
    switch (msg[0]) {
    case BAT_MSG_CHARGE_START:
        led_ui_set_state(LED_STA_RED_ON, DISP_CLEAR_OTHERS);
        break;
    case BAT_MSG_CHARGE_FULL:
    case BAT_MSG_CHARGE_CLOSE:
    case BAT_MSG_CHARGE_ERR:
    case BAT_MSG_CHARGE_LDO5V_OFF:
        led_ui_set_state(LED_STA_ALL_OFF, DISP_CLEAR_OTHERS);
        break;
    case POWER_EVENT_POWER_WARNING:
    case POWER_EVENT_POWER_LOW:
        //低电灯效
        led_ui_set_state(LED_STA_RED_SLOW_FLASH, DISP_NON_INTR | DISP_TWS_SYNC);
        break;
    }
    return 0;
}

static void led_enter_mode(u8 mode)
{
    switch (mode) {
    case APP_MODE_POWERON:
        break;
    case APP_MODE_BT:
        //这是不设置的话，出仓开机灯效会丢失
        //暂时不清楚会不会覆盖其他灯效
        //公版没有这个
        led_ui_set_state(LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, DISP_CLEAR_OTHERS);
        break;
    case APP_MODE_PC:
    case APP_MODE_LINEIN:
        //led_ui_set_state(LED_STA_ALL_OFF, DISP_TWS_SYNC);
        break;

    }
}

u8 app_msg_low_lantecy=0;//低延迟标志

static int ui_app_msg_handler(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_ENTER_MODE:
        //这里不设置灯效。
        led_enter_mode(msg[1] & 0xff);
        //都不设置灯效，避免打断开机灯效
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_ENTER_MODE");
        break;
    case APP_MSG_POWER_ON:
        // 开机状态不可打断
        led_ui_set_state(LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, DISP_CLEAR_OTHERS);
        app_msg_power_on_flag = 1;
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_POWER_ON");
        break;
    case APP_MSG_POWER_OFF:
        // 超时自动关机
        if (msg[1] == POWEROFF_NORMAL ||
            msg[1] == POWEROFF_NORMAL_TWS) {
            led_ui_set_state(LED_STA_RED_FLASH_3TIMES, DISP_CLEAR_OTHERS | DISP_TWS_SYNC);
        }
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_POWER_OFF");
        break;
    case APP_MSG_TWS_PAIRED:
    case APP_MSG_TWS_UNPAIRED:
        //这里也不设置，避免打断其他灯效
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_TWS_UNPAIRED");
        break;
    case APP_MSG_BT_IN_PAGE_MODE:
        // 超距回连状态
        if(app_msg_power_on_flag == 1){
            //耳机有配对记录时，开机会走这里，这里不设置灯效
            log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAGE_MODE----app_msg_power_on_flag");
            //当真的超距回连状态来时app_msg_power_on_flag应该为0
            //什么时候置0？当连接手机蓝牙后，这个置0，那么超距断开时回连就会走到下面。
        } else {
            led_ui_set_state(LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, DISP_CLEAR_OTHERS);
            log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAGE_MODE");
        }        
        break;
    case APP_MSG_BT_IN_PAIRING_MODE:
        // 等待手机连接状态----这里继承蓝牙断开灯效，不然会打乱超距回连灯效。
        log_info("MSG_FROM_APP----ui_app_msg_handler----APP_MSG_BT_IN_PAIRING_MODE");
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
        if (!bt_get_total_connect_dev() && !is_cig_phone_conn() && !is_cig_other_phone_conn()) {
#else
        if (bt_get_total_connect_dev() == 0) {
#endif
            //led_ui_set_state(LED_STA_BLUE_FLASH_1TIMES_PER_1S, DISP_TWS_SYNC);
        }
        break;
    case APP_MSG_LOW_LANTECY:
        if(app_msg_low_lantecy == 0){
            app_msg_low_lantecy = 1;//第一次进入低延迟模式
            led_ui_set_state(LED_STA_NONE, DISP_CLEAR_OTHERS | DISP_TWS_SYNC);
        } else {
            //已经是低延迟模式了，进来切换为音乐模式
            //这里设置音乐模式灯效
            led_ui_set_state(LED_STA_NONE, DISP_CLEAR_OTHERS | DISP_TWS_SYNC);
            app_msg_low_lantecy = 0;
        }
        break;
    }
    return 0;
}



static int ui_bt_stack_msg_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;
    char channel = tws_api_get_local_channel();
    int tws_state = tws_api_get_tws_state();
//这里不注释，下面主从灯效不生效    
// #if TCFG_USER_TWS_ENABLE
//     int tws_role = tws_api_get_role_async();
//     if (tws_role == TWS_ROLE_SLAVE) {
//         return 0;
//     }
// #endif

    printf("ui_bt_stack_msg_handler:%d\n", bt->event);
    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        //不设置灯效，会覆盖开机灯效
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_INIT_OK");
        break;
    case BT_STATUS_FIRST_CONNECTED:
    case BT_STATUS_SECOND_CONNECTED:
        //蓝牙连接成功灯效
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_FIRST_CONNECTED");
        app_msg_power_on_flag = 0; //连接成功后置0
#if 0//_LED_BT_STATUS_CONNECTED_LEFT_RIGHT
        //灯效分左右
        if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_LEFT_NAME, _LED_BT_STATUS_CONNECTED_LEFT_DISP_MODE);  
        }else {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_RIGHT_NAME, _LED_BT_STATUS_CONNECTED_RIGHT_DISP_MODE);    
        }
#elif 0//_LED_BT_STATUS_CONNECTED_MASTER_SLAVE
        //灯效分主从
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_MASTER_NAME, _LED_BT_STATUS_CONNECTED_MASTER_DISP_MODE);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
            led_ui_set_state(_LED_BT_STATUS_CONNECTED_SLAVE_NAME, _LED_BT_STATUS_CONNECTED_SLAVE_DISP_MODE);
        }
#else
        //默认双耳同步灯效
        led_ui_set_state(LED_STA_ALL_OFF, DISP_CLEAR_OTHERS | DISP_TWS_SYNC);
#endif
        break;
    case BT_STATUS_A2DP_MEDIA_START:
    case BT_STATUS_PHONE_ACTIVE:
    case BT_STATUS_PHONE_HANGUP:
        //音乐与电话类一般无灯效，先不设置
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_PHONE_HANGUP----BT_STATUS_A2DP_MEDIA_START----BT_STATUS_PHONE_ACTIVE");
        break;
    case BT_STATUS_PHONE_INCOME:
        // 来电一般无灯效，先不设置
        if (!esco_player_runing()) {
            //led_ui_set_state(_LED_BT_STATUS_PHONE_INCOME_NAME, _LED_BT_STATUS_PHONE_INCOME_DISP_MODE);
        }
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_PHONE_INCOME-------------------------------------------------------");
        break;
    case BT_STATUS_SCO_CONNECTION_REQ:
        u8 call_status = bt_get_call_status_for_addr(bt->args);
        if (call_status == BT_CALL_INCOMING) {
            //来电，这个不修改，来电灯效同BT_STATUS_PHONE_INCOME，
            //这个没必要设置
            if (!esco_player_runing()) {
                //led_ui_set_state(LED_STA_BLUE_FAST_FLASH, DISP_TWS_SYNC);
                //led_ui_set_state(LED_STA_ALL_OFF, DISP_NON_INTR | DISP_TWS_SYNC);
                log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SCO_CONNECTION_REQ----------BT_CALL_INCOMING---------");
            }
        } else if (call_status == BT_CALL_OUTGOING) {
            log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SCO_CONNECTION_REQ----------BT_CALL_OUTGOING---------");
        }
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        //停止音乐一般无灯效，先不设置
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_A2DP_MEDIA_STOP");
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        /*
         * 关机不执行断开灯效
         */
        if (app_var.goto_poweroff_flag) {
            //在关机流程中，这种变量会在灯效前被置1
            break;
        }
        //蓝牙断开灯效
#if 0//_LED_BT_STATUS_DISCONNECT_LEFT_RIGHT
        //灯效分左右
    #if _LED_BT_STATUS_DISCONNECT_TWS_STA_SIBLING_DISCONNECTED_ENABLE
        //单耳时是否指定灯效
        if(tws_state & TWS_STA_SIBLING_DISCONNECTED){
            //单耳状态闪烁谁的灯效
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_TWS_STA_SIBLING_DISCONNECTED_NAME, _LED_BT_STATUS_DISCONNECT_TWS_STA_SIBLING_DISCONNECTED_DISP_MODE);   
            //单耳如果指定了就不走下面了。
            break;
        }
    #endif
        if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_LEFT_NAME, _LED_BT_STATUS_DISCONNECT_LEFT_DISP_MODE);   
        }else {
            led_ui_set_state(_LED_BT_STATUS_DISCONNECT_RIGHT_NAME, _LED_BT_STATUS_DISCONNECT_RIGHT_DISP_MODE);    
        }
#elif 1//_LED_BT_STATUS_DISCONNECT_MASTER_SLAVE
        //灯效分主从
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, DISP_CLEAR_OTHERS);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE){
            led_ui_set_state(LED_STA_RED_FLASH_1TIMES_PER_5S, DISP_CLEAR_OTHERS);
        }
#else
        //默认双耳同步灯效
        led_ui_set_state(_LED_BT_STATUS_DISCONNECT_NAME, _LED_BT_STATUS_DISCONNECT_DISP_MODE);
#endif
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_FIRST_DISCONNECT");
        break;
    case BT_STATUS_SNIFF_STATE_UPDATE:
        //这个在蓝牙连接后周期性进入
        log_info("MSG_FROM_BT_STACK----ui_bt_stack_msg_handler----BT_STATUS_SNIFF_STATE_UPDATE");
        break;
    }
    return 0;
}

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
/**
 * @brief CIG连接状态事件处理函数
 *
 * @param msg:状态事件附带的返回参数
 *
 * @return
 */
static int led_ui_app_connected_conn_status_event_handler(int *msg)
{
    int *event = msg;
    /* g_printf("led_ui_app_connected_conn_status_event_handler=%d", event[0]); */

    switch (event[0]) {
    case CIG_EVENT_PERIP_CONNECT:
        /* log_info("CIG_EVENT_PERIP_CONNECT\n"); */
        /* led_ui_set_state(LED_STA_ALL_OFF, 0); */
        break;
    case CIG_EVENT_PERIP_DISCONNECT:
        break;
    case CIG_EVENT_ACL_CONNECT:
        break;
    case CIG_EVENT_ACL_DISCONNECT:
        break;
    case CIG_EVENT_JL_DONGLE_CONNECT:
    case CIG_EVENT_PHONE_CONNECT:
        log_info("CIG_EVENT_PHONE_CONNECT\n");
        led_ui_set_state(LED_STA_BLUE_ON_1S, DISP_NON_INTR);
        break;
    case CIG_EVENT_JL_DONGLE_DISCONNECT:
    case CIG_EVENT_PHONE_DISCONNECT:
        break;
    default:
        break;
    }

    return 0;
}
APP_MSG_PROB_HANDLER(app_le_connected_led_ui_msg_entry) = {
    .owner = 0xff,
    .from = MSG_FROM_CIG,
    .handler = led_ui_app_connected_conn_status_event_handler,
};
#endif

void ui_pwm_led_msg_handler(int *msg)
{
    struct led_state_obj *obj;

    switch (msg[0]) {
    case LED_MSG_STATE_END:
        // 执行灯效结束消息, 可通过分配唯一的name来区分是哪个灯效
        obj = led_ui_get_state(msg[1]);
        if (obj) {
            log_info("LED_STATE_END: name = %d\n", obj->name);
            if (obj->name == LED_STA_POWERON) {

            }
        }
#if TCFG_USER_TWS_ENABLE
        led_tws_state_sync_stop();
#endif
        led_ui_state_machine_run();
        break;
    }

}

#if TCFG_USER_TWS_ENABLE
static int ui_tws_msg_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 state = evt->args[1];
    u8 *bt_addr = evt->args + 3;

    char channel = tws_api_get_local_channel();
    int tws_state = tws_api_get_tws_state();

    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
        //TWS连接灯效
        log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTED------------------------------");
        // if (role == TWS_ROLE_SLAVE) {
        //     led_ui_set_state(LED_STA_ALL_OFF, 0);
        // } else {
        //     struct led_state_obj *obj;
        //     obj = led_ui_get_first_state();
        //     if (obj && obj->disp_mode & DISP_TWS_SYNC) {
        //         led_ui_state_machine_run();
        //     }
        // }
#if 0//_LED_TWS_EVENT_CONNECTED_LEFT_RIGHT
        if ((channel == 'L' && msg[1] != APP_KEY_MSG_FROM_TWS) ||
            (channel == 'R' && msg[1] == APP_KEY_MSG_FROM_TWS)) {
            led_ui_set_state(_LED_TWS_EVENT_CONNECTED_LEFT_NAME, _LED_TWS_EVENT_CONNECTED_LEFT_DISP_MODE);    
        }else {
            led_ui_set_state(_LED_TWS_EVENT_CONNECTED_RIGHT_NAME, _LED_TWS_EVENT_CONNECTED_RIGHT_DISP_MODE);
        }
#elif 1//_LED_TWS_EVENT_CONNECTED_MASTER_SLAVE
        //灯效分主从
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            led_ui_set_state(LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, DISP_CLEAR_OTHERS);
        }else if(tws_api_get_role() == TWS_ROLE_SLAVE) {
            led_ui_set_state(LED_STA_RED_FLASH_1TIMES_PER_5S, DISP_CLEAR_OTHERS);
        }
#else
        //默认双耳同步灯效
        led_ui_set_state(_LED_TWS_EVENT_CONNECTED_NAME, _LED_TWS_EVENT_CONNECTED_DISP_MODE);
#endif
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        /*
         * 关机不执行断开灯效
         */
        if (app_var.goto_poweroff_flag) {
            //在关机流程中，这种变量会在灯效前被置1
            break;
        }
        //TWS断开灯效
        log_info("MSG_FROM_TWS----ui_tws_msg_handler----TWS_EVENT_CONNECTION_DETACH------------------------------");
        //断开后都是主耳，设置一种灯效就可以，一般跟未连接手机灯效一致。
        led_ui_set_state(LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, DISP_CLEAR_OTHERS);
        led_tws_state_sync_stop();//注意位置的灯效变化
        break;
    }
    return 0;
}
#endif



static int ui_led_msg_handler(int *msg)
{
    switch (msg[0]) {
#if TCFG_USER_TWS_ENABLE
    case MSG_FROM_TWS:
        //TWS对耳的灯效消息
        ui_tws_msg_handler(msg + 1);
        break;
#endif
    case MSG_FROM_BT_STACK:
        //跟蓝牙状态有关的灯效消息
        ui_bt_stack_msg_handler(msg + 1);
        break;
    case MSG_FROM_APP:
        //跟应用状态有关的灯效消息
        ui_app_msg_handler(msg + 1);
        break;
    case MSG_FROM_BATTERY:
        //电池相关的灯效消息
        ui_battery_msg_handler(msg + 1);
        break;
    case MSG_FROM_PWM_LED:
        ui_pwm_led_msg_handler(msg + 1);
        break;
    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(led_msg_entry) = {
    .owner      = 0xff,
    .from       = 0xff,
    .handler    = ui_led_msg_handler,
};


#endif

