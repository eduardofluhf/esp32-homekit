#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#include "hap.h"



#define TAG "SWITCH"

#define ACCESSORY_NAME  "SWITCH"
#define MANUFACTURER_NAME   "Eduardo"
#define MODEL_NAME  "ESP32_ACC"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#if 1
#define EXAMPLE_ESP_WIFI_SSID "Eduardo"
#define EXAMPLE_ESP_WIFI_PASS "1020304050"
#endif
#if 0
#define EXAMPLE_ESP_WIFI_SSID "NO_RUN"
#define EXAMPLE_ESP_WIFI_PASS "1qaz2wsx"
#endif

static gpio_num_t LED_PORT_1 = GPIO_NUM_2;
static gpio_num_t LED_PORT_2 = GPIO_NUM_14;
static gpio_num_t LED_PORT_3 = GPIO_NUM_15;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static void* a;
static void* _ev_handle_1;
static void* _ev_handle_2;
static void* _ev_handle_3;
static int led_1 = false;
static int led_2 = false;
static int led_3 = false;

void* led_read_1(void* arg)
{
    printf("[MAIN] LED READ\n");
    return (void*)led_1;
}

void* led_read_2(void* arg)
{
    printf("[MAIN] LED READ\n");
    return (void*)led_2;
}

void* led_read_3(void* arg)
{
    printf("[MAIN] LED READ\n");
    return (void*)led_3;
}

void led_write_1(void* arg, void* value, int len)
{
    printf("[MAIN] LED WRITE. %d\n", (int)value);

    led_1 = (int)value;
    if (value) {
        led_1 = true;
        gpio_set_level(LED_PORT_1, 1);
    }
    else {
        led_1 = false;
        gpio_set_level(LED_PORT_1, 0);
    }

    if (_ev_handle_1)
        hap_event_response(a, _ev_handle_1, (void*)led_1);

    return;
}

void led_write_2(void* arg, void* value, int len)
{
    printf("[MAIN] LED WRITE. %d\n", (int)value);

    led_2 = (int)value;
    if (value) {
        led_2 = true;
        gpio_set_level(LED_PORT_2, 1);
    }
    else {
        led_2 = false;
        gpio_set_level(LED_PORT_2, 0);
    }

    if (_ev_handle_2)
        hap_event_response(a, _ev_handle_2, (void*)led_2);

    return;
}

void led_write_3(void* arg, void* value, int len)
{
    printf("[MAIN] LED WRITE. %d\n", (int)value);

    led_3 = (int)value;
    if (value) {
        led_3 = true;
        gpio_set_level(LED_PORT_3, 1);
    }
    else {
        led_3 = false;
        gpio_set_level(LED_PORT_3, 0);
    }

    if (_ev_handle_3)
        hap_event_response(a, _ev_handle_3, (void*)led_3);

    return;
}

void led_notify_1(void* arg, void* ev_handle, bool enable)
{
    if (enable) {
        _ev_handle_1 = ev_handle;
    }
    else {
        _ev_handle_1 = NULL;
    }
}

void led_notify_2(void* arg, void* ev_handle, bool enable)
{
    if (enable) {
        _ev_handle_2 = ev_handle;
    }
    else {
        _ev_handle_2 = NULL;
    }
}
void led_notify_3(void* arg, void* ev_handle, bool enable)
{
    if (enable) {
        _ev_handle_3 = ev_handle;
    }
    else {
        _ev_handle_3 = NULL;
    }
}

static bool _identifed = false;
void* identify_read(void* arg)
{
    return (void*)true;
}

void hap_object_init(void* arg)
{
    void* accessory_object = hap_accessory_add(a);
    struct hap_characteristic cs[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.0", NULL, NULL, NULL, NULL},
    };
    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_ACCESSORY_INFORMATION, cs, ARRAY_SIZE(cs));

    struct hap_characteristic c1[] = {
        {HAP_CHARACTER_ON, (void*)led_1, NULL, led_read_1, led_write_1, led_notify_1},
    };
    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_SWITCHS, c1, ARRAY_SIZE(c1));

    struct hap_characteristic c2[] = {
        {HAP_CHARACTER_ON, (void*)led_2, NULL, led_read_2, led_write_2, led_notify_2},
    };
    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_SWITCHS, c2, ARRAY_SIZE(c2));

    struct hap_characteristic c3[] = {
        {HAP_CHARACTER_ON, (void*)led_3, NULL, led_read_3, led_write_3, led_notify_3},
    };
    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_SWITCHS, c3, ARRAY_SIZE(c3));
}


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        {
            hap_init();

            uint8_t mac[6];
            esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
            char accessory_id[32] = {0,};
            sprintf(accessory_id, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            hap_accessory_callback_t callback;
            callback.hap_object_init = hap_object_init;
            a = hap_accessory_register((char*)ACCESSORY_NAME, accessory_id, (char*)"053-58-197", (char*)MANUFACTURER_NAME, HAP_ACCESSORY_CATEGORY_OTHER, 811, 1, NULL, &callback);
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    gpio_pad_select_gpio(LED_PORT_1);
    gpio_pad_select_gpio(LED_PORT_2);
    gpio_pad_select_gpio(LED_PORT_3);
    gpio_set_direction(LED_PORT_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_PORT_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_PORT_3, GPIO_MODE_OUTPUT);

    wifi_init_sta();
}
