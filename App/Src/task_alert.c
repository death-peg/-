/**
 * @file    task_alert.c
 * @brief   报警发送任务 — Air780E 4G Cat.1 MQTT 全国远程报警
 *
 * 链路:  报警事件 → JSON 序列化 → MQTT Publish → 云服务器 → 监护人 App
 * 冗余:  发送失败自动重试, 网络断开自动重连
 */

#include "task_alert.h"
#include "hw_air780e.h"
#include "hw_vibrator.h"
#include <string.h>
#include <stdio.h>

/* ========================== MQTT 配置 (来自 app_manager.h) ========================== */
static const MQTT_Config_t mqtt_cfg = {
    .broker          = MQTT_BROKER_ADDR,
    .port            = MQTT_BROKER_PORT,
    .client_id       = MQTT_CLIENT_ID,
    .username        = MQTT_USERNAME,
    .password        = MQTT_PASSWORD,
    .pub_topic       = MQTT_PUB_TOPIC,
    .heartbeat_topic = MQTT_HEARTBEAT_TOPIC,
    .keepalive       = MQTT_KEEPALIVE_SEC,
};

/* ========================== 工具函数 ========================== */

static void BuildAlertJSON(const AlertEvent_t *e, char *buf, uint16_t max)
{
    snprintf(buf, max,
        "{\"type\":%d,\"level\":%d,\"val\":%lu,\"msg\":\"%s\",\"ts\":%lu}",
        (int)e->type, (int)e->level,
        (unsigned long)e->value, e->message,
        (unsigned long)e->timestamp_ms);
}

static uint8_t PublishAlert(const AlertEvent_t *alert)
{
    if (Air780E_GetState() != AIR780E_STATE_READY) return 0;
    if (!Air780E_MQTTIsConnected()) {
        Air780E_MQTTConnect(8000);
        if (Air780E_GetState() != AIR780E_STATE_READY) return 0;
    }
    char json[256];
    BuildAlertJSON(alert, json, sizeof(json));
    return Air780E_MQTTPublish(MQTT_PUB_TOPIC, json);
}

static void PublishHeartbeat(void)
{
    if (Air780E_GetState() != AIR780E_STATE_READY) return;
    int8_t csq = Air780E_GetCSQ();
    char hb[128];
    snprintf(hb, sizeof(hb),
        "{\"st\":\"online\",\"csq\":%d,\"ts\":%lu}",
        (int)csq, (unsigned long)xTaskGetTickCount());
    Air780E_MQTTPublish(MQTT_HEARTBEAT_TOPIC, hb);
}

static uint8_t RecoverLink(void)
{
    Air780E_State_t s = Air780E_GetState();
    if (s == AIR780E_STATE_READY && Air780E_MQTTIsConnected()) return 1;
    return Air780E_AutoInit(&mqtt_cfg, 40000);
}

/* ========================== 任务入口 ========================== */

void Task_AlertSend(void *argument)
{
    AlertEvent_t  alert;
    TickType_t    last_hb, last_check;
    const TickType_t HB_PERIOD  = pdMS_TO_TICKS(MQTT_KEEPALIVE_SEC * 500);
    const TickType_t CHK_PERIOD = pdMS_TO_TICKS(30000);

    (void)argument;

    Air780E_AutoInit(&mqtt_cfg, 50000);
    last_hb = last_check = xTaskGetTickCount();

    while (1)
    {
        if ((xTaskGetTickCount() - last_check) >= CHK_PERIOD) {
            last_check = xTaskGetTickCount();
            if (Air780E_GetState() != AIR780E_STATE_READY) RecoverLink();
        }

        if (xQueueReceive(queue_alert_event, &alert, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            Vibrator_Beep(100);
            if (alert.level >= ALERT_LEVEL_CRITICAL) {
                Vibrator_Beep(100); vTaskDelay(pdMS_TO_TICKS(200)); Vibrator_Beep(100);
            }
            uint8_t n = (alert.level >= ALERT_LEVEL_CRITICAL) ? 3 :
                        (alert.level >= ALERT_LEVEL_DANGER)  ? 2 : 1;
            for (uint8_t i = 0; i < n; i++) {
                if (PublishAlert(&alert)) break;
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }

        if ((xTaskGetTickCount() - last_hb) >= HB_PERIOD) {
            PublishHeartbeat();
            last_hb = xTaskGetTickCount();
        }
    }
}
