// Define Blynk template ID, name, and authentication token
#define BLYNK_TEMPLATE_ID "TMPL6yZsq3MWN"
#define BLYNK_TEMPLATE_NAME "esp32"
#define BLYNK_AUTH_TOKEN "Dzp-i1Ef5FurNHz9rWUzCoSsfx7yTD3P"

// Include necessary libraries
#include <Wire.h>                     // For I2C communication
#include <LiquidCrystal_I2C.h>        // For LCD screen via I2C
#include <DHT.h>                      // For DHT temperature and humidity sensor
#include <WiFi.h>                     // For WiFi connection
#include <WiFiClient.h>               // For WiFi client services
#include <BlynkSimpleEsp32.h>         // For Blynk integration with ESP32

// WiFi credentials
char ssid[] = "Sm freeWIFI";          // WiFi SSID
char pass[] = "152439298m";           // WiFi password

// Define sensor and LED pin numbers
#define MQ4_PIN     34                // Analog pin connected to MQ-4 sensor (methane)
#define MQ135_PIN   35                // Analog pin connected to MQ-135 sensor (air quality)
#define DHT_PIN     15                // Digital pin connected to DHT11 sensor
#define RED_LED     26                // Red LED for spoiled indication
#define GREEN_LED   27                // Green LED for fresh indication

// Define DHT sensor type
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);            // Create DHT sensor object

// Initialize I2C LCD at address 0x27, with 16 columns and 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Define gas threshold values for spoilage
const int MQ4_THRESHOLD = 500;        // Threshold for methane level (MQ-4)
const int MQ135_THRESHOLD = 500;      // Threshold for air quality (MQ-135)

// Blynk timer object for periodic tasks
BlynkTimer timer;

// Flags to prevent repeated alerts
bool spoilageAlertSent = false;
bool dhtAlertSent = false;

// Function to read sensors and send data to Blynk and LCD
void sendToBlynk() {
  // Read temperature and humidity from DHT sensor
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();
  
  // Read analog values from MQ-4 and MQ-135 sensors
  int mq4Value = analogRead(MQ4_PIN);
  int mq135Value = analogRead(MQ135_PIN);

  // Send sensor values to Blynk virtual pins
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, humid);
  Blynk.virtualWrite(V2, mq4Value);
  Blynk.virtualWrite(V3, mq135Value);

  // Check if gas values exceed thresholds (spoiled)
  if (mq4Value > MQ4_THRESHOLD || mq135Value > MQ135_THRESHOLD) {
    digitalWrite(RED_LED, HIGH);         // Turn on red LED
    digitalWrite(GREEN_LED, LOW);        // Turn off green LED
    Blynk.virtualWrite(V4, "⚠️ Spoiled"); // Display spoilage warning on Blynk app

    // Send Blynk notification only once per spoilage event
    if (!spoilageAlertSent) {
      Blynk.logEvent("food_spoilage_detected", "⚠️ Food spoilage detected!");
      spoilageAlertSent = true;
    }

    Serial.println("⚠️ Food Spoilage Detected!"); // Debug output
  } else {
    digitalWrite(RED_LED, LOW);          // Turn off red LED
    digitalWrite(GREEN_LED, HIGH);       // Turn on green LED
    Blynk.virtualWrite(V4, "✅ Fresh");   // Indicate freshness on Blynk app

    // Reset spoilage alert flag
    if (spoilageAlertSent) {
      spoilageAlertSent = false;
    }

    Serial.println("✅ Food is Fresh.");  // Debug output
  }

  // Check for dangerous temperature or humidity and send alert
  if ((temp > 40 || humid > 80) && !dhtAlertSent) {
    Blynk.logEvent("dht11", "The surrounding can spoil your food");
    dhtAlertSent = true;
  }

  // Reset DHT alert if conditions return to safe levels
  if (temp <= 30 && humid <= 80) {
    dhtAlertSent = false;
  }

  // Display sensor values on LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);              // Show temperature with 1 decimal
  lcd.print("C H:");
  lcd.print(humid, 0);             // Show humidity with no decimals
  lcd.print("%  ");

  lcd.setCursor(0, 1);
  lcd.print("G1:");
  lcd.print(mq4Value);             // Show MQ-4 sensor value
  lcd.print(" G2:");
  lcd.print(mq135Value);           // Show MQ-135 sensor value
}

// Initial setup function
void setup() {
  Serial.begin(115200);            // Start serial communication at 115200 baud
  dht.begin();                     // Initialize DHT sensor
  lcd.init();                      // Initialize LCD
  lcd.backlight();                 // Turn on LCD backlight

  // Set LED pins as outputs
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // Show startup message on LCD
  lcd.setCursor(0, 0);
  lcd.print(" Spoil Alert!");
  lcd.setCursor(0, 1);
  lcd.print(" Connecting...");

  // Connect to WiFi and Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Clear LCD after connection
  lcd.clear();

  // Set a function to run every 2 seconds
  timer.setInterval(2000L, sendToBlynk);
}

// Main loop function
void loop() {
  Blynk.run();      // Run Blynk background tasks
  timer.run();      // Run scheduled timer functions
}

