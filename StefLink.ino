#include <ESP8266WiFi.h>
#include <string.h>
#include "FS.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// variables and definitions need for handling WiFi connection
const char* ssid = "";
const char* password = "";
#define PORT 999
#define ledPin 13

// variables and definitions needed for handling SPIFFS
bool    spiffsActive = false;
#define SOURCEFILE "/index.txt"

// definitions needed for software SPI communication with OLED
#define OLED_MOSI   16
#define OLED_CLK    5
#define OLED_CS     4
#define OLED_DC     0
#define OLED_RESET  2

// carrot bmp definition
#define HEART_GLCD_WIDTH  16
#define HEART_GLCD_HEIGHT 14 
static const unsigned char PROGMEM heart_glcd_bmp[] =
{
    0x1E, 0x78, 0x3F, 0xFC, 0x7F, 0xFE, 0xE3, 0xFF, 0xE7, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF,
    0x7F, 0xFE, 0x1F, 0xF8, 0x1F, 0xF8, 0x0F, 0xF0, 0x03, 0xC0, 0x01, 0x80, 0x01, 0x80
};  

// variables needed for handling communication via serial port
String serialBuffer;
bool outMsgReady = false;

// OLED constructor (software SPI with custom pin allocations)
Adafruit_SSD1306 display (OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// WiFi server constructor
WiFiServer server(999); // custom port number


void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println(WiFi.localIP());
    // prepare GPI13
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.println("Connecting to local network");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Connected to: " + WiFi.SSID());
    Serial.printf("Received Signal Strength: %d dBm\n", WiFi.RSSI());

    // Attempt to reconnect when lost connection with AP
    WiFi.setAutoReconnect(true);

    // Start the server
    server.begin();
    Serial.println("Server started");

    // Print the gateway IP address
    Serial.printf("Gataway IP: %s\n", WiFi.gatewayIP().toString().c_str());

    // Print the local IP address
    Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());

    // Print port
    Serial.printf("Port: %d\n", PORT);

    // Print ESP's host name in local network
    Serial.println("Host name: " + WiFi.hostname());

    // Start filing subsystem
    if (SPIFFS.begin()) {
        Serial.println("File system actived");
        spiffsActive = true;
    } else {
        Serial.println("Unable to activate file system");
    }

    // Init OLED
    display.begin(SSD1306_SWITCHCAPVCC);
    display.clearDisplay();
    display.display();
    Serial.println("OLED initialized");
    Serial.println("\nAquired requests:");

    delay(100);
}


void loop() {

    if (spiffsActive && SPIFFS.exists(SOURCEFILE)) {

        // Read from serial port when available
          while(Serial.available()) {
              serialBuffer = Serial.readString(); // read the incoming data as string
              outMsgReady = true;
          }

          // Check if a client has connected
          WiFiClient client = server.available();
          if (!client) {
            return;
          }
      
          // Wait until the client sends some data
          while (!client.available()) {
             delay(1);
          }
      
          // Read the whole message
          String text;
          while (client.available()>0) {
             text += char(client.read());
          }
          
          // Read request (first line)
          String req = text.substring(0, text.indexOf('\r'));
          client.flush();

          // display request type
          Serial.println(req);
      
          // Match the request
          if (req.indexOf("POST / HTTP/1.1") != -1) {  // checks if you're receiving message from browser user
            
            // flashing LED when receiving message
            digitalWrite(ledPin, HIGH);
            delay(100);
            digitalWrite(ledPin, LOW);

            // preparing diplay
            display.clearDisplay();
            display.setTextColor(WHITE);

            // extracting username and text from the message
            bool contentCorrect = false;  // content correct flag
            String username;

            int content_index = text.indexOf("!%&"); // index of start of content
            int indicator_index = text.indexOf("&%!"); // index of indicator
            
            if ((content_index != -1) && (indicator_index != -1))
            {
              contentCorrect = true;
              
              content_index += 3; // skip over "!%&"
              username = text.substring(content_index, indicator_index);
              indicator_index += 3; // skip over "&%!"
              text = text.substring(indicator_index);
            }
            
            // displaying message content
            Serial.println("Received: " + text);

            // drawing username
            display.setTextSize(2);
            display.setCursor(0,0);

            // secret addon
            if (username == "xxx") display.drawBitmap(112, 1, heart_glcd_bmp, HEART_GLCD_WIDTH, HEART_GLCD_HEIGHT, WHITE);

            if (contentCorrect) {
              // handling words distribution
              int text_index = username.indexOf(" ");
              int lineLength = 0;
              String singleWord;
              // writing words into OLED
              while (text_index != -1) {
                singleWord = username.substring(0,text_index);
                lineLength += singleWord.length() + 1;  // include space after word
                if (lineLength > 11) break;
                display.print(singleWord + " ");
                username = username.substring(text_index+1); // skip " "
                text_index = username.indexOf(" ");
              }
              // writing last word into OLED
              lineLength += username.length();  // include space after word
              if (lineLength < 11) display.print(username);
            }

            // drawing text
            display.setTextSize(1);
            display.setCursor(0,16);
            
            if (contentCorrect) {
              // handling words distribution
              int text_index = text.indexOf(" ");
              int lineLength = 0;
              int lineIndex = 16;
              String singleWord;
              // writing words into OLED
              while (text_index != -1) {
                singleWord = text.substring(0,text_index);
                lineLength += singleWord.length() + 1;  // include space after word
                if (lineLength > 21) {
                  lineIndex += 8;
                  if (lineIndex > 56) break;
                  display.setCursor(0,lineIndex);
                  lineLength = singleWord.length() + 1;
                }
                display.print(singleWord + " ");
                text = text.substring(text_index+1); // skip " "
                text_index = text.indexOf(" ");
              }
              // writing last word into OLED
              lineLength += text.length();  // include space after word
              if (lineLength > 21) {
                lineIndex += 8;
                display.setCursor(0,lineIndex);
              }
              if (lineIndex <= 56) display.print(text);
            }
            display.display();
            
          }
          else if (req.indexOf("GET / HTTP/1.1") != -1) {   // checks if you're receiving outgoing message query
            
            // Prepare answer
            String s = "HTTP/1.1 200 OK\r\n";
            s += "Content-Type: text/html\r\n";
            s += "\r\n";
            if (outMsgReady) {
              s += serialBuffer;
              outMsgReady = false;
              Serial.println("Send: " + serialBuffer);
            }
            client.flush();
            // Send the response to the client
            client.print(s);
            delay(1);
          }
          else {
            Serial.println("invalid request");
            client.stop();
            return;
          }
          
    } else {
      Serial.print("Unable To Find ");
      Serial.println(SOURCEFILE);
      while (true){yield();}
    }
    
}
