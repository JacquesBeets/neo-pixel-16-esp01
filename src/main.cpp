#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <ESP8266HTTPUpdateServer.h>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const uint16_t PixelCount = 16;
const uint8_t PixelPin = 2;
const uint8_t AnimationChannels = 1;

NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(AnimationChannels);
boolean fadeToColor = true;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};

MyAnimationState animationState[AnimationChannels];

float currentBrightness = 0.35f;
bool animationEnabled = false;  // Start with animation disabled


RgbColor currentColor = RgbColor(255, 100, 0); 

void SetRandomSeed()
{
    uint32_t seed = analogRead(0);
    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }
    randomSeed(seed);
}

void BlendAnimUpdate(const AnimationParam& param)
{
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);

    updatedColor = RgbColor::LinearBlend(RgbColor(0, 0, 0), updatedColor, currentBrightness);

    for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
    {
        strip.SetPixelColor(pixel, updatedColor);
    }
}

void FadeInFadeOutRinseRepeat()
{
    if (fadeToColor)
    {
        RgbColor target = HslColor(random(360) / 360.0f, 1.0f, 0.5f);
        uint16_t time = random(5000, 10000);  

        animationState[0].StartingColor = strip.GetPixelColor<RgbColor>(0);
        animationState[0].EndingColor = target;

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }
    else 
    {
        uint16_t time = random(3000, 5000); 

        animationState[0].StartingColor = strip.GetPixelColor<RgbColor>(0);
        animationState[0].EndingColor = RgbColor(0);

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }

    fadeToColor = !fadeToColor;
}

void handleRoot() {
    String html = "<html>";
    html += "<head><title>LED LAMP NEOPIXEL</title></head>";
    html += "<body><h1>NeoPixel Control</h1>";
    html += "<p><a href='/ota_update'>Update Firmware</a></p>";
    html += "<p><a href='/toggle'>Toggle Animation</a></p>";
    html += "<p>Brightness: <a href='/brightness?value=0.1'>Low</a> | <a href='/brightness?value=0.4'>Medium</a> | <a href='/brightness?value=0.7'>High</a> | <a href='/brightness?value=1.0'>Maximum</a></p>";
    html += "<p><a href='/setcolor?color=red'>Red</a> | <a href='/setcolor?color=green'>Green</a> | <a href='/setcolor?color=blue'>Blue</a> | <a href='/setcolor?color=white'>White</a> | <a href='/setcolor?color=warmwhite'>Warm White</a></p>";
    html += "<p><a href='/off'><strong>SWITCH OFF</strong></a></p>";
    html += "<form action='/setcustomcolor' method='get'>";
    html += "R: <input type='number' name='r' min='0' max='255' required>";
    html += "G: <input type='number' name='g' min='0' max='255' required>";
    html += "B: <input type='number' name='b' min='0' max='255' required>";
    html += "<input type='submit' style='display: block;' value='Set Custom Color'>";
    html += "</form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

const char update_page[] PROGMEM = R"=====(
    <!DOCTYPE html>
    <html><head><title>Flash this ESP8266!</title></head><body>
    <h2>Welcome!</h2>
    You are successfully connected to your ESP8266 via its WiFi.<br>
    Please click the button to proceed and upload a new binary firmware!<br><br>
    <b>Be sure to double check the firmware (.bin) is suitable for your chip!<br>
    I am not to be held liable if you accidentally flash a cat pic instead or something goes wrong during the update!<br>
    You are solely responsible for using this tool!</b><br><br>
    <form><input type="button" value="Select firmware..." onclick="window.location.href='/update'" />
    </form><br>
    </body></html>
    )=====";

void handleToggle() {
    animationEnabled = !animationEnabled;
    if (!animationEnabled) {
        // If animation is disabled, revert to warm white
        RgbColor warmWhite = RgbColor::LinearBlend(RgbColor(0, 0, 0), currentColor, currentBrightness);
        for (uint16_t i = 0; i < PixelCount; i++) {
            strip.SetPixelColor(i, warmWhite);
        }
        strip.Show();
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void turnOffNeoPixels() {
    for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
        strip.SetPixelColor(pixel, RgbColor(0, 0, 0));
    }
    strip.Show();
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleBrightness() {
    if (server.hasArg("value")) {
        currentBrightness = server.arg("value").toFloat();
        Serial.printf("Brightness set to: %f\n", currentBrightness);
        
        // Update the current color with new brightness
        RgbColor color = RgbColor::LinearBlend(RgbColor(0, 0, 0), currentColor, currentBrightness);
        for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
            strip.SetPixelColor(pixel, color);
        }
        strip.Show();
    }
    server.sendHeader("Location", "/");
    server.send(303);
}


void handleSetColor() {
    if (server.hasArg("color")) {
        String colorName = server.arg("color");
        if (colorName == "red") currentColor = RgbColor(255, 0, 0);
        else if (colorName == "green") currentColor = RgbColor(0, 255, 0);
        else if (colorName == "blue") currentColor = RgbColor(0, 0, 255);
        else if (colorName == "white") currentColor = RgbColor(255, 255, 255);
        else if (colorName == "warmwhite") currentColor = RgbColor(255, 100, 0);
        else {
            server.send(400, "text/plain", "Invalid color");
            return;
        }
        
        RgbColor color = RgbColor::LinearBlend(RgbColor(0, 0, 0), currentColor, currentBrightness);
        for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
            strip.SetPixelColor(pixel, color);
        }
        strip.Show();
        animationEnabled = false;
        Serial.printf("Color set to: %s\n", colorName.c_str());
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void handleSetCustomColor() {
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
        int r = server.arg("r").toInt();
        int g = server.arg("g").toInt();
        int b = server.arg("b").toInt();
        
        // Ensure values are within 0-255 range
        r = constrain(r, 0, 255);
        g = constrain(g, 0, 255);
        b = constrain(b, 0, 255);

        currentColor = RgbColor(r, g, b);
        
        RgbColor color = RgbColor::LinearBlend(RgbColor(0, 0, 0), currentColor, currentBrightness);
        for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
            strip.SetPixelColor(pixel, color);
        }
        strip.Show();
        animationEnabled = false;
        Serial.printf("Custom color set to RGB(%d, %d, %d)\n", r, g, b);
    } else {
        server.send(400, "text/plain", "Missing RGB values");
        return;
    }
    server.sendHeader("Location", "/");
    server.send(303);
}

void initializeStrip() {
    Serial.println("Initializing NeoPixel strip...");
    
    strip.Begin();
    strip.Show(); // Initialize all pixels to 'off'
    
    // Set warm white color
    RgbColor warmWhite = RgbColor::LinearBlend(RgbColor(0, 0, 0), currentColor, currentBrightness);
    for (uint16_t i = 0; i < PixelCount; i++) {
        strip.SetPixelColor(i, warmWhite);
    }
    strip.Show();
    
    Serial.println("NeoPixel strip initialized with warm white color");
}

void handleUpdateView() {
  server.send(200, "text/html", update_page);
}


void setup() {
    // Short delay to ensure stable power-up
    delay(100);

    Serial.begin(115200);
    while (!Serial);
    Serial.println("\nStarting up...");
        
    // Initialize NeoPixel strip
    initializeStrip();
    

    // Setup WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());



    // Setup web server
    server.on("/", handleRoot);
    server.on("/toggle", handleToggle);
    server.on("/brightness", handleBrightness);
    server.on("/setcolor", handleSetColor);
    server.on("/setcustomcolor", handleSetCustomColor);
    server.on("/ota_update", handleUpdateView);
    server.on("/off", turnOffNeoPixels);
    server.begin();
    Serial.println("HTTP server started");
    httpUpdater.setup(&server);
    SetRandomSeed();
}

void loop() {
    server.handleClient();
    
    if (animationEnabled) {
        if (animations.IsAnimating()) {
            animations.UpdateAnimations();
            strip.Show();
        } else {
            FadeInFadeOutRinseRepeat();
        }
    }
}