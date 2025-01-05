#include <WireGuard-ESP32.h>

// Sensor name
  const char* sensor = "SEN55-<SENSOR_ID>";     /* Sensor name SEN55-<SENSOR_ID> */
  const char* zone = "<PLANT_LOCATION>";        /* Location of the Plant */
// Sensor position
  const float longi = <LONGITUDE>>;               /* Longitide of the sensor */
  const float lat = <LATITUDE>>;                /* Latidude of the sensor */
// MQTT Topic - <SENSOR_MQTT_TOPIC>-SEN55-<SENSOR_ID>
  const char* topic = "<SENSOR_MQTT_TOPIC/SEN55_SENSOR_ID>";
// WireGuard configuration - no PresharedKey - dbl quotes will came from WG config file
  char private_key[] = "WIREGUARD_PRIVATE_KEY";
// WireGard sensor IP in the form W, X, Y, Z (comas + space instead of dots) e.g. (10, 1, 0, 0)
  IPAddress local_ip(WIREGUARD_SENSOR_IP);
