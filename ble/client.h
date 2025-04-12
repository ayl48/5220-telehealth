// =======================================================================
// client.h
// =======================================================================
// Declarations of our BLE client functions
//
// Note that users should only ever construct ONE client, due to the
// global callback function used for GATT events

#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include "btstack.h"
#include "utils/pt_cornell_rp2040_v1.h"
#include <cstdint>

// Maximum length of a BLE Characteristic Description
#define GATT_MAX_DESCRIPTION_LENGTH 50

// Maximum length of a BLE Characteristic Value
#define GATT_MAX_VALUE_LENGTH 100

// -----------------------------------------------------------------------
// GATT State Machine States
// -----------------------------------------------------------------------

enum gc_state_t {
  TC_OFF = 0,
  TC_IDLE,
  TC_W4_SCAN_RESULT,
  TC_W4_CONNECT,
  TC_W4_SERVICE_RESULT,
  TC_W4_CHARACTERISTIC_RESULT,
  TC_W4_CHARACTERISTIC_DESCRIPTOR,
  TC_W4_CHARACTERISTIC_DESCRIPTION,
  TC_W4_CHARACTERISTIC_VALUE,
  TC_W4_CHARACTERISTIC_CONFIG,
  TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
  TC_W4_READY
};

// -----------------------------------------------------------------------
// BLE Scan Parameters
// -----------------------------------------------------------------------

enum gap_scan_type_t { GAP_SCAN_PASSIVE = 0, GAP_SCAN_ACTIVE = 1 };
enum gap_scan_policy_t { GAP_SCAN_ALL = 0, GAP_SCAN_WHITELIST = 1 };

// -----------------------------------------------------------------------
// Two descriptors per characteristic
// -----------------------------------------------------------------------

typedef gatt_client_characteristic_descriptor_t
    gatt_client_characteristic_descriptors_t[2];

// -----------------------------------------------------------------------
// Client
// -----------------------------------------------------------------------

class BaseClient {
  // Just a class with necessary handlers, to avoid templating issues
 public:
  virtual void gatt_client_event_handler( uint8_t  packet_type,
                                          uint16_t channel,
                                          uint8_t* packet,
                                          uint16_t size )          = 0;
  virtual void hci_event_handler( uint8_t packet_type, uint16_t channel,
                                  uint8_t* packet, uint16_t size ) = 0;
};

template <int MAX_CHARACTERISTICS>
class Client : public BaseClient {
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BLE Definitions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Child classes will need to override these

  virtual uint16_t get_service()      = 0;
  virtual uint8_t* get_service_name() = 0;  // 16 entries for 128b UUID

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Samephore for waiting for characteristics to be discovered
  struct pt_sem characteristics_discovered;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Public accessor functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Client();
  ~Client();

  // Connecting to/from servers
  void connect_to_server();
  void disconnect_from_server();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  gc_state_t                             state;
  btstack_packet_callback_registration_t hci_event_callback_registration;

  // Address of server we're connected to
  bd_addr_t      server_addr;
  bd_addr_type_t server_addr_type;

  // Current connection handle
  hci_con_handle_t connection_handle;

  // Service information
  gatt_client_service_t server_service;

  // Listener
  bool                       listener_registered;
  gatt_client_notification_t notification_listener;

  // Characteristics
  char                         notifications_enabled[MAX_CHARACTERISTICS];
  gatt_client_characteristic_t server_characteristic[MAX_CHARACTERISTICS];
  gatt_client_characteristic_descriptors_t
      server_characteristic_descriptor[MAX_CHARACTERISTICS];
  uint8_t
      server_characteristic_user_description[MAX_CHARACTERISTICS]
                                            [GATT_MAX_DESCRIPTION_LENGTH];
  char server_characteristic_values[MAX_CHARACTERISTICS]
                                   [GATT_MAX_VALUE_LENGTH];
  uint16_t server_characteristic_configurations[MAX_CHARACTERISTICS];
  int      num_characteristics_discovered;

  // Characteristic helper values
  int curr_char_idx;
  int curr_char_descr_idx;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Protected functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 protected:
  bool correct_service( uint8_t* advertisement_report );

  // Helper functions for state transitions
  void off();
  void start();
  void connect();
  void service_discovery();
  void characteristic_discovery();
  void characteristic_descriptor_discovery();
  void characteristic_description_discovery();
  void read_characteristic_value();
  void read_characteristic_config();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Event handlers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 public:
  void gatt_client_event_handler( uint8_t packet_type, uint16_t channel,
                                  uint8_t* packet,
                                  uint16_t size ) override;
  void gatt_client_notification_handler( uint8_t* packet );
  void hci_event_handler( uint8_t packet_type, uint16_t channel,
                          uint8_t* packet, uint16_t size ) override;
};

#endif  // BLE_CLIENT_H