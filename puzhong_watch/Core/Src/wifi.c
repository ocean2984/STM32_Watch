#include "wifi.h"
#include "esp8266.h"
#include "rtc.h"
#include "usart.h"

uint8_t WiFi_Connect(void) {
    OLED_Clear();
    OLED_ShowString(0, 0, "Connecting...");
    
    ESP_SendCmd("AT+CWMODE=1", "OK", 1000);

    if(ESP_SendCmd("AT+CWJAP=\"self\",\"11223345\"", "OK", 15000)) {
        OLED_ShowString(0, 2, "WiFi: OK!");
        return 1;
    } else {
        OLED_ShowString(0, 2, "WiFi: Timeout!");
        return 0;
    }
}

void Sync_Time_From_NowAPI(void) {
    // 1. 建立 TCP 连接
    if (ESP_SendCmd("AT+CIPSTART=\"TCP\",\"api.k780.com\",80", "OK", 5000)) {
        
        // 这里的请求字符串一定不能错，尤其是末尾要有两个 \r\n
        char *get_req = "GET /?app=life.time&appkey=&sign=4001a049f617c93340eb4af27ac5a153&format=json HTTP/1.1\r\nHost: api.k780.com\r\nConnection: close\r\n\r\n";
        
        char send_cmd[32];
        sprintf(send_cmd, "AT+CIPSEND=%d", (int)strlen(get_req));
        
        // 2. 发送长度，等待 ">" 
        if (ESP_SendCmd(send_cmd, ">", 2000)) {
            // 关键点：看到 > 之后，立刻清空缓冲区，准备接收接下来的 HTTP 报文
            ESP_ClearBuf(); 
            
            // 发送真正的 GET 请求
            HAL_UART_Transmit(&huart3, (uint8_t*)get_req, strlen(get_req), 1000);
            
            // 3. 关键修改：NowAPI 返回数据比较慢，且数据量大，给它 3 秒时间
            OLED_ShowString(0, 4, "Waiting Time...");
            HAL_Delay(3000); 

            // 调试：把收到的 JSON 内容发给电脑，看看到底回了什么
            HAL_UART_Transmit(&huart1, (uint8_t*)"Recv Data: ", 11, 100);
            HAL_UART_Transmit(&huart1, (uint8_t*)esp_buf, strlen(esp_buf), 100);

            // 4. 在 esp_buf 中查找关键字
            char *p = strstr(esp_buf, "datetime_1\":\""); 
            if (p) {
                p += 13; // 移动到 "2024..." 开始的地方
                int year, month, day, hour, min, sec;
                // 注意 sscanf 匹配格式
                 if (sscanf(p, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
									RTC_TimeTypeDef sTime = {0};
									RTC_DateTypeDef sDate = {0};

									sTime.Hours = (uint8_t)hour;
									sTime.Minutes = (uint8_t)min;
									sTime.Seconds = (uint8_t)sec;
									HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

									sDate.Year = (uint8_t)(year % 100);
									sDate.Month = (uint8_t)month;
									sDate.Date = (uint8_t)day;
									sDate.WeekDay = RTC_WEEKDAY_MONDAY; 
									HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        
									OLED_ShowString(0, 6, "Time Sync OK!   ");
                } else {
                    OLED_ShowString(0, 6, "Parse Error     ");
                }
            } else {
                OLED_ShowString(0, 6, "No Time Data    ");
            }
        }
    }
}
// 3. 上传到 ThingsCloud
void ThingsCloud_Upload(int hr, int spo2, float temp, int steps) {
    // 配置MQTT  
    ESP_SendCmd("AT+MQTTUSERCFG=0,1,\"Client_ID\",\"User\",\"Pass\",0,0,\"\"", "OK", 1000);
    // 连接
    if (ESP_SendCmd("AT+MQTTCONN=0,\"mqtt.thingscloud.cn\",1883,1", "OK", 3000)) {
        char payload[128];
        char pub[200];
        // 构建 ThingsCloud 需要的 JSON 格式
        sprintf(payload, "{\\\"hr\\\":%d,\\\"spo2\\\":%d,\\\"temp\\\":%.1f,\\\"steps\\\":%d}", hr, spo2, temp, steps);
        sprintf(pub, "AT+MQTTPUB=0,\"attributes/push\",\"%s\",0,0", payload);
        ESP_SendCmd(pub, "OK", 2000);
    }
}

/*void ThingsCloud_Upload(int hr, int spo2, float temp, int steps) {
    // 1. 配置参数 (LinkID=0, Scheme=1代表TCP, ClientID, User, Pass)
    
    char *AccessToken = "你的AccessToken"; 
    char auth_cmd[128];
    sprintf(auth_cmd, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"你的ProjectKey\",0,0,\"\"", AccessToken, AccessToken);
    ESP_SendCmd(auth_cmd, "OK", 1000);

    // 2. 连接服务器
    if (ESP_SendCmd("AT+MQTTCONN=0,\"mqtt.thingscloud.cn\",1883,1", "OK", 3000)) {
        char payload[128];
        char pub[200];
        // ThingsCloud 接收属性上报的格式：{"method":"thing.property.post","params":{"标识符":值}}
        // 注意：标识符（hr, spo2等）必须和你在 ThingsCloud 网页端定义的一模一样
        sprintf(payload, "{\\\"hr\\\":%d,\\\"spo2\\\":%d,\\\"temp\\\":%.1f,\\\"steps\\\":%d}", hr, spo2, temp, steps);
        
        // 发布到属性上报的主题
        sprintf(pub, "AT+MQTTPUB=0,\"attributes/push\",\"%s\",0,0", payload);
        ESP_SendCmd(pub, "OK", 2000);
    }
}
*/

// 这是一个简单的页面刷新函数，根据 main 里的 current_page 变量显示不同内容
void Refresh_OLED(uint8_t page) {
    // OLED_Clear(); // 如果刷新太快会闪烁，可以考虑局部刷新
    
    if (page == 0) { // 时间页面
        // 这里显示 RTC 获取到的时间
        OLED_ShowString(0, 0, "--- TIME ---");
        // OLED_ShowString(0, 2, time_buffer); 
    } 
    else if (page == 1) { // 健康数据页面
        OLED_ShowString(0, 0, "--- HEALTH ---");
        // OLED_ShowString(0, 2, "HR: "); OLED_ShowNum(30, 2, heart_rate, 3);
        // ... 其他显示内容
    }
    else if (page == 2) { // 运动步数页面
        OLED_ShowString(0, 0, "--- SPORT ---");
        // OLED_ShowString(0, 2, "Step: "); OLED_ShowNum(40, 2, step, 5);
    }
}
