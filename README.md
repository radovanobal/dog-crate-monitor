# Project: Dog Crate Monitor
Dog Crate Monitor is a device being developed to monitor the environment inside a dog crate. My personal use case is to have a screen visible in my rearview mirror so I can check the temperature in the trunk where the dog crate is located. This helps me monitor conditions in the back of the car and adjust the AC as needed.

This is a project where I want to increase my experience in C development and create a working open-source tool that will actually help me (and hopefully others) when traveling with dogs.

Currently, the device displays temperature, humidity, and time. Eventually, I want to add features like flipping the screen so I can read the numbers from the rearview mirror. I also plan to include all the step files for the 3D-printed casing and mounting mechanisms for my dog crate.

In the future, I want to connect it to Home Assistant through a mobile hotspot and get notifications if the temperature goes higher than my automation settings. In summer, trunk temperatures can get uncomfortable quickly, and I want to be alerted before they do.

A distant goal is to broadcast the environmental data over BLE so I can display the temperature in Android Auto.

# Development Environment

Development is done in [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html) v5.5.3

# Hardware

This device is based on the [Waveshare ESP32-S3 3.97-inch e-paper display](https://www.waveshare.com/esp32-s3-epaper-3.97.htm). This hardware was chosen because it has extremely low power consumption and long battery life—my car doesn't have a 12V port in the trunk. Since our trips sometimes last many hours with minimal charging opportunities, long battery life is essential.

The board integrates the e-paper screen, temperature and humidity sensors, and all the connectivity features like BLE and Wi-Fi that this project needs—all in one neat package. Plus, it has incredibly low power consumption, which is perfect for a device that potentially needs to run for days on a single charge.






