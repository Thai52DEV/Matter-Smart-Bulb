/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * OpenThread Command Line Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_openthread.h"
#include "esp_openthread_cli.h"
#include "esp_openthread_lock.h"
#include "esp_openthread_netif_glue.h"
#include "esp_openthread_types.h"
#include "esp_ot_config.h"
#include "esp_vfs_eventfd.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/uart_types.h"
#include "nvs_flash.h"
#include "openthread/cli.h"
#include "openthread/instance.h"
#include "openthread/logging.h"
#include "openthread/tasklet.h"
#include "openthread/udp.h"
#include "openthread/ip6.h"
#include "openthread/thread.h"

#if CONFIG_OPENTHREAD_STATE_INDICATOR_ENABLE
#include "ot_led_strip.h"
#endif

#if CONFIG_OPENTHREAD_CLI_ESP_EXTENSION
#include "esp_ot_cli_extension.h"
#endif // CONFIG_OPENTHREAD_CLI_ESP_EXTENSION

#define TAG "ot_esp_cli"
#define UDP_PORT 5083
#define MULTICAST_ADDR "ff04::123"

static otUdpSocket sUdpSocket;
static bool udp_initialized = false;
static otInstance *g_instance = NULL;

// Callback khi nhận UDP packet
static void udp_receive_callback(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    char buf[128];
    int len = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = 0;
        printf("Received UDP from [%x:%x:%x:%x:%x:%x:%x:%x]:%d: %s\n",
               aMessageInfo->mPeerAddr.mFields.m16[0], aMessageInfo->mPeerAddr.mFields.m16[1],
               aMessageInfo->mPeerAddr.mFields.m16[2], aMessageInfo->mPeerAddr.mFields.m16[3],
               aMessageInfo->mPeerAddr.mFields.m16[4], aMessageInfo->mPeerAddr.mFields.m16[5],
               aMessageInfo->mPeerAddr.mFields.m16[6], aMessageInfo->mPeerAddr.mFields.m16[7],
               aMessageInfo->mPeerPort, buf);
    }
}

// Hàm khởi tạo UDP socket và join multicast
static void udp_init_and_join_mcast(otInstance *instance)
{
    otError error;
    otIp6Address mcastAddr;
    otSockAddr bindAddr;

    if (udp_initialized) {
        printf("UDP multicast already initialized\n");
        return;
    }

    // Join multicast group
    otIp6AddressFromString(MULTICAST_ADDR, &mcastAddr);
    error = otIp6SubscribeMulticastAddress(instance, &mcastAddr);
    if (error != OT_ERROR_NONE) {
        printf("Join multicast failed: %d\n", error);
        return;
    } else {
        printf("Joined multicast group: %s\n", MULTICAST_ADDR);
    }

    // Open UDP socket
    error = otUdpOpen(instance, &sUdpSocket, udp_receive_callback, NULL);
    if (error != OT_ERROR_NONE) {
        printf("UDP open failed: %d\n", error);
        return;
    }

    // Bind UDP socket
    memset(&bindAddr, 0, sizeof(bindAddr));
    bindAddr.mPort = UDP_PORT; // Bind all local addresses (::)
    error = otUdpBind(instance, &sUdpSocket, &bindAddr, OT_NETIF_THREAD);
    if (error != OT_ERROR_NONE) {
        printf("UDP bind failed: %d\n", error);
        otUdpClose(instance, &sUdpSocket);
        return;
    } else {
        printf("UDP socket bound to port %d\n", UDP_PORT);
    }

    udp_initialized = true;
    printf("UDP multicast initialization completed successfully!\n");
}

// Hàm gửi dữ liệu UDP tới multicast group
static void udp_send_mcast(otInstance *instance, const char *payload)
{
    if (!udp_initialized) {
        printf("UDP not initialized yet\n");
        return;
    }

    otMessage *msg;
    otMessageInfo msgInfo;
    otIp6Address mcastAddr;

    otIp6AddressFromString(MULTICAST_ADDR, &mcastAddr);

    memset(&msgInfo, 0, sizeof(msgInfo));
    msgInfo.mPeerAddr = mcastAddr;
    msgInfo.mPeerPort = UDP_PORT;

    msg = otUdpNewMessage(instance, NULL);
    if (msg == NULL) {
        printf("Failed to create UDP message\n");
        return;
    }

    otMessageAppend(msg, payload, strlen(payload));
    otError error = otUdpSend(instance, &sUdpSocket, msg, &msgInfo);
    if (error != OT_ERROR_NONE) {
        printf("UDP send failed: %d\n", error);
        otMessageFree(msg);
    } else {
        printf("Sent UDP multicast: %s\n", payload);
    }
}

// Callback để theo dõi trạng thái Thread
static void thread_state_changed_callback(otChangedFlags aFlags, void *aContext)
{
    otInstance *instance = (otInstance *)aContext;
    
    if (aFlags & OT_CHANGED_THREAD_ROLE) {
        otDeviceRole role = otThreadGetDeviceRole(instance);
        printf("Thread role changed to: ");
        switch (role) {
            case OT_DEVICE_ROLE_DISABLED:
                printf("Disabled\n");
                break;
            case OT_DEVICE_ROLE_DETACHED:
                printf("Detached\n");
                break;
            case OT_DEVICE_ROLE_CHILD:
                printf("Child\n");
                // Khởi tạo UDP khi đã tham gia mạng
                if (!udp_initialized) {
                    printf("Device joined network, initializing UDP multicast...\n");
                    udp_init_and_join_mcast(instance);
                }
                break;
            case OT_DEVICE_ROLE_ROUTER:
                printf("Router\n");
                // Khởi tạo UDP khi đã tham gia mạng
                if (!udp_initialized) {
                    printf("Device joined network, initializing UDP multicast...\n");
                    udp_init_and_join_mcast(instance);
                }
                break;
            case OT_DEVICE_ROLE_LEADER:
                printf("Leader\n");
                // Khởi tạo UDP khi đã tham gia mạng
                if (!udp_initialized) {
                    printf("Device joined network, initializing UDP multicast...\n");
                    udp_init_and_join_mcast(instance);
                }
                break;
        }
    }
}

// CLI command để test gửi UDP
static void send_test_udp(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otInstance *instance = esp_openthread_get_instance();
    if (aArgsLength > 0) {
        udp_send_mcast(instance, aArgs[0]);
    } else {
        udp_send_mcast(instance, "test message from ESP32");
    }
}

static otCliCommand sCommands[] = {
    {"sendudp", send_test_udp},
};

static esp_netif_t *init_openthread_netif(const esp_openthread_platform_config_t *config)
{
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_OPENTHREAD();
    esp_netif_t *netif = esp_netif_new(&cfg);
    assert(netif != NULL);
    ESP_ERROR_CHECK(esp_netif_attach(netif, esp_openthread_netif_glue_init(config)));

    return netif;
}

static void ot_task_worker(void *aContext)
{
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };

    // Initialize the OpenThread stack
    ESP_ERROR_CHECK(esp_openthread_init(&config));
    otInstance *instance = esp_openthread_get_instance();
    g_instance = instance;

    // Đăng ký callback để theo dõi trạng thái Thread
    otSetStateChangedCallback(instance, thread_state_changed_callback, instance);

#if CONFIG_OPENTHREAD_STATE_INDICATOR_ENABLE
    ESP_ERROR_CHECK(esp_openthread_state_indicator_init(instance));
#endif

#if CONFIG_OPENTHREAD_LOG_LEVEL_DYNAMIC
    // The OpenThread log level directly matches ESP log level
    (void)otLoggingSetLevel(CONFIG_LOG_DEFAULT_LEVEL);
#endif
    // Initialize the OpenThread cli
#if CONFIG_OPENTHREAD_CLI
    esp_openthread_cli_init();
    // Thêm custom command
    otCliSetUserCommands(sCommands, sizeof(sCommands) / sizeof(sCommands[0]), instance);
#endif

    esp_netif_t *openthread_netif;
    // Initialize the esp_netif bindings
    openthread_netif = init_openthread_netif(&config);
    esp_netif_set_default_netif(openthread_netif);

#if CONFIG_OPENTHREAD_CLI_ESP_EXTENSION
    esp_cli_custom_command_init();
#endif // CONFIG_OPENTHREAD_CLI_ESP_EXTENSION

    printf("OpenThread CLI initialized. Use 'ifconfig up' and 'thread start' to join network.\n");
    printf("UDP multicast will be automatically initialized when device joins the network.\n");

    // Run the main loop
#if CONFIG_OPENTHREAD_CLI
    esp_openthread_cli_create_task();
#endif
#if CONFIG_OPENTHREAD_AUTO_START
    otOperationalDatasetTlvs dataset;
    otError error = otDatasetGetActiveTlvs(instance, &dataset);
    ESP_ERROR_CHECK(esp_openthread_auto_start((error == OT_ERROR_NONE) ? &dataset : NULL));
#endif
    esp_openthread_launch_mainloop();

    // Clean up
    esp_openthread_netif_glue_deinit();
    esp_netif_destroy(openthread_netif);

    esp_vfs_eventfd_unregister();
    vTaskDelete(NULL);
}

void app_main(void)
{
    // Used eventfds:
    // * netif
    // * ot task queue
    // * radio driver
    esp_vfs_eventfd_config_t eventfd_config = {
        .max_fds = 3,
    };

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_vfs_eventfd_register(&eventfd_config));
    xTaskCreate(ot_task_worker, "ot_cli_main", 10240, xTaskGetCurrentTaskHandle(), 5, NULL);
}