#pragma once
#include <stdint.h>
enum { BLE_GAP_EVENT_DISC = 1, BLE_GAP_EVENT_DISC_COMPLETE = 2 };
enum { BLE_HCI_ADV_RPT_EVTYPE_ADV_IND = 0, BLE_HCI_ADV_RPT_EVTYPE_DIR_IND = 1,
       BLE_HCI_ADV_RPT_EVTYPE_SCAN_IND = 2, BLE_HCI_ADV_RPT_EVTYPE_NONCONN_IND = 3,
       BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP = 4 };
struct ble_addr_t { uint8_t type; uint8_t val[6]; };
struct ble_gap_disc_desc {
  ble_addr_t addr; uint8_t event_type; int rssi;
  const uint8_t* data; uint8_t length_data;
};
struct ble_gap_event {
  int type; ble_gap_disc_desc disc;
  struct { int reason; } disc_complete;
};
struct ble_gap_disc_params {
  uint16_t itvl, window;
  uint8_t filter_policy, limited, passive, filter_duplicates;
};
int ble_gap_disc(uint8_t,int32_t,const ble_gap_disc_params*,int(*)(ble_gap_event*,void*),void*);
