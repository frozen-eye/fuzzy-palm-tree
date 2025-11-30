#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

// General settings
#define CFG_TUSB_MCU              OPT_MCU_RP2040   // RP2350 is compatible with RP2040 at the TinyUSB level
#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define BOARD_TUD_RHPORT          0

// Buffer sizes
#define CFG_TUD_ENDPOINT0_SIZE    64

// ====== KEY: 4 CDC + MGMT ======
#define CFG_TUD_CDC               5

// we can disable everything else
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

// CDC parameters
#define CFG_TUD_CDC_RX_BUFSIZE    256
#define CFG_TUD_CDC_TX_BUFSIZE    256

#endif
