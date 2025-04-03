#ifndef TWS_DUAL_CONN_H
#define TWS_DUAL_CONN_H

void tws_dual_conn_state_handler();
void tws_sniff_controle_check_disable(void);
void send_page_device_addr_2_sibling();

/**
 * @brief 清除回连配对列表的设备信息
 */
void clr_device_in_page_list();

#endif
