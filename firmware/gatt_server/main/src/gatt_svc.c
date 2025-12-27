/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "gatt_svc.h"
#include "common.h"

static int set_wifi_chr_access(uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg);

static int set_time_chr_access(uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg);

static int set_alarm_chr_access(uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Automation IO service */
static const ble_uuid128_t config_svc_uuid =
    BLE_UUID128_INIT(0x12,0x34,0x56,0x78,
                     0x9a,0xbc,0xde,0xf0,
                     0xf0,0xde,0xbc,0x9a,
                     0x78,0x56,0x34,0x12);

/*                     
static uint16_t set_wifi_chr_val_handle;
static uint16_t set_time_chr_val_handle;
static uint16_t set_alarm_chr_val_handle;
*/

/* set wifi uid */
static const ble_uuid128_t set_wifi_chr_uuid =
    BLE_UUID128_INIT(0x9a,0xbc,0xde,0xf0,
                     0x12,0x34,0x56,0x78,
                     0x78,0x56,0x34,0x12,
                     0xf0,0xde,0xbc,0x9a);

/* set wifi uid */
static const ble_uuid128_t set_time_chr_uuid =
    BLE_UUID128_INIT(0x9b,0xbc,0xde,0xf0,
                     0x12,0x34,0x56,0x78,
                     0x78,0x56,0x34,0x12,
                     0xf0,0xde,0xbc,0x9b);

/* set wifi uid */
static const ble_uuid128_t set_alarm_chr_uuid =
    BLE_UUID128_INIT(0x9c,0xbc,0xde,0xf0,
                     0x12,0x34,0x56,0x78,
                     0x78,0x56,0x34,0x12,
                     0xf0,0xde,0xbc,0x9c);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &config_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            /* STRING WRITE characteristic */
            {
                .uuid = &set_wifi_chr_uuid.u,
                .access_cb = set_wifi_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                /* val_handle OPTIONAL */
            },

            /* STRING WRITE characteristic */
            {
                .uuid = &set_time_chr_uuid.u,
                .access_cb = set_time_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                /* val_handle OPTIONAL */
            },

            /* STRING WRITE characteristic */
            {
                .uuid = &set_alarm_chr_uuid.u,
                .access_cb = set_alarm_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                /* val_handle OPTIONAL */
            },            

            {0}  /* <-- KRAJ CHARACTERISTICS */
        },
    },

    {0}  /* <-- KRAJ SERVICES */
};

static int
set_wifi_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    char buf[128] = {0};
    int len = OS_MBUF_PKTLEN(ctxt->om);

    if (len >= sizeof(buf)) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf) - 1, NULL);

    ESP_LOGI(TAG, "STRING RX (%d bytes): %s", len, buf);

    return 0;
}

static int
set_time_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    char buf[128] = {0};
    int len = OS_MBUF_PKTLEN(ctxt->om);

    if (len >= sizeof(buf)) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf) - 1, NULL);

    ESP_LOGI(TAG, "STRING RX (%d bytes): %s", len, buf);

    return 0;
}

static int
set_alarm_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                  struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    char buf[128] = {0};
    int len = OS_MBUF_PKTLEN(ctxt->om);

    if (len >= sizeof(buf)) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf) - 1, NULL);

    ESP_LOGI(TAG, "STRING RX (%d bytes): %s", len, buf);

    return 0;
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

/*
 *  GATT server subscribe event callback
 *      1. Update heart rate subscription status
 */

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
