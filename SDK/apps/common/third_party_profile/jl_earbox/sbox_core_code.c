#include "sbox_core_config.h"
#include "sbox_user_app.h"
#include "aes.h"
#include "app_config.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & JL_SBOX_EN)
#if (SBOX_CORE_DEBUG==1)
#define log_info          printf
#define log_info_hexdump  put_buf
#else
#define log_info(...) 
#define log_info_hexdump(...)
#endif

/*----------------协议格式--------------------*/
#define CUSTOM_BLE_PROTOCOL_HEADER1     0x5a // 协议头1
#define CUSTOM_BLE_PROTOCOL_HEADER2     0xa5 // 协议头2

#define CUSTOM_BLE_PROTOCOL_HEADER      0 // 2Byte
#define CUSTOM_BLE_PROTOCOL_LENGTH      2 // 1Byte
#define CUSTOM_BLE_PROTOCOL_CMD         3 // 1Byte
#define CUSTOM_BLE_PROTOCOL_VALUE       4 // nByte
/*----------------协议格式--------------------*/

struct box_info myboxinfo;

static u8 user_key[16]={1,2,3,4,5,6,7,8,9,0,'a','d','c','u','z','i'};
static u8 send_buf[SEND_BUF_MAX_LEN];  //发送BUF最大256，可以修改必须为16的倍数
static u8 sbox_aes_data[SEND_BUF_MAX_LEN];

bool sbox_ble_aes128_ecb_decrypt(uint8_t *key, uint8_t *input, uint16_t input_len, uint8_t *output)
{
    uint16_t length;
    mbedtls_aes_context aes_ctx;
    //
    if (input_len % 16) {
        return false;
    }

    length = input_len;

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);

    while (length > 0) {
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, input, output);
        input  += 16;
        output += 16;
        length -= 16;
    }

    mbedtls_aes_free(&aes_ctx);
    return true;
}
bool sbox_ble_aes128_ecb_encrypt(uint8_t *key, uint8_t *input, uint16_t input_len, uint8_t *output)
{
    uint16_t length;
    mbedtls_aes_context aes_ctx;
    //
    if (input_len % 16) {
        return false;
    }

    length = input_len;

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);

    while (length > 0) {
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, input, output);
        input  += 16;
        output += 16;
        length -= 16;
    }

    mbedtls_aes_free(&aes_ctx);

    return true;
}

void custom_app_receive_ble_data(u8 *data, u16 len, u8 *output);

int att_write_without_rsp_handler(uint8_t *buffer, uint16_t buffer_size)
{
    u8 buffers[SEND_BUF_MAX_LEN];
    log_info("att_write_without_rsp_handler %d\n",buffer_size);
    // put_buf(buffer,buffer_size);
    if((buffer_size %16) == 0){
        custom_app_receive_ble_data(buffer,buffer_size,buffers);
        myboxinfo.cmd = buffers[CUSTOM_BLE_PROTOCOL_CMD];
        memset(myboxinfo.data,0,SEND_BUF_MAX_LEN);
        memcpy(myboxinfo.data,buffers+CUSTOM_BLE_PROTOCOL_VALUE,buffers[CUSTOM_BLE_PROTOCOL_LENGTH]-1);
        u32 ret = 0;
        myboxinfo.lens = buffers[CUSTOM_BLE_PROTOCOL_LENGTH]-1;
      
        if ((buffers[CUSTOM_BLE_PROTOCOL_HEADER] != CUSTOM_BLE_PROTOCOL_HEADER1)|| (buffers[CUSTOM_BLE_PROTOCOL_HEADER+1] != CUSTOM_BLE_PROTOCOL_HEADER2)) {
            line_inf
            return -1;
        }
        log_info("att_write_without_rsp_handler , cmd:0x%x, buffer:",myboxinfo.cmd);
        put_buf(myboxinfo.data, myboxinfo.lens);
        sys_smartstore_notify_event(&myboxinfo);
        return ret;
    }
    
    
}

void custom_app_receive_ble_data(u8 *data, u16 len, u8 *output)
{
    log_info("custom_app_receive_ble_data %d\n",len);
    u8 tmp_lens = len;
    u8 local_offset=0;
    while((tmp_lens != 0) && (local_offset != len)){
        // putchar('!');
        // sbox_ble_aes128_ecb_decrypt(user_key,data,len,output);
        sbox_ble_aes128_ecb_decrypt(user_key,data+local_offset,16,output + local_offset);
        local_offset +=16;
        tmp_lens-=16;
    }
    // put_buf(output,strlen(output));
}


void custom_app_send_ble_data(u8 *data, u16 len)
{
    printf("custom_app_send_ble_data %d\n",len);
    u8 tmp_lens = len;
    u8 local_offset=0;
    memset(sbox_aes_data,0,sizeof(sbox_aes_data));
    while((tmp_lens != 0) && (local_offset != len)){
        putchar('!');
        sbox_ble_aes128_ecb_encrypt(user_key,data+local_offset,16,sbox_aes_data + local_offset);
        local_offset +=16;
        tmp_lens-=16;
    }
    // put_buf(sbox_aes_data,len);
    if(sbox_func_hdl.sbox_ble_send_data){
        sbox_func_hdl.sbox_ble_send_data(sbox_aes_data,len);
    }else{
        log_info("sbox_func_hdl.sbox_ble_send_data no register\n");
    }
    
    
}


/*数据组包处理，并组够16的倍数的长度
    HEADER1 + HEADER1 + LEN + CMD + DATA + 补齐的随机数据
*/
void sbox_ble_att_send_data(u8 cmd, u8 *data, u16 len)
{                                          
    u8 offset = 0;
    u8 debug_offset =0;
    if(data == NULL || (len > (SEND_BUF_MAX_LEN-4))){
        log_info("DATA IS NULL OR LENS IS ERROR > MAX LEN");
        return ;
    }
    memset(send_buf, 0, sizeof(send_buf));
    send_buf[0] = CUSTOM_BLE_PROTOCOL_HEADER1; offset++;
    send_buf[1] = CUSTOM_BLE_PROTOCOL_HEADER2; offset++;
    send_buf[2] = len+1; offset++; //加上cmd的长度
    send_buf[3] = cmd; offset++;
    memcpy(send_buf+4, data, len); offset+=len;

    debug_offset = offset%16;        //data len加上4个处理字节为全包长度

    // log_info("sbox_ble_att_send_data  debug offset:%d<%d offset:%d, send_buf:", debug_offset, len,offset);
    if (debug_offset){
        for(int i =0;i<(16-debug_offset);i++){

            send_buf[offset+i] = rand32()%16; //补齐16个字节
        }
        offset+=(16-debug_offset);
    } 
    // else {
    //     custom_app_send_ble_data(service, send_buf, offset, handle_type);
    // }
    log_info("sbox_ble_att_send_data offset:%d" ,offset);
    // put_buf(send_buf,offset);
    custom_app_send_ble_data(send_buf, offset);
}
#endif