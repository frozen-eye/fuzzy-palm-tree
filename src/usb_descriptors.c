#include "tusb.h"
#include <string.h>

// ============================================================================
// String descriptor indices
// ============================================================================

/**
 * @brief String descriptor index enumeration
 */
enum {
  STRID_LANGID = 0,     ///< Language ID string
  STRID_MANUFACTURER,   ///< Manufacturer string
  STRID_PRODUCT,        ///< Product string
};

// ============================================================================
// Device descriptor
// ============================================================================

/**
 * @brief USB device descriptor
 */
tusb_desc_device_t const desc_device =
{
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,

  .bDeviceClass       = TUSB_CLASS_MISC,
  .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol    = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

  .idVendor           = 0xCafe,
  .idProduct          = 0x4000,
  .bcdDevice          = 0x0100,

  .iManufacturer      = STRID_MANUFACTURER,
  .iProduct           = STRID_PRODUCT,
  .iSerialNumber      = 0x00,

  .bNumConfigurations = 0x01
};

/**
 * @brief TinyUSB device descriptor callback
 * @return Pointer to device descriptor
 */
uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

// ============================================================================
// Configuration descriptor
// ============================================================================

/**
 * @brief Interface number enumeration
 */
enum {
  ITF_NUM_CDC0 = 0,        ///< CDC0 control interface
  ITF_NUM_CDC0_DATA,       ///< CDC0 data interface
  ITF_NUM_CDC1,           ///< CDC1 control interface
  ITF_NUM_CDC1_DATA,       ///< CDC1 data interface
  ITF_NUM_CDC2,           ///< CDC2 control interface
  ITF_NUM_CDC2_DATA,       ///< CDC2 data interface
  ITF_NUM_CDC3,           ///< CDC3 control interface
  ITF_NUM_CDC3_DATA,       ///< CDC3 data interface
  ITF_NUM_CDC4,           ///< CDC4 control interface (management)
  ITF_NUM_CDC4_DATA,       ///< CDC4 data interface (management)
  ITF_NUM_TOTAL            ///< Total number of interfaces
};

/// Total configuration descriptor length
#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN*5)

/// CDC0 endpoint numbers
#define EPNUM_CDC0_NOTIF   0x81  ///< Notification endpoint
#define EPNUM_CDC0_OUT     0x02  ///< OUT endpoint
#define EPNUM_CDC0_IN      0x82  ///< IN endpoint

/// CDC1 endpoint numbers
#define EPNUM_CDC1_NOTIF   0x83  ///< Notification endpoint
#define EPNUM_CDC1_OUT     0x04  ///< OUT endpoint
#define EPNUM_CDC1_IN      0x84  ///< IN endpoint

/// CDC2 endpoint numbers
#define EPNUM_CDC2_NOTIF   0x85  ///< Notification endpoint
#define EPNUM_CDC2_OUT     0x06  ///< OUT endpoint
#define EPNUM_CDC2_IN      0x86  ///< IN endpoint

/// CDC3 endpoint numbers
#define EPNUM_CDC3_NOTIF   0x87  ///< Notification endpoint
#define EPNUM_CDC3_OUT     0x08  ///< OUT endpoint
#define EPNUM_CDC3_IN      0x88  ///< IN endpoint

/// CDC4 endpoint numbers (management)
#define EPNUM_CDC4_NOTIF   0x89  ///< Notification endpoint
#define EPNUM_CDC4_OUT     0x0A  ///< OUT endpoint
#define EPNUM_CDC4_IN      0x8A  ///< IN endpoint

/**
 * @brief Full-speed configuration descriptor
 * @note Contains configuration descriptor and 5 CDC descriptors (4 data + 1 management)
 */
uint8_t const desc_fs_configuration[] =
{
  // Configuration descriptor
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  // CDC0 (data port 0)
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC0, 0,
                     EPNUM_CDC0_NOTIF,
                     8,
                     EPNUM_CDC0_OUT,
                     EPNUM_CDC0_IN,
                     64),

  // CDC1 (data port 1)
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC1, 0,
                     EPNUM_CDC1_NOTIF,
                     8,
                     EPNUM_CDC1_OUT,
                     EPNUM_CDC1_IN,
                     64),

  // CDC2 (data port 2)
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC2, 0,
                     EPNUM_CDC2_NOTIF,
                     8,
                     EPNUM_CDC2_OUT,
                     EPNUM_CDC2_IN,
                     64),

  // CDC3 (data port 3)
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC3, 0,
                     EPNUM_CDC3_NOTIF,
                     8,
                     EPNUM_CDC3_OUT,
                     EPNUM_CDC3_IN,
                     64),

  // CDC4 (management port)
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC4, 0,
                     EPNUM_CDC4_NOTIF,
                     8,
                     EPNUM_CDC4_OUT,
                     EPNUM_CDC4_IN,
                     64),
};

/**
 * @brief TinyUSB configuration descriptor callback
 * @param index Configuration index (unused, always 0)
 * @return Pointer to configuration descriptor
 */
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index;
  return desc_fs_configuration;
}

// ============================================================================
// String descriptors
// ============================================================================

/**
 * @brief String descriptor array
 */
char const* string_desc_arr[] =
{
  (const char[]) { 0x09, 0x04 }, ///< 0: LANGID = 0x0409 (en-US)
  "Chainsaw Labs",              ///< 1: Manufacturer
  "RP2350 4xCDC UART Bridge",   ///< 2: Product
};

/// String descriptor buffer
static uint16_t _desc_str[32];

/**
 * @brief TinyUSB string descriptor callback
 * @param index String descriptor index
 * @param langid Language ID (unused)
 * @return Pointer to string descriptor
 */
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  if ( index == 0 )
  {
    // Language ID string
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }
  else
  {
    // Regular string descriptor
    const char* str = string_desc_arr[index];
    chr_count = (uint8_t) strlen(str);
    if ( chr_count > 31 ) chr_count = 31;  // Limit to 31 characters
    
    for ( uint8_t i = 0; i < chr_count; i++ ) {
      _desc_str[1+i] = str[i];
    }
  }

  // Set descriptor header: type (STRING) and length
  _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

  return _desc_str;
}
