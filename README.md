# modem-myriota
Code to utilize the Myriota Developer Toolkit as a modem

## Environment Setup
https://developer.myriota.com/install-sdk

## Usage
### Real Satellites
```
cd src
make clean; make
updater.py -u -leuart-tracker.bin -s -l
```
### Satellite Simulator
```
cd src
make clean; SATELLITES=LabWithLocation make
updater.py -u -leuart-tracker.bin -s -l
```

## Sending Data
The Myriota module functions as a modem in this application.

Sensor/controller MCU should send data to the Myriota module using the LEUART pins.
```
┌────────────────┐                 ┌─────────────────┐
│                │                 │                 │
│                │                 │                 │
│     SENSOR     │                 │     MYRIOTA     │
│    PLATFORM    │                 │     MODULE      │
│       /        │                 │                 │
│      MCU       │                 │                 │
│                │                 │                 │
│            3.3V├─────────────────┤VEXT             │
│                │                 │                 │
│             GND├─────────────────┤GND              │
│                │                 │                 │
│              RX├─────────────────┤LEUART_TX        │
│                │                 │                 │
│              TX├─────────────────┤LEUART_RX        │
│                │                 │                 │
└────────────────┘                 └─────────────────┘
```
The modem expects to receive two bytes which are treated as a uint16_t. Each new data message is added to the `sensor_data` variable. 

Messages are sent using the `BeforeSatelliteTransmit` function to reduce latency between sensor readings and satellite transmission.