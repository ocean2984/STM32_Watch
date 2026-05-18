#include "wifi.h"
#include "esp8266.h"
#include "rtc.h"
#include "usart.h"
#include <stdio.h>

// 1. WiFi 连接 (保持原框架)
uint8_t WiFi_Connect(void) {
    OLED_Clear();
    OLED_ShowString(0, 0, "Connecting WiFi");
    
    ESP_SendCmd("AT+CWMODE=1", "OK", 1000);
    // 连接热点
    if(ESP_SendCmd("AT+CWJAP=\"self\",\"11223345\"", "OK", 10000)) {
        OLED_ShowString(0, 2, "WiFi: OK!       ");
        return 1;
    }
    return 0;
}

// 2. ThingsCloud 初始化 (新增加)
uint8_t ThingsCloud_Init(void) {
    OLED_ShowString(0, 4, "Cloud Connect...");
    
    // 配置MQTT：客户端ID、用户名、密码。ThingsCloud通常三者都填 AccessToken 或根据后台给的填
    // 假设你的 AccessToken 是 rNtEJmqwiV，设备ID是 av3xw7ed
    // 指令格式: AT+MQTTUSERCFG=0,1,"ClientID","User","Pass",0,0,""
    ESP_SendCmd("AT+MQTTUSERCFG=0,1,\"av3xw7ed\",\"av3xw7ed\",\"rNtEJmqwiV\",0,0,\"\"", "OK", 1000);
    
    // 连接服务器
    if (ESP_SendCmd("AT+MQTTCONN=0,\"mqtt.thingscloud.cn\",1883,1", "OK", 5000)) {
        // 订阅属性推送主题，用于接收小程序开关控制
        ESP_SendCmd("AT+MQTTSUB=0,\"attributes/push\",1", "OK", 1000);
        OLED_ShowString(0, 6, "Cloud: OK!      ");
        return 1;
    }
    OLED_ShowString(0, 6, "Cloud: Fail     ");
    return 0;
}

// 3. 上传数据 (修改后的版本)
void ThingsCloud_Upload(int hr, int spo2, float temp, int steps) {
    char payload[128];
    char pub[256];
    
    // 构建 ThingsCloud 要求的 JSON。注意：新版固件在 AT 指令里双引号需要转义
    // 最终发出的样子应该是: AT+MQTTPUB=0,"attributes/push","{\"hr\":70,\"spo2\":98}",0,0
    // 在 C 语言里要写成 \\\"
    sprintf(payload, "{\\\"hr\\\":%d,\\\"spo2\\\":%d,\\\"temp\\\":%.1f,\\\"steps\\\":%d}", 
            hr, spo2, temp, steps);
    
    sprintf(pub, "AT+MQTTPUB=0,\"attributes/push\",\"%s\",0,0", payload);
    
    ESP_SendCmd(pub, "OK", 2000);
}

// 4. NowAPI 对时 (修复后的逻辑)
void Sync_Time_From_NowAPI(void) {
    if (ESP_SendCmd("AT+CIPSTART=\"TCP\",\"api.k780.com\",80", "OK", 5000)) {
        char *get_req = "GET /?app=life.time&appkey=78254&sign=4001a049f617c93340eb4af27ac5a153&format=json HTTP/1.1\r\nHost: api.k780.com\r\nConnection: close\r\n\r\n";
        char send_cmd[32];
        sprintf(send_cmd, "AT+CIPSEND=%d", (int)strlen(get_req));
        
        if (ESP_SendCmd(send_cmd, ">", 2000)) {
            ESP_ClearBuf(); 
            // 关键：这里直接用原始发送，不加 AT 指令后缀
            HAL_UART_Transmit(&huart3, (uint8_t*)get_req, strlen(get_req), 1000);
            
            HAL_Delay(2000); // 等待返回内容到 esp_buf

            char *p = strstr(esp_buf, "datetime_1\":\""); 
            if (p) {
                p += 13;
                int year, month, day, hour, min, sec;
                if (sscanf(p, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
                    RTC_TimeTypeDef sTime = {(uint8_t)hour, (uint8_t)min, (uint8_t)sec};
                    RTC_DateTypeDef sDate = {RTC_WEEKDAY_MONDAY, (uint8_t)month, (uint8_t)day, (uint8_t)(year % 100)};
                    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
                    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
                }
            }
        }
        ESP_SendCmd("AT+CIPCLOSE", "OK", 1000);
    }
}
