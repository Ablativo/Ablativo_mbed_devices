# Ablativo mbed device

This repo contains the code for the sensors/beacon. It is made to work with the **STM BSP_B-L475E-IOT01A1** board and tested using **mbed-os** version 5.

## Setup
Before compiling, remember to 
1. Setup `MQTT_server_setting.h` with your credentials;
2. Set the correct museum and device ID in `mbed_app.json` (unique for each device);
3. Set "WiFi SSID" and "WiFi Password" in `mbed_app.json`;
4. Look at [this great tutorial](https://os.mbed.com/users/coisme/notebook/aws-iot-from-mbed-os-device/) to understand how to setup AWS IoT Core;


## Libraries
* **BSP_B-L475e-IOT01A**: sensors drivers, in our case:
    - Capacitive digital sensor for relative humidity and temperature (HTS221)
    - 260-1260 hPa absolute digital output barometer (LPS22HB)
* **wifi-ism43362**: WiFi driver for the homonymous component (built in)
* **mbed-mqtt**: great MQTT library for mbed-os
* **TARGET_CORDIO_BLUENRG.lib**: BLE library

## Known issues
* Certificates, keys and passwords must be hardcoded => do not share compiled code, an attacker can easily reverse them
* For the moment, the ID requires manual setup on each device

### NOTE
The beacon service is not registered, we are using a random uuid.