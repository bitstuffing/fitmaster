# FitMaster: An Affordable Open-Source Reaction Light System
==============================================

FitMaster is a moderm DIY reaction light system; inspired by the reaction lights commonly used in fitness centers, senior rehabilitation centers, and popular in toy stores back in the 80s. 

FitMaster is designed to be highly customizable and scalable, allowing users to manage multiple devices from a web-based control panel.

## Features

* **Modular design**: Add or remove FitMaster devices to create a system that suits your needs
* **Connection**: Devices automatically connect to each other, with one device acting as the "master" and the others as "slaves"
* **Dashboard**: Configure exercises, adjust settings, and manage devices through a web interface


## Components (per module)

* ESP32-C3 microcontroller MCU
* ws2812&buzzer (6 neopixels) integrated module 
* Touch sensor
* 18650 battery
* PowerBoost charger

All components can be purchased on AliExpress, and the total cost of all components is less than 4€, making it an extremely affordable solution compared to other commercial products.

## Enclosure

The 3D printed enclosure design for FitMaster can be found on Onshape: https://cad.onshape.com/documents/81dda1dfedf65ecd9c30f590/w/bc1c4f0ca9940fdda67e0bc5/e/88cbd19db8e4cb3bbd0e881b?renderMode=0&uiState=67b0f53b7c46a20752be0c29. This design provides a compact and durable housing for the electronics, making it easy to assemble and use.

## Development

Currently FitMaster is being developed using PlatformIO, a popular platform for building and deploying IoT applications. The code is written in Arduino framework, making it easy to modify and customize.

## License

FitMaster is released under the GPLv3 license by @bitstuffing with love. 