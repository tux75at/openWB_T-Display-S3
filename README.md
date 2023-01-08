# openWB_T-Display-S3
OpenWB status display using Lilygo T-Display-S3 based on ESP32-S3 and 1.9" LCD color Display.
Displays current EVU, PV and combined power of all charging ports plus SoC of charge port 1 [and if this charge port is plugged in (P) or charging (C). state of Plug is not implemented yet]

This code is based on the openWB_OLEDDisplay https://github.com/MartinRinas/openWB_OLEDDisplay and https://github.com/Xinyuan-LilyGO/T-Display-S3 (factory).

# Configuration
You need to enter SSID, PW and IP of openWB in [a relative link](include/config.h).

This is not finished yet, basic functions are implemented, the look of the GUI will be changed.
