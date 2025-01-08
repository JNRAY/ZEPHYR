
# Zéphyr XIAO-C3 code installation and Download

## Basic software installation

### Install Vscode

Go to [https://code.visualstudio.com/]() and download the stable build for your operating system.

Launch Vscode.

### Instam PlatforIO extension on Vscode

In Vscode Extension seache and load PlartforIO IDE

Download XIAO-ESP32-C3 source code folder and copy in your conputer, open it from PIO Home/Open Project Quick Acess Menu.

## Zéphyr code configuration

You need to replace the values betxeen < > for:

### Server Configuration


From line 64 to 96 you need to update the folloving variables depending on your server configuration.

Name of the .h include file:

`<SEN55_SENSOR_ID>` (e.g. SEN55-001 or SCD30-001 if CO2 captor is installed)

Wireguard Configuration:

`public_key`&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`<WIREGUARD_PUBLIC_KEY>`  
`endpoint_address` `<YOUR_DOMAIN>`  
`endpoint_port` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`<WIREGUARD_PORT>`

MQTT configuration:

`mqtt-Server` &nbsp;&nbsp;&nbsp;`<MQTT_SERVEUR_IP>`  
`mqttPort `&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`<MQTT_SERVEUR_PORT>`  
`mqttUser` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`<MQTT_USER>`  
`mqttPassword` &nbsp;`<MQTT_PASSWORD>`

### Include .h file configuration for each captor

There is one .h include file for each sensor:

Sensor name and plant location:

`sensor` `SEN55-<SENSOR_ID>` (e.g. SEN55-001 or SCD30-001 if CO2 captor is installed)  
`zone` &nbsp;&nbsp;&nbsp;&nbsp;`<PLANT_LOCATION>` (Several plant location could be used if needed)

Sensor position (Used for the cartography in Grafana):

`longi` `<LONGITUDE>` of the Sensor  
`lat` &nbsp;&nbsp;&nbsp;&nbsp;`<LATITUDE>` of the Sensor

Mqtt Topic:

`topic` `<SENSOR_MQTT_TOPIC/SEN55-SENSOR_ID>`D (e.g. Environement/SEN55-001)

Wireguard configuration:

`privatr_key` `<WIREGUARD_PRIVATE_KEY>`  
`local_ip` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`<WIREGUARD_SENSOR_IP>`

### Other configurations

From line 30 to 63 there is 4 viriables (See comment for definition):

`DEBUG`	&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 or 1  
`SEND_DATA` &nbsp;&nbsp;0 or 1  
`WIFI_RESET` 0 or 1  
`SDC30` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0 or 1


### Upload Code

[See GitBook site:](https://rhizobiome.gitbook.io/atrosca-degaze/le-capteur-zephyr/montage)



