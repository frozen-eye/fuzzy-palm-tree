# RP2350 Multi-UART USB Bridge

USB-to-UART bridge for Raspberry Pi Pico 2 (RP2350) providing 4 data ports and 1 management port via USB CDC interfaces.

## Description

This project implements a USB-to-UART bridge that exposes multiple UART ports as USB CDC (Communication Device Class) interfaces. The device provides:

- **4 data ports** (CDC interfaces 0-3) for transparent UART bridging
- **1 management port** (CDC interface 4) for configuration and monitoring
- **2 hardware UART ports** (UART0, UART1) using RP2350's built-in UART peripherals
- **2 PIO UART ports** (UART2, UART3) using Programmable I/O state machines

Each data port appears as a standard USB serial port on the host computer, allowing transparent bidirectional communication with UART devices.

## Features

- ✅ 4 simultaneous USB CDC interfaces for data transfer
- ✅ 1 management console for configuration
- ✅ Automatic line coding support (baud rate, parity, stop bits)
- ✅ Hardware UART support (2 ports)
- ✅ PIO-based UART support (2 ports)
- ✅ Heartbeat LED indicator
- ✅ Doxygen documentation
- ✅ Management console with commands:
  - `help` - Show available commands
  - `show` - Display port configuration
  - `set <port> baud=<rate> parity=<0|1|2> stop=<1|2>` - Configure UART port

## Project Structure

```
.
├── .devcontainer/          # Dev Container configuration
│   ├── devcontainer.json   # VS Code Dev Container config
│   ├── Dockerfile          # Docker image for development
│   └── post-create.sh      # Environment initialization script
├── CMakeLists.txt          # Build configuration
├── pico_sdk_import.cmake   # Pico SDK import
├── include/
│   ├── tusb_config.h       # TinyUSB configuration
│   └── version.h           # Version information
├── pio/
│   ├── uart_tx.pio         # PIO program for UART TX
│   └── uart_rx.pio         # PIO program for UART RX
├── src/
│   ├── main.c              # Main application code
│   └── usb_descriptors.c   # USB descriptor definitions
└── README.md               # This file
```

## Requirements

- Raspberry Pi Pico SDK
- CMake 3.13 or higher
- ARM GCC toolchain (arm-none-eabi-gcc)
- TinyUSB (included with Pico SDK)

## Quick Start with Dev Container (Recommended)

The easiest way to start development is using Dev Container in VS Code:

1. Install extensions:
   - [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
   - [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
   - [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)

2. Open the project in VS Code

3. Press `F1` and select **"Dev Containers: Reopen in Container"**

4. VS Code will automatically build the Docker image and configure the environment

5. After the container loads, Pico SDK will be automatically installed

6. Build the project:
```bash
mkdir -p build
cd build
cmake ..
make
```

7. Upload `rp_multi_uart_bridge.uf2` to your Pico 2

## Manual Environment Setup

If you prefer to set up the environment manually:

1. Install Pico SDK:
```bash
git clone https://github.com/raspberrypi/pico-sdk.git
export PICO_SDK_PATH=/path/to/pico-sdk
cd pico-sdk
git submodule update --init
```

2. Install required tools:
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential

# macOS (with Homebrew)
brew install cmake arm-none-eabi-gcc
```

3. Create build directory:
```bash
mkdir build
cd build
```

4. Run CMake:
```bash
cmake ..
```

5. Build the project:
```bash
make
```

6. Upload `rp_multi_uart_bridge.uf2` to your Pico 2

## Usage

### Hardware Connections

The default GPIO pin assignments are:

- **UART0** (Hardware): GPIO 0 (TX), GPIO 1 (RX) → USB CDC Interface 0
- **UART1** (Hardware): GPIO 4 (TX), GPIO 5 (RX) → USB CDC Interface 1
- **UART2** (PIO): GPIO 8 (TX), GPIO 9 (RX) → USB CDC Interface 2
- **UART3** (PIO): GPIO 12 (TX), GPIO 13 (RX) → USB CDC Interface 3
- **Management Console**: USB CDC Interface 4

### Connecting to Host

After connecting the device via USB:

1. The device will appear as 5 USB serial ports:
   - `/dev/ttyACM0` - Data port 0 (UART0)
   - `/dev/ttyACM1` - Data port 1 (UART1)
   - `/dev/ttyACM2` - Data port 2 (UART2)
   - `/dev/ttyACM3` - Data port 3 (UART3)
   - `/dev/ttyACM4` - Management console

2. Connect to the management console:
```bash
screen /dev/ttyACM4 115200
# or
minicom -D /dev/ttyACM4 -b 115200
```

3. Use the management commands:
```
> help
Available commands:
  help                      - show this help
  show                      - show ports configuration
  set <port> baud=<v> parity=<0|1|2> stop=<1|2>
                            - configure UART port

> show
Ports:
  port 0: baud=115200 parity=0 stop=0 type=HW
  port 1: baud=115200 parity=0 stop=0 type=HW
  port 2: baud=115200 parity=0 stop=0 type=PIO
  port 3: baud=115200 parity=0 stop=0 type=PIO

> set 0 baud=9600 parity=0 stop=1
OK
```

### Using Data Ports

Data ports work transparently - any data sent to the USB CDC interface is forwarded to the corresponding UART port, and vice versa.

Example using `screen`:
```bash
# Connect to data port 0 (UART0)
screen /dev/ttyACM0 115200

# Or using minicom
minicom -D /dev/ttyACM0 -b 115200
```

Example using Python:
```python
import serial

# Open data port 0
ser = serial.Serial('/dev/ttyACM0', 115200)

# Send data
ser.write(b'Hello UART!\r\n')

# Read data
data = ser.read(64)
print(data)

ser.close()
```

### Automatic Line Coding

The device automatically configures UART parameters when the host sets line coding on the USB CDC interface. This means you can change baud rate, parity, and stop bits from your serial terminal application, and the device will automatically reconfigure the UART port.

## Port Configuration

### Default Settings

- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None

### Configuration Methods

1. **Via Management Console**: Use the `set` command
2. **Via USB CDC Line Coding**: Host application sets line coding automatically
3. **At Runtime**: Configuration can be changed without restarting the device

## Resource Allocation

- **UART 0-1**: Hardware UART modules (uart0, uart1)
- **UART 2-3**: PIO state machines (pio0, SM 0-3)

PIO resources are allocated automatically:
- PIO0: 4 state machines available
- Each PIO UART uses 2 state machines (TX and RX)

## Limitations

- PIO UART has lower performance compared to hardware UART
- Maximum baud rate for PIO UART may be limited
- PIO UART requires more CPU resources
- Only 4 data ports are currently supported (can be extended)

## LED Indicator

The device includes a heartbeat LED that blinks every 500ms to indicate the device is running. The LED pin is defined by `PICO_DEFAULT_LED_PIN` (typically GPIO 25 on Pico, may vary on Pico 2).

## Building Documentation

The code is documented with Doxygen comments. To generate documentation:

```bash
# Install Doxygen and Graphviz (if not already installed)
sudo apt-get install doxygen graphviz

# Generate documentation
doxygen Doxyfile

# Documentation will be generated in docs/html/
# Open docs/html/index.html in a web browser
```

The Doxyfile is configured to:
- Extract documentation from `src/` and `include/` directories
- Use README.md as the main page
- Exclude Pico SDK and build directories
- Generate HTML output with search functionality
- Include source code browsing

## USB Device Information

- **Vendor ID**: 0xCafe
- **Product ID**: 0x4000
- **Manufacturer**: Chainsaw Labs
- **Product**: RP2350 4xCDC UART Bridge

## Troubleshooting

### Device Not Recognized

- Ensure USB cable supports data transfer (not charge-only)
- Check USB port is working with other devices
- Try different USB port or cable

### Ports Not Appearing

- Check device is powered and LED is blinking
- Verify USB connection
- Check `dmesg` or system logs for USB device information

### Data Not Transferring

- Verify UART connections (TX/RX pins)
- Check baud rate matches on both ends
- Ensure ground connection between devices
- Check management console for port status

## License

MIT License - see LICENSE file

## Author

FrozenEye

## Acknowledgments

- Raspberry Pi Foundation for Pico SDK
- TinyUSB project for USB stack
- Pico SDK community
