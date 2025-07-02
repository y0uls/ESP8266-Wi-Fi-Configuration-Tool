# ESP8266 Wi-Fi Configuration Tool 🛠️

This project provides a simple web-based interface for configuring Wi-Fi settings on an ESP8266-based device. The device can operate in two modes: **Access Point (AP)** and **Client**. When in AP mode, users can access the configuration page to set up the device. In Client mode, the device connects to a Wi-Fi network using the credentials stored in EEPROM.

## Features ⚙️

- **Wi-Fi Setup via Web Interface**: Users can select or manually enter an SSID and set a password for the device to connect to.
- **Reset Wi-Fi Settings**: A reset page allows users to clear the stored Wi-Fi credentials and revert to AP mode.
- **MDNS Support**: The device can be accessed via `http://esp8266.local` when in Client mode.
- **LED Status Indicators**: The on-board LED shows different states based on Wi-Fi connection status.

## Components 🧩

- **ESP8266**: The main microcontroller running the web server.
- **LED**: Used for indicating the current Wi-Fi status (ON, OFF, BLINKING).
- **Button**: For triggering Wi-Fi reset, using the reset pin.

## Setup 🚀

### 1. Install Required Libraries 📚

To use this project, ensure you have the following libraries installed in the Arduino IDE:

- `ESP8266WiFi`
- `ESP8266WebServer`
- `DNSServer`
- `EEPROM`
- `ESP8266mDNS`

### 2. Configure Wi-Fi 🌐

The device can either operate in **Access Point** (AP) mode or **Client** mode. In AP mode, the device creates a Wi-Fi hotspot (`APWEMOS`) where users can connect to and configure the device to join a different Wi-Fi network.

Once the credentials are saved, the device restarts in **Client** mode and connects to the configured network. If the credentials are incorrect or not set, the device will fall back to AP mode for reconfiguration.

### 3. Access the Device 🖥️

- **AP Mode**: Connect to the Wi-Fi network `APWEMOS` and open a browser. The login page will appear, where you can set up the SSID and password.
- **Client Mode**: After configuration, the device will connect to the specified Wi-Fi network and can be accessed via `http://esp8266.local`.

### 4. Reset Wi-Fi Settings 🔄

To reset the device and clear the saved Wi-Fi credentials, press the reset button on the ESP8266. This will return the device to AP mode, allowing you to reconfigure the Wi-Fi settings.

### 5. LED Status 💡

The device uses the onboard LED to indicate the current status:

- **LED ON**: Connected to Wi-Fi.
- **LED BLINKING SLOW**: In AP mode and waiting for Wi-Fi credentials.
- **LED BLINKING FAST**: Attempting to connect to the Wi-Fi network.

## Code Structure 📝

The main logic of the project is divided into several functions:

- **`setup()`**: Initializes the device, checks for the reset pin press, and starts either the AP or Client mode based on the saved Wi-Fi credentials.
- **`startAPMode()`**: Starts the device in AP mode, allowing Wi-Fi configuration.
- **`startClientMode()`**: Attempts to connect to the configured Wi-Fi network in Client mode.
- **`handleRoot()`**: Displays the login page for Wi-Fi setup.
- **`handleSave()`**: Saves the configured Wi-Fi credentials to EEPROM.
- **`handleStatus()`**: Displays the current status of the device once it is connected to Wi-Fi.
- **`handleResetPage()`**: Clears the saved Wi-Fi credentials and resets the device.

## Files 📂

- **ESP8266WiFi.h**: Library for Wi-Fi communication.
- **ESP8266WebServer.h**: Provides functionality for handling HTTP requests.
- **DNSServer.h**: Allows DNS services in AP mode.
- **EEPROM.h**: Enables storing the Wi-Fi credentials in EEPROM.
- **ESP8266mDNS.h**: Provides Multicast DNS for accessing the device via `http://esp8266.local`.

## License 📄

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
