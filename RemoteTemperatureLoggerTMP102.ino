/********************************************************************************
   RemoteTemperatureLogger.ino

   Written by: B. Huang (brian@sparkfun.com)
   Please let me know if you have problems with this code.

   Reads temperature and pressure data and posts this to data.sparkfun.com
   useing SparkFun ESP8266 Thing Dev, BMP180, and Phant.
 ********************************************************************************/

// Include the ESP8266 WiFi library. (Works a lot like the Arduino WiFi library.)
#include <ESP8266WiFi.h>

// Include the SparkFun Phant library.
#include "Phant.h"

// Include the TMP102 library and the wire library for the sensor
#include "SparkFunTMP102.h"
#include <Wire.h>

//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "TP-LINK_FFC0";
const char WiFiPSK[] = "71845968";

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED

////////////////
// Phant Keys //
////////////////
const char PhantHost[] = "data.sparkfun.com";
const char PublicKey[] = "enter public key here";
const char PrivateKey[] = "enter private key here";
// delete key: 
// fields: time,data

/////////////////
// Post Timing //
/////////////////
const unsigned long postRate = 30000;  //rate to post data in milliseconds
unsigned long lastPost = 0;
unsigned long ndx = 0;
unsigned long postingTime;

TMP102 tempSensor(0x48); // Initialize sensor at I2C address 0x48
float tempF; //global variables used by bmp180

/********************************************************************************/
void setup()
{
  Serial.begin(9600);
  Serial.println("Starting Now!");
  pinMode(LED_PIN, OUTPUT);
  blink(2);
  digitalWrite(LED_PIN, LOW);
  tempSensor.begin();  //initialize the tmp102 sensor

  // set the Conversion Rate (how quickly the sensor gets a new reading)
  //0-3: 0:0.25Hz, 1:1Hz, 2:4Hz, 3:8Hz
  tempSensor.setConversionRate(2);

  //set Extended Mode.
  //0:12-bit Temperature(-55C to +128C) 1:13-bit Temperature(-55C to +150C)
  tempSensor.setExtendedMode(0);

  blink(3);  //three quick blinks at the beginning indicates successful init of BMP
  delay(500);
  Serial.print("Connecting to WiFi");
  connectWiFi();
  digitalWrite(LED_PIN, HIGH); //light on indicates successful connection to WiFi
}

/********************************************************************************/
void loop()
{
  if (lastPost + postRate <= millis())
  {

    // Turn sensor on to start temperature measurement.
    // Current consumtion typically ~10uA.
    tempSensor.wakeup();

    // read temperature data
    tempF = tempSensor.readTempF();
    // Place sensor in sleep mode to save power.
    // Current consumtion typically <0.5uA.
    tempSensor.sleep();
    blink(1);  //quick blinks indicates a successful data read
    delay(100);
    if (postToPhant())
      lastPost = millis();
    else
      delay(5000);  // wait 5 seconds before re-trying
  }
}

/********************************************************************************/
void blink(int numTimes)
{
  for (int x = 0; x < numTimes; x++)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}
/********************************************************************************/
int postToPhant()
{
  //LED turns on when we enter, it'll go off when we
  //successfully post.
  digitalWrite(LED_PIN, HIGH);

  postingTime = millis() / 1000;
  //Declare an object from the Phant library - phant
  Phant phant(PhantHost, PublicKey, PrivateKey);

  //Do a little work to get a unique-ish name. Use the
  //last two bytes of the MAC (HEX) as ID
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);

  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  
  // Add the four field/value pairs defined by our stream:
  //phant.add("id", macID);
  //phant.add("n", ndx);
  phant.add("time", postingTime);
  phant.add("data", tempF);

  //Debug print out data to Serial
  Serial.println("Posting!");
  Serial.print(macID); //id
  Serial.print("\t");  //tab character
  Serial.print(ndx);   //data point index 
  Serial.print("\t");  //tab character
  Serial.print(tempF); //temperature in degF
  Serial.print("\t");  //tab character
  Serial.println(postingTime);  //posting time in seconds

  ndx++;

  // Connect to data.sparkfun.com, and post our data:
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(PhantHost, httpPort))
  {
    // If we fail to connect, return 0.
    return 0;
  }
  // If we successfully connected, print our Phant post:
  client.print(phant.post());

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
  }

  // Before we exit, turn the LED off.
  blink(3); //two successive blinks indicates success
  return 1; // Return success
}
/********************************************************************************/
void connectWiFi()
{
  byte ledStatus = LOW;

  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
}
