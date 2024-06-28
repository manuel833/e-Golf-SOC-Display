
# Real-Time Battery Percentage Display for e-Golf and e-Up

This project provides a real-time display of the battery percentage for the Volkswagen e-Golf and e-Up electric vehicles.

## Features
- WIFI or Bluetooth Dongle
- Real-time battery percentage display
- Easy integration with e-Golf and e-Up

## Screenshots

![Screenshot 1](screenshot1.png)
![Screenshot 2](screenshot2.png)

## Support

If you find this project useful, you can support its development:

[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-FFDD00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://buymeacoffee.com/manuel833)

[Donate via PayPal](https://paypal.me/mherzog45?country.x=AT&locale.x=de_DE)

## Installation

### Requirements

- Arduino IDE
- Libraries: `ELMduino.h`, `TFT_eSPI`, `BluetoothSerial`,`WIFI`
- ELM327 Bluetooth or WIFI OBDII adapter
- [TTGO LoRa32-OLED ESP](https://www.amazon.de/gp/product/B09FYY8PBH/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

### Steps

1. Clone or download this repository:
   ```bash
   git clone https://github.com/manuel833/e-Golf-SOC-Display.git
   ```

2. Make the following changes in the code:
   ```cpp
   const char* DEVICE_NAME = "Golf"; // Gerätenamen des ESP
   const char* OBDII_NAME = "OBDII"; // OBDII Namen
   const char* BLUETOOTH_PIN = "1234"; // OBDII Bluetooth pin
   ```

3. Open the Arduino IDE and install the required libraries:
   - Go to **Sketch** -> **Include Library** -> **Manage Libraries...**
   - Search for `ELMduino` and install it
   - Search for `TFT_eSPI` and install it
   - Search for `BluetoothSerial` and install it

4. Upload the code to your TTGO LoRa32-OLED board.

5. Plug in your ELM327 Bluetooth OBDII adapter into your e-Golf or e-Up.

6. Power on the TTGO LoRa32-OLED display and it should connect to the OBDII adapter and start displaying the battery percentage.


## License

This project is licensed under the MIT License. See the `LICENSE.md` file for more details.
```

This version provides a clear and concise set of instructions for users to follow, ensuring they can easily set up and use the real-time battery percentage display for their e-Golf and e-Up vehicles.
