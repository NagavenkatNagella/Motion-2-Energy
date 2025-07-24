#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>  // DHT library commented out

// WiFi credentials
const char* ssid = "Hellooo";
const char* password = "1234567 ";
//const char* apiKey = "1057d02cd694480f9a129d613535d8ef"; // Your OpenCage API key

// LCD setup (I2C address 0x27, 16 columns x 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);


 #define DHTPIN D3  // Changed from D3 (GPIO0) to D4 (GPIO2) for better reliability
 #define DHTTYPE DHT11
 DHT dht(DHTPIN, DHTTYPE);

// Touch sensor pins
#define TOUCH1 D5
#define TOUCH2 D6
#define TOUCH3 D7

// Web server
ESP8266WebServer server(80);

// Global variables
float temperature = 0.0;
float humidity = 0.0;
String statusMessage = "";
String lastStatus = "";

void setup() {
  Serial.begin(115200);

  // Initialize sensors (DHT code commented out)
   dht.begin();
  delay(2000);  // Wait for DHT to stabilize (if DHT is re-enabled)

  pinMode(TOUCH1, INPUT);
  pinMode(TOUCH2, INPUT);
  pinMode(TOUCH3, INPUT);

  // Start I2C LCD
  Wire.begin(D2, D1);  // SDA = D2, SCL = D1
  lcd.init();
  lcd.backlight();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  
  // Display "Welcome to VAWT" for 7 seconds
  lcd.setCursor(0, 0);
  lcd.print("Welcome to VAWT");
  delay(7000);  // Wait for 7 seconds before continuing

  // WiFi connected message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");

// Web page route with styling and content
server.on("/", []() {
    String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    
    // Enhanced styling with more animations and visual effects
    html += "<style>";
    html += "@import url('https://fonts.googleapis.com/css2?family=Montserrat:wght@300;400;600;700&display=swap');";
    
    // Core styling
    html += "body {font-family: 'Montserrat', sans-serif; margin: 0; padding: 0; min-height: 100vh; overflow-x: hidden;";
    html += "background: linear-gradient(135deg, #87CEEB, #1E90FF, #4169E1); background-size: 400% 400%; animation: gradientBG 15s ease infinite;}";
    
    // Animated background effects
    html += ".clouds {position: absolute; top: 0; left: 0; width: 100%; height: 100%; overflow: hidden; pointer-events: none; z-index: -1;}";
    html += ".cloud {position: absolute; width: 200px; height: 60px; background: rgba(255, 255, 255, 0.5); border-radius: 50px; filter: blur(5px); opacity: 0.6;}";
    html += ".cloud:nth-child(1) {top: 10%; left: -10%; animation: cloudDrift1 30s linear infinite;}";
    html += ".cloud:nth-child(2) {top: 30%; left: -20%; animation: cloudDrift2 45s linear infinite;}";
    html += ".cloud:nth-child(3) {top: 50%; left: -15%; animation: cloudDrift3 55s linear infinite;}";
    html += ".cloud:nth-child(4) {top: 70%; left: -5%; animation: cloudDrift4 40s linear infinite;}";
    
    // Container styling
    html += ".container {max-width: 1000px; margin: 0 auto; padding: 40px 20px; position: relative; z-index: 1;}";
    

    
    // Location card styling (unchanged)
    html += ".location-card {width: 100%; max-width: 400px; text-align: center; position: relative; overflow: hidden;}";
    html += ".location-title {color: #e6f7ff; font-size: 24px; margin-bottom: 10px; font-weight: 600;}";
    html += ".location-data {display: flex; flex-direction: column; gap: 15px;}";
    html += ".location-item {display: flex; justify-content: space-between; align-items: center; background: rgba(255, 255, 255, 0.1); border-radius: 12px; padding: 12px 15px;}";
    html += ".location-label {color: rgba(255, 255, 255, 0.8); font-size: 16px;}";
    html += ".location-value {color: #fff; font-weight: 600; font-size: 18px; text-shadow: 0 0 8px rgba(255, 255, 255, 0.4);}";
    html += ".location-loading {text-align: center; color: #fff; padding: 20px; font-style: italic;}";
    html += ".location-permission {background: rgba(0, 82, 204, 0.4); color: #fff; border: none; border-radius: 8px; padding: 12px 20px; font-family: 'Montserrat', sans-serif; font-weight: 600; cursor: pointer; margin-top: 15px; transition: all 0.3s ease; backdrop-filter: blur(4px);}";
    html += ".location-permission:hover {background: rgba(0, 102, 255, 0.6); transform: translateY(-2px);}";
    html += ".pulse-dot {display: inline-block; width: 12px; height: 12px; background: #4fc3f7; border-radius: 50%; margin-right: 8px; position: relative; top: 1px; animation: pulseDot 1.5s infinite;}";
    html += "@keyframes pulseDot {0% {transform: scale(0.8); opacity: 0.5;} 50% {transform: scale(1.2); opacity: 1;} 100% {transform: scale(0.8); opacity: 0.5;}}";

    // Header styling with enhanced animations
    html += ".header {text-align: center; padding: 20px; position: relative;}";
    html += "h1 {font-size: 50px; color: #fff; font-weight: 700; margin-bottom: 10px; text-shadow: 2px 2px 8px rgba(0, 0, 0, 0.3);";
    html += "animation: fadeInDown 1.5s ease, glowText 3s infinite alternate;}";
    html += "h2 {font-size: 28px; color: #e6f7ff; font-weight: 300; font-style: italic; margin-top: 0; text-shadow: 1px 1px 4px rgba(0, 0, 0, 0.2);";
    html += "animation: fadeInUp 1.8s ease;}";
    
    // Dashboard elements
    html += ".dashboard {display: flex; flex-wrap: wrap; justify-content: center; gap: 30px; margin-top: 40px;}";
    
    // Card styling with hover effects
    html += ".card {background: rgba(255, 255, 255, 0.15); backdrop-filter: blur(10px); border-radius: 20px; padding: 25px;";
    html += "box-shadow: 0 8px 32px rgba(0, 31, 63, 0.1); transition: all 0.3s ease; animation: fadeIn 2s ease, float 6s infinite ease-in-out;";
    html += "border: 1px solid rgba(255, 255, 255, 0.18);}";
    html += ".card:hover {transform: translateY(-10px); box-shadow: 0 15px 35px rgba(0, 31, 63, 0.2); background: rgba(255, 255, 255, 0.25);}";
    
    // Status card
    html += ".status-card {width: 100%; max-width: 400px; text-align: center;}";
    html += ".status-label {color: #e6f7ff; font-size: 24px; margin-bottom: 10px; font-weight: 600;}";
    html += ".status-value {color: #fff; font-size: 36px; font-weight: 700; text-shadow: 0 0 10px rgba(255, 255, 255, 0.5);";
    html += "animation: pulseGlow 2s infinite alternate; margin: 15px 0;}";
    
    // Measurements card with gauge-like visualization
    html += ".measurements-card {width: 100%; max-width: 400px;}";
    html += ".measurement {margin: 20px 0; position: relative;}";
    html += ".measurement-label {display: flex; justify-content: space-between; color: #e6f7ff; font-size: 18px; margin-bottom: 8px;}";
    html += ".measurement-value {font-weight: 600;}";
    html += ".progress-bar {height: 12px; background: rgba(255, 255, 255, 0.2); border-radius: 6px; overflow: hidden; position: relative;}";
    html += ".progress-fill-temp {height: 100%; border-radius: 6px; background: linear-gradient(90deg, #00ff9d, #00c3ff);";
    html += "animation: progressPulse 2s infinite alternate, shimmer 3s infinite linear;}";
    html += ".progress-fill-humid {height: 100%; border-radius: 6px; background: linear-gradient(90deg, #00c3ff, #9500ff);";
    html += "animation: progressPulse 2.5s infinite alternate, shimmer 3s infinite linear;}";
    
    // Quote section
    html += ".quote-section {margin: 40px 0; text-align: center;}";
    html += ".quote {font-size: 24px; color: #fff; font-style: italic; max-width: 800px; margin: 0 auto; padding: 20px;";
    html += "border-radius: 15px; background: rgba(255, 255, 255, 0.1); position: relative;";
    html += "animation: fadeIn 2.5s ease, pulse 4s infinite ease-in-out;}";
    html += ".quote::before, .quote::after {content: '\\201C'; position: absolute; font-size: 60px; opacity: 0.2; color: #fff;}";
    html += ".quote::before {top: 0; left: 10px;}";
    html += ".quote::after {content: '\\201D'; bottom: -20px; right: 10px;}";
    
    // Turbine animation
html += ".turbine-section {width: 100%; overflow: hidden; position: relative; height: 300px; margin: 50px 0; perspective: 1000px;}";
html += ".turbine-container {width: 100%; height: 100%; display: flex; justify-content: center; align-items: center; transform-style: preserve-3d; animation: turbineFloat 8s ease-in-out infinite;}";
html += ".turbine {width: 160px; height: 160px; position: relative; transform-style: preserve-3d; transform: rotateX(10deg);}";
html += ".turbine-center {width: 30px; height: 30px; background: linear-gradient(135deg, #fff, #a3d9ff); border-radius: 50%; position: absolute; top: 65px; left: 65px; z-index: 2;";
html += "box-shadow: 0 0 15px rgba(255, 255, 255, 0.8), 0 0 5px rgba(77, 195, 247, 0.8) inset;}";
html += ".turbine-blade {position: absolute; width: 120px; height: 20px; background: linear-gradient(90deg, rgba(255, 255, 255, 0.9), rgba(255, 255, 255, 0.7)); top: 70px; left: 20px;";
html += "transform-origin: 60px 10px; border-radius: 10px; box-shadow: 0 0 10px rgba(255, 255, 255, 0.5), 0 5px 15px rgba(0, 0, 0, 0.2);}";
html += ".turbine-blade:nth-child(1) {transform: rotate(0deg); animation: spinFast 3s linear infinite;}";
html += ".turbine-blade:nth-child(2) {transform: rotate(120deg); animation: spinFast 3s linear infinite;}";
html += ".turbine-blade:nth-child(3) {transform: rotate(240deg); animation: spinFast 3s linear infinite;}";
html += ".turbine-pole {width: 15px; height: 120px; background: linear-gradient(to bottom, #e6e6e6, #b3b3b3); position: absolute; bottom: -60px; left: 72px; z-index: 1;";
html += "border-radius: 8px 8px 0 0; box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.2);}";

/* Air flow animations */
html += ".air-flow {position: absolute; width: 100%; height: 100%; top: 0; left: 0; overflow: hidden; pointer-events: none;}";
html += ".air-particle {position: absolute; background: rgba(255, 255, 255, 0.4); border-radius: 50%; filter: blur(2px); opacity: 0; animation-name: airFlow; animation-timing-function: linear; animation-iteration-count: infinite;}";

/* Generate 20 different air particles with different sizes, positions and animations */
for (int i = 1; i <= 20; i++) {
    int size = 3 + (i % 5);  // Sizes between 3 and 7px
    int delay = i * 0.2;     // Staggered delays
    int duration = 3 + (i % 4);  // Durations between 3 and 6s
    int startY = 10 + (i * 15);  // Different vertical positions
    
    html += ".air-particle:nth-child(" + String(i) + ") {";
    html += "width: " + String(size) + "px; height: " + String(size) + "px;";
    html += "top: " + String(startY) + "%;";
    html += "animation-duration: " + String(duration) + "s;";
    html += "animation-delay: " + String(delay) + "s;";
    html += "}";
}

/* Related animations */
html += "@keyframes spinFast {0% {transform: rotate(0deg);} 100% {transform: rotate(360deg);}}";
html += "@keyframes turbineFloat {0% {transform: translateY(0) rotateX(0);} 50% {transform: translateY(-20px) rotateX(5deg);} 100% {transform: translateY(0) rotateX(0);}}";
html += "@keyframes airFlow {";
html += "0% {left: -50px; opacity: 0; transform: scale(0.5);}";
html += "10% {opacity: 0.7; transform: scale(1);}";
html += "90% {opacity: 0.7; transform: scale(1);}";
html += "100% {left: calc(100% + 50px); opacity: 0; transform: scale(0.5);}";
html += "}";

/* Responsive design for turbine */
html += "@media (max-width: 768px) {";
html += "  .turbine-section {height: 250px;}";
html += "  .turbine {width: 120px; height: 120px;}";
html += "  .turbine-center {top: 45px; left: 45px;}";
html += "  .turbine-blade {width: 100px; top: 50px;}";
html += "  .turbine-pole {left: 52px;}";
html += "}";
    
    // Footer
    html += "footer {text-align: center; padding: 20px; color: rgba(255, 255, 255, 0.8); font-size: 16px; margin-top: 40px;";
    html += "border-top: 1px solid rgba(255, 255, 255, 0.2); animation: fadeIn 3s ease;}";
    html += ".footer-logo {font-weight: 700; font-size: 18px; display: block; margin-bottom: 10px; letter-spacing: 1px;}";
    
    // Multiple new animations
    html += "@keyframes gradientBG {0% {background-position: 0% 50%;} 50% {background-position: 100% 50%;} 100% {background-position: 0% 50%;}}";
    html += "@keyframes fadeIn {0% {opacity: 0;} 100% {opacity: 1;}}";
    html += "@keyframes fadeInDown {0% {opacity: 0; transform: translateY(-30px);} 100% {opacity: 1; transform: translateY(0);}}";
    html += "@keyframes fadeInUp {0% {opacity: 0; transform: translateY(30px);} 100% {opacity: 1; transform: translateY(0);}}";
    html += "@keyframes pulse {0% {transform: scale(1);} 50% {transform: scale(1.03);} 100% {transform: scale(1);}}";
    html += "@keyframes pulseGlow {0% {text-shadow: 0 0 5px rgba(255, 255, 255, 0.5);} 100% {text-shadow: 0 0 20px rgba(255, 255, 255, 0.8), 0 0 30px rgba(0, 195, 255, 0.6);}}";
    html += "@keyframes glowText {0% {text-shadow: 0 0 5px rgba(255, 255, 255, 0.5);} 100% {text-shadow: 0 0 10px #fff, 0 0 20px #4fc3f7;}}";
    html += "@keyframes float {0% {transform: translateY(0px);} 50% {transform: translateY(-10px);} 100% {transform: translateY(0px);}}";
    html += "@keyframes spin {0% {transform: rotate(0deg);} 100% {transform: rotate(360deg);}}";
    html += "@keyframes progressPulse {0% {opacity: 0.8;} 100% {opacity: 1;}}";
    html += "@keyframes shimmer {0% {background-position: -200% 0;} 100% {background-position: 200% 0;}}";
    html += "@keyframes cloudDrift1 {0% {left: -10%;} 100% {left: 120%;}}";
    html += "@keyframes cloudDrift2 {0% {left: -20%;} 100% {left: 110%;}}";
    html += "@keyframes cloudDrift3 {0% {left: -15%;} 100% {left: 105%;}}";
    html += "@keyframes cloudDrift4 {0% {left: -5%;} 100% {left: 115%;}}";
    
    // Responsive design
    html += "@media (max-width: 768px) {";
    html += "  h1 {font-size: 32px;}";
    html += "  h2 {font-size: 22px;}";
    html += "  .status-value {font-size: 28px;}";
    html += "  .quote {font-size: 20px;}";
    html += "}";
    html += "</style>";
    
    // Body content with enhanced structure
    html += "</head><body>";
    
    // Moving clouds background effect
    html += "<div class='clouds'>";
    html += "<div class='cloud'></div>";
    html += "<div class='cloud'></div>";
    html += "<div class='cloud'></div>";
    html += "<div class='cloud'></div>";
    html += "</div>";
    

    
    // Header section
    html += "<div class='header'>";
    html += "<h1>VERTICAL AXIS WIND TURBINE</h1>";
    html += "<h2>Advanced Renewable Energy Solution</h2>";
    html += "</div>";
    
    // Animated turbine visual
 html += "<div class='turbine-section'>";
html += "<div class='turbine-container'>";
html += "<div class='turbine'>";
html += "<div class='turbine-blade'></div>";
html += "<div class='turbine-blade'></div>";
html += "<div class='turbine-blade'></div>";
html += "<div class='turbine-center'></div>";
html += "<div class='turbine-pole'></div>";
html += "</div>";
html += "</div>";

/* Air flow animation around turbine */
html += "<div class='air-flow'>";
for (int i = 1; i <= 20; i++) {
    html += "<div class='air-particle'></div>";
}
html += "</div>";
html += "</div>";
    
    // Dashboard section with cards
    html += "<div class='dashboard'>";
    
    // Status card
    html += "<div class='card status-card'>";
    html += "<div class='status-label'>System Status</div>";
    html += "<div class='status-value'>" + statusMessage + "</div>";
    html += "</div>";
    
    // Location card (new)
    html += "<div class='card location-card'>";
    html += "<div class='location-title'>Your Location</div>";
    html += "<div id='locationData' class='location-data'>";
    html += "<div class='location-loading'><span class='pulse-dot'></span>Fetching your location...</div>";
    html += "</div>";
    html += "</div>";
    
    // Measurements card with visual indicators
    html += "<div class='card measurements-card'>";
    html += "<div class='status-label'>Weather</div>";
    // Temperature with progress bar
    int tempPercent = map(constrain((int)temperature, 0, 40), 0, 40, 5, 95);
    html += "<div class='measurement'>";
    html += "<div class='measurement-label'><span>Temperature</span><span class='measurement-value'>" + String(temperature, 1) + "°C</span></div>";
    html += "<div class='progress-bar'><div class='progress-fill-temp' style='width: " + String(tempPercent) + "%'></div></div>";
    html += "</div>";
    
    // Humidity with progress bar
    int humidPercent = map(constrain((int)humidity, 0, 100), 0, 100, 5, 95);
    html += "<div class='measurement'>";
    html += "<div class='measurement-label'><span>Humidity</span><span class='measurement-value'>" + String(humidity, 1) + " %</span></div>";
    html += "<div class='progress-bar'><div class='progress-fill-humid' style='width: " + String(humidPercent) + "%'></div></div>";
    html += "</div>";
    
    html += "</div>"; // End measurements card
    html += "</div>"; // End dashboard
    
    // Inspirational quote section
    html += "<div class='quote-section'>";
    html += "<div class='quote'>Harnessing the power of the wind for a sustainable future, one revolution at a time.</div>";
    html += "</div>";
    
    // Footer with enhanced styling
    html += "<footer>";
    html += "<span class='footer-logo'>GreenFlow</span>";
    html += "Renewable Energy Innovations Monitoring System v2.0";
    html += "</footer>";
    
    html += "</div>"; // End container
    
    // JavaScript for  dropdown and geolocation
    html += "<script>";
    
   // Geolocation functionality to fetch and display real coordinates
html += "function getLocation() {";
html += "  const locationData = document.getElementById('locationData');";
html += "  locationData.innerHTML = \"<div class='location-loading'><span class='pulse-dot'></span> Fetching your location...</div>\";";
html += "  const apiKey = '1057d02cd694480f9a129d613535d8ef';";  // OpenCage API Key
html += "  fetch('http://ip-api.com/json/')";  // No https needed
html += "    .then(response => response.json())";
html += "    .then(data => {";
html += "      if (data.status === 'success') {";
html += "        const lat = data.lat;";
html += "        const lon = data.lon;";
html += "        locationData.innerHTML = `";
html += "          <div class='location-item'><span class='location-label'>Latitude</span><span class='location-value'>${lat}</span></div>";
html += "          <div class='location-item'><span class='location-label'>Longitude</span><span class='location-value'>${lon}</span></div>`;";
html += "        fetch(https://api.opencagedata.com/geocode/v1/json?q=${lat}+${lon}&key=${apiKey})";  // Use apiKey here
html += "          .then(response => response.json())";
html += "          .then(addressData => {";
html += "            if (addressData.status.code === 200 && addressData.results.length > 0) {";
html += "              const result = addressData.results[0];";
html += "              const city = result.components.city || result.components.town || result.components.village;";
html += "              const state = result.components.state || result.components.province;";
html += "              const country = result.components.country;";
html += "              const pincode = result.components.postcode || 'N/A';";
html += "              const street = result.components.road || 'N/A';";
html += "              locationData.innerHTML += `";
html += "                <div class='location-item'><span class='location-label'>City</span><span class='location-value'>${city}</span></div>";
html += "                <div class='location-item'><span class='location-label'>State</span><span class='location-value'>${state}</span></div>";
html += "                <div class='location-item'><span class='location-label'>Pincode</span><span class='location-value'>${pincode}</span></div>";
html += "                <div class='location-item'><span class='location-label'>Street</span><span class='location-value'>${street}</span></div>";
html += "              `;";
html += "            } else {";
html += "              locationData.innerHTML += <div style='color: #fff;'>Unable to fetch address from OpenCage</div>;";
html += "            }";
html += "          })";
html += "          .catch(err => {";
html += "            locationData.innerHTML += <div style='color: #fff;'>Error fetching address: ${err}</div>;";
html += "          });";
html += "      } else {";
html += "        locationData.innerHTML = <div style='color: #fff;'>Unable to fetch location via IP</div>;";
html += "      }";
html += "    })";
html += "    .catch(err => {";
html += "      locationData.innerHTML = <div style='color: #fff;'>Error fetching location: ${err}</div>;";
html += "    });";
html += "}";
html += "window.onload = getLocation;";

    html += "</script>";
    
    html += "</body></html>";
    
    // Send the enhanced page
    server.send(200, "text/html", html);
});


  server.begin();
}

void loop() {
  server.handleClient();

   
   humidity = dht.readHumidity();
   temperature = dht.readTemperature();

  // Commented out to prevent crashes if DHT is not present
   if (isnan(humidity) || isnan(temperature)) {
     Serial.println("Failed to read from DHT sensor!");
     return;  // Skip LCD update if reading failed
   }

  // Read touch sensor states
  bool t1 = digitalRead(TOUCH1) == HIGH;
  bool t2 = digitalRead(TOUCH2) == HIGH;
  bool t3 = digitalRead(TOUCH3) == HIGH;

  // Debugging output
  Serial.println("T1: " + String(t1) + " T2: " + String(t2) + " T3: " + String(t3));

  // Determine status message
  if (!t1 && !t2 && !t3) {
    statusMessage = "Everything fine";
  } else {
    statusMessage = "Threat at";
    if (t1) statusMessage += " 1";
    if (t2) statusMessage += " 2";
    if (t3) statusMessage += " 3";
  }

  // Only update LCD if message changed
  if (statusMessage != lastStatus) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(statusMessage.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print("T:" + String(temperature, 1) + " H:" + String(humidity, 1) + "%");
    lastStatus = statusMessage;
  }

  delay(1000);  // Reduced delay for better responsiveness
}