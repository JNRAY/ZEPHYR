/*
  Programme Envirennement - SEN55-XXX
  Author: Jean Noël Raynaud
  Version     Date        Modification

  V1        30/03/2024    Création of final version
  V2        21/05/2024    Minor update for DEBUG
  V3        16/06/2024    Integration WiFiManagement librry
  V4        26/06/2024    Integration include file for vaiables
  V5        25/07/2024    Modification IP adress of MQTT Server 
  V6        28/07/2024    Integration of SCD30 C02 captor
  V7        28/01/2025    Adaption for WiFi connection router with only WPA(TKIP/AES) protocol and addition
                          of timeout for WifiManager AutoConnectAP

  Copywrigt: 
    For SEN55 code part: Copyright (c) 2021, Sensirion AG under the BSD 3-Clause License
    For the other part: Copyright (c) 2024, Zéphir - LSB under the CC BY-NC-SA 4.0 License
*/

// Librairies

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SensirionI2CSen5x.h>
#include <SensirionCore.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <SensirionI2cScd30.h>
#include <WireGuard-ESP32.h>
#include "time.h"
#include "math.h"


// ******************************************************************************
// Variable DEBUG put:
//    0 if production
//    1 if test or coding mode to see the Serial.print information in the monitor
// ******************************************************************************

#define DEBUG 0

// *********************************************************************************
// Variable SEND_DATA put:
//    0 Data set sent every hour and 1/2 hour. Synchronised whit ntp server
//    1 Data set sent every 5 mins 300000 Milliseconds and we remove 6.25 secondes
//      to integrate calculation time
// *********************************************************************************

#define SEND_DATA 1
const int send_data = 300000-6250;

// *********************************************************************************
// Variable WIFI_RESET put:
//    0 To keep current stored WIFI SSID and PASSWORD
//    1 To reset current stored WIFI SSID and PASSWORD You will need to upload the
//    code again with 0 to keep your stored WIFI SSID and PASSWORD
// *********************************************************************************

#define WIFI_RESET 0

// *********************************************************************************
// Variable SCD30 C02 Captor put:
//    0 If SCD30 Captor is not installed
//    1 If SCD30 Captor is installed
// *********************************************************************************

#define SCD30 0

// *********************************************************************************
// All paremeters between < > bellow must be replaced by your own serveur parameters
// *********************************************************************************

    // **********************************************************************
    // Variables to be updated for each sensor inside the SEN55-<SENSOR_ID>.h
    // files located in the include directory
    // **********************************************************************

    #include <SEN55-<SENSOR_ID>.h>

    // *****************************************************************
    // Variables to be updated depending of your server environnment
    // *****************************************************************
      
    // WireGuard configuration

      char public_key[] = "<WIREGUARD_PUBLIC_KEY>";
      char endpoint_address[] = "vpn.<YOUR_DOMAIN>";  // IP of Wireguard endpoint to connect to.
      int endpoint_port = <WIREGUARD_PORT>;

      static WireGuard wg;

    // MQTT server connection variables

      const char* mqttServer = "<MQTT_SERVEUR_IP>";
      const int mqttPort = <MQTT_SERVEUR_PORT>;
      const char* mqttUser = "<MQTT_USER>";
      const char* mqttPassword ="<MQTT_PASSWORD>";

// ********************************************************************************
// End of parameters to be updated depending of your serveor parameters
// ********************************************************************************

// Retention of messages true or false - if false no message sent
  
  boolean retained;

// WiFi and MQTT client configuration

  WiFiClient espClient;
  PubSubClient client(espClient);

// NTP server variable for Local Time to be updated depending of your location

  const char* ntpServer = "fr.pool.ntp.org";
  const char* localTimeZone = "CET-1CEST,M3.5.0,M10.5.0/3";

// *********************************************************************
// End Variables to be updated depending of your server environnment
// Do not modify the code below
// *********************************************************************

// Sensirrion SEN55

// The commands used use up to 48 bytes. On some Arduinos,
// the default buffer space is not large enough

#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) &&                 \
     (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
     (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

SensirionI2CSen5x sen5x;

// SCD30

SensirionI2cScd30 sensor2;

static char errorMessage[128];
static int16_t error;

// FUnction print Local Time

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Fonstion printLocalTime: Impossible d'obtenir l'heure");
    return;
  }
  Serial.print("Date et heure: ");
  Serial.println(&timeinfo, "%d/%m/%Y %H:%M:%S");
}

// FUnction reboot

void restartEps32(String error_message){
  if (DEBUG) {
    Serial.println();
    Serial.println("Restart ESP32");
    Serial.println(error_message);
    Serial.println();
  }
  ESP.restart();
}

// Function printModuleVersions

void printModuleVersions() {
  uint16_t error;
  char errorMessage[256];

  unsigned char productName[32];
    uint8_t productNameSize = 32;

    error = sen5x.getProductName(productName, productNameSize);

  if (error) {
    Serial.print("SEN-55 Erreur getProductName(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("SEN-55 ProductName:");
    Serial.println((char*)productName);
  }

  uint8_t firmwareMajor;
  uint8_t firmwareMinor;
  bool firmwaredebug;
  uint8_t hardwareMajor;
  uint8_t hardwareMinor;
  uint8_t protocolMajor;
  uint8_t protocolMinor;

  error = sen5x.getVersion(firmwareMajor, firmwareMinor, firmwaredebug,
                            hardwareMajor, hardwareMinor, protocolMajor,
                            protocolMinor);
  if (error) {
    Serial.print("SEN-55 Erreur getVersion(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("SEN-55 Firmware: ");
    Serial.print(firmwareMajor);
    Serial.print(".");
    Serial.print(firmwareMinor);
    Serial.print(", ");
    Serial.print("Hardware: ");
    Serial.print(hardwareMajor);
    Serial.print(".");
    Serial.println(hardwareMinor);
  }
}

// Function printSerialNumber

void printSerialNumber() {
  uint16_t error;
  char errorMessage[256];
  unsigned char serialNumber[32];
  uint8_t serialNumberSize = 32;

  error = sen5x.getSerialNumber(serialNumber, serialNumberSize);
  if (error) {
    Serial.print("SEN-55 Erreur getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("SEN-55 SerialNumber:");
     Serial.println((char*)serialNumber);
  }
}

// *****
// Setup
// *****

void setup() {

  Serial.begin(115200);

  // while (!Serial){} // Do not use for XIAO cart

  delay(1000);

  if (DEBUG) {
    Serial.println();
    Serial.println("-------------------------");
    Serial.println("Début setup");
    Serial.println("-------------------------");
  }

  // SEN55

  Wire.begin();

  sen5x.begin(Wire);

    uint16_t error;
    char errorMessage[256];
    error = sen5x.deviceReset();

  if (DEBUG) {

    if (error) {("SEN-55 Erreur deviceReset(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
    }

    printSerialNumber();
    printModuleVersions();

  }

  // Set a temperature offset in degrees Celsius
  // Note: supported by SEN54 and SEN55 sensors
  // By default, the sensor temperature and humidity outputs
  // are compensated for the self-heating of the modules. If the module is
  // contained in a box, temperature compensation may require
  // to be adapted to integrate the change in thermal coupling and
  // self-heating of other components of the device.
  //
  // A guide to achieving peak performance, including benchmarks
  // Examples of mechanical design can be found in the application note
  // “SEN5x – Temperature compensation instruction” on www.sensirion.com.
  // Please refer to these application notes for more information
  // on the advanced compensation parameters used
  // in `setTemperatureOffsetParameters`, `setWarmStartParameter` and
  // `setRhtAccelerationMode`.
  //
  // Adjust tempOffset to account for additional temperature offsets
  // self-heating of the SEN module exceeded.

  float tempOffset = 0.0;
  error = sen5x.setTemperatureOffsetSimple(tempOffset);
  if (DEBUG) {
    if (error) {
      Serial.print("SEN-55 Erreur setTemperatureOffsetSimple(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
    } else {
      Serial.print("SEN-55 Temperature Offset set to ");
      Serial.print(tempOffset);
      Serial.println(" deg. Celsius (SEN54/SEN55 only");
    }
  }

  // Start Measurement

  error = sen5x.startMeasurement();

  if (DEBUG) {
    if (error) {
      Serial.print("SEN-55 Erreur startMeasurement(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
    }
    Serial.println("Start SEN55 Measurement");
    Serial.println();
  }

  // SCD30

  if (SCD30) {
    Serial.setDebugOutput(true);
    Wire.begin();
    sensor2.begin(Wire, SCD30_I2C_ADDR_61);

    if (DEBUG) {
      Serial.print("SCD30 I2C Adress: ");
      Serial.println(SCD30_I2C_ADDR_61);
    }

    sensor2.stopPeriodicMeasurement();
    sensor2.softReset();
    delay(2000);
    uint8_t major = 0;
    uint8_t minor = 0;
    error = sensor2.readFirmwareVersion(major, minor);
    if (DEBUG) {
      if (error != NO_ERROR) {
          Serial.print("SDC 30 Error trying to execute readFirmwareVersion(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          Serial.println(errorMessage);
          return;
      }
      Serial.print("SCD30 firmware version major: ");
      Serial.print(major);
      Serial.print("\t");
      Serial.print("minor: ");
      Serial.print(minor);
      Serial.println();
      error = sensor2.startPeriodicMeasurement(0);
      if (error != NO_ERROR) {
          Serial.print("SCD30 Error trying to execute startPeriodicMeasurement(): ");
          errorToString(error, errorMessage, sizeof errorMessage);
          Serial.println(errorMessage);
          return;
      }
    }
  }

  // Wifi connexion

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  if (DEBUG) {
    Serial.println();
    Serial.println("Debut setup WiFi");
    Serial.println();
  }
    
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  
  WiFiManager wm;

  // For WiFi router with only WPA(TKIP/AES) protocol

  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);

  // Set time out fot WiFiManager AutoConnectAP portal 60 second

  wm.setConfigPortalTimeout(60);
 
  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  
  if (WIFI_RESET) {
    if (DEBUG) {
      Serial.println("Reset WiFi SSID and Password");
      Serial.println();
    }
    wm.resetSettings();
  }

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result
 
  bool res;

  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

  res = wm.autoConnect("AutoConnectAP");
  
  if(!res) {
    if (DEBUG) {
      Serial.println();
      Serial.println("Failed to connect to WiFI");
      Serial.println("EPS Restart");
      Serial.print("res = ");
      Serial.println(res);
      Serial.println();
    }
    ESP.restart();
  } 
  else {
    //if you get here you are connected to the WiFi
    if (DEBUG) {
      Serial.println();   
      Serial.print("Connected to WiFi network: ");
      Serial.print("Local ESP32 IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("res = ");
      Serial.println(res);
      Serial.println();
    }
  }

  // WireGuard connection

  if (DEBUG) {
    Serial.println();
    Serial.println("Start WireGuard Connexion");
    Serial.println("Adjusting system time...");
  }
  
  configTzTime(localTimeZone, ntpServer);
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    if (DEBUG) {
      Serial.println("impossible to get the time");
    }
  }

  if (DEBUG) {
    Serial.print("Date et heure: ");
    Serial.println(&timeinfo, "%d/%m/%Y %H:%M:%S");
    Serial.println();
    Serial.println("Connected. Initializing WireGuard...");
    Serial.print("Local_ip: ");
    Serial.println(local_ip);
    Serial.print("Private_key: ");
    Serial.println(private_key);
    Serial.print("Endpoint_address: ");
    Serial.println(endpoint_address);
    Serial.print("Public_key: ");
    Serial.println(public_key);
    Serial.print("Endpoint_port: ");
    Serial.println(endpoint_port);
  }

  if (wg.begin(local_ip,private_key,endpoint_address,public_key,endpoint_port)) {
    if (DEBUG) {
      Serial.println("WireGuard Connection OK");
    }
  } else {
    if (DEBUG) {
      Serial.println("WireGuard Connection FAIL");
    }
  }

  if (DEBUG) {
    Serial.println();
    Serial.println("-------------------------");
    Serial.println("End setup");
    Serial.println("-------------------------");
    Serial.println();
  }
}

// ****
// Loop
// ****

void loop() {

  if (DEBUG) {
    Serial.println("-------------------------");
    Serial.println("Start loop");
    Serial.println("-------------------------");
  }

  String error_message;

  // We configure the ntp server and retrieve the time

  configTzTime(localTimeZone, ntpServer);
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    if (DEBUG) {
      Serial.println("impossible to get the time");
    }
  }
  
  if (DEBUG) {
    printLocalTime();
    Serial.println();
  }
  
  if(SEND_DATA) {
    // 1 data set sent every 5 minutes = 300000 Milliseconds
    if (DEBUG) {
        Serial.println ("SEND_DATA = 1");
        Serial.println ();
      }
  } else {
    // 1 data set sent every hour and 1/2 hour

    int minutes = timeinfo.tm_min;
    int secondes = timeinfo.tm_sec;

    if (DEBUG) {
      Serial.println ("SEND_DATA = 0");
      Serial.println ();
      Serial.println ("minutes = " + String(minutes) + " seconds = " + String(secondes));
    }

    if (minutes == 0) {
      if (DEBUG) {
        Serial.println ("minutes = 0 minutes");
      }
    } else if (minutes < 30) {
      minutes = (30 - minutes);
        if (DEBUG) {
          Serial.println ("minutes < ou = 30 minutes = " + String(minutes));
        }
    } else if (minutes == 30) {
      minutes = 0;
      if (DEBUG) {
        Serial.println ("minutes = 0 minutes");
      }
    } else if (minutes > 30) {
      minutes = (60 - minutes);
      if (DEBUG) {
        Serial.println ("minutes > 30 minutes = " + String(minutes));
      }
    } 

    if (minutes > 0) {
      if (DEBUG) {
        Serial.println("We wait = " + String(minutes) + " minutes - " + String(secondes) + " secondes and delay = " + String((minutes * 60000) - ((secondes * 1000) + 2000)) + " millisecondes");
        printLocalTime();
        Serial.println();
      }
      // We add 2 secondes = 2000 millisceondes to integrate calculation time
      delay((minutes * 60000) - ((secondes * 1000) + 2000));
    } else {
      if (DEBUG) {
        Serial.println("Minutes = 0, we continue the program");
        printLocalTime();
        Serial.println();
      }
    }
  }

  // SEN55

  uint16_t error;
  char errorMessage[256];

  delay(1000);

  // Read Measurement

  float massConcentrationPm1p0;
  float massConcentrationPm2p5;
  float massConcentrationPm4p0;
  float massConcentrationPm10p0;
  float ambientHumidity;
  float ambientTemperature;
  float vocIndex;
  float noxIndex;

  error = sen5x.readMeasuredValues(
  massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
  massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
  noxIndex);

  if (error) {
    if (DEBUG) {
      Serial.print("Erreur readMeasuredValues(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
    }
    delay(2000);
    String error_message = "";
    restartEps32(error_message);
  } else {
    if (DEBUG) {
      Serial.println("Données Capteur SEN55: ");
      printLocalTime();
      Serial.print("MassConcentrationPm1p0:");
      Serial.print(massConcentrationPm1p0);
      Serial.println();
      Serial.print("MassConcentrationPm2p5:");
      Serial.print(massConcentrationPm2p5);
      Serial.println();
      Serial.print("MassConcentrationPm4p0:");
      Serial.print(massConcentrationPm4p0);
      Serial.println();
      Serial.print("MassConcentrationPm10p0:");
      Serial.print(massConcentrationPm10p0);
      Serial.println();
      Serial.print("AmbientHumidity:");
      if (isnan(ambientHumidity)) {
        Serial.print("n/a");
      } else {
        Serial.print(ambientHumidity);
      }
      Serial.println();
      Serial.print("AmbientTemperature:");
      if (isnan(ambientTemperature)) {
        Serial.print("n/a");
      } else {
        Serial.print(ambientTemperature);
      }
      Serial.println();
      Serial.print("VocIndex:");
      if (isnan(vocIndex)) {
        Serial.print("n/a");
      } else {
        Serial.print(vocIndex);
      }
      Serial.println();
      Serial.print("NoxIndex:");
      if (isnan(noxIndex)) {
        Serial.println("n/a");
      } else {
        Serial.println(noxIndex);
      }
    }
  }

  // SCD30

  float co2Concentration = 0.0;
  float temperature = 0.0;
  float humidity = 0.0;

  if (SCD30) {
    delay(1500);
    error = sensor2.blockingReadMeasurementData(co2Concentration, temperature, humidity);
    if (error != NO_ERROR) {
      if (DEBUG) {
      Serial.print("SCD30 Error trying to execute blockingReadMeasurementData(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
      }
      delay(2000);
      String error_message = "";
      restartEps32(error_message);
    } else {
      if (DEBUG) {
        Serial.println();
        Serial.println("Données Capteur SCD30:");
        Serial.print("co2Concentration: ");
        Serial.println(co2Concentration);
      }
    }
  }

  // JSON File

  if (DEBUG) {
    Serial.println();
    Serial.println("Start JSON");
  }

  StaticJsonDocument<700> doc;
  
  doc["Sensor"] = sensor;
  doc["Zone"] = zone;
  doc["Long"] = longi;
  doc["Lat"] = lat;
  doc["Pm1p0"] = massConcentrationPm1p0;
  doc["Pm2p5"] = massConcentrationPm2p5;
  doc["Pm4p0"] = massConcentrationPm4p0;
  doc["Pm10p0"] = massConcentrationPm10p0;
  doc["Humidity"] = ambientHumidity;
  doc["Temperature"] = ambientTemperature;
  doc["VOC"] = vocIndex;
  doc["NOx"] = noxIndex;
  if (SCD30) {
    doc["CO2"] = co2Concentration;
  }

  if (DEBUG) {
    Serial.println("End JSON");
    Serial.println();
  }

  // MQTT connexion

  client.setServer(mqttServer, mqttPort);

  while (!client.connected()) {
    if (DEBUG) {
      Serial.println("Connection to the MQTT server...");
    }
  
    if (client.connect(sensor , mqttUser, mqttPassword)) {
      if (DEBUG) {
        Serial.println("MQTT server connected");
      }
    } else {
      if (DEBUG) {
        Serial.print("MQTT connection failure with status: ");
        Serial.println(client.state());
        Serial.print("mqttserver: ");
        Serial.println(mqttServer);
        Serial.print("mqttPort: ");
        Serial.println(mqttPort);
        Serial.print("sensor: ");
        Serial.println(sensor);
        Serial.print("mqttUser: ");
        Serial.println(mqttUser);
        Serial.print("mqttPassword: ");
        Serial.println(mqttPassword);

      }
      delay(2000);
      String error_message = "";
      restartEps32(error_message);
    }
  }

  // Send MQTT message

  if (DEBUG) {
    Serial.print("Send message to MQTT topic server: ");
    Serial.println(topic);
  }

  client.beginPublish(topic, measureJson(doc), retained);
  if (DEBUG) {
    Serial.println("Start publication");
  }

  serializeJson(doc, client);

  if (DEBUG) {
    Serial.println("serializeJson");
   }

  boolean rc = client.endPublish();

  delay(5000);

  if (DEBUG) {
    if (rc == true) {
      Serial.print("Success sending MQTT message - ");
      printLocalTime();
      Serial.println();
    }
  }

  if (rc == false) {
    if (DEBUG) {
      Serial.println(error_message);
    }
    String error_message = "";
    restartEps32(error_message);
  }
    
  // Serial.print ficher JSON
  
  if (DEBUG) {
    char buffer[700];
    serializeJsonPretty(doc, buffer);
    Serial.println(buffer);
  }

  // Disconnect mqtt server

  void disconned();

  if (DEBUG) {
    Serial.println();
    Serial.print("MQTT server disconnected - ");
      printLocalTime();
  }
    
  // Wait 30 mins 1800000 25 mins 1500000 15 mins 900000 Milliseconds 5 mins 300000 Milliseconds 1 min 60000 Milliseconds

  if(SEND_DATA) {

    if (DEBUG) {
      Serial.println();
      Serial.println("---------------------------------------------");
      Serial.println("End of loop, SEND_DATA = 1, we wait 5 Minutes");
      Serial.println("---------------------------------------------");
      Serial.println();
    }
    delay(send_data);
  } else {
    if (DEBUG) {
      Serial.println();
      Serial.println("----------------------------------------------");
      Serial.println("End of loop, SEND_DATA = 0, we wait 25 minutes");
      Serial.println("----------------------------------------------");
      Serial.println();
    }
    delay(1500000);
  }

}
