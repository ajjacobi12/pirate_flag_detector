// pull libraries needed to manage hardware components
#include <SPI.h> // Serial Peripheral Interface: hardware communication protocol the ESP32 chip uses to talk to LoRa radio chip
#include <LoRa.h> // translates commands into complex RF frequencies and packet encoding structures required by the radio
#include <Wire.h> // manages the I^2C (Inter-Integrated Circuit) protocol, which is a two-wire communcation standard used to drive the built-in OLED screen
#include <U8g2lib.h> // monochrome graphics library used to render text, fonts, and basic shapes on the physical screen

// Heltec V3 LoRa pin definitions
// #define is a preprocessor directive, tells compiler "every time you see SS in the code, replace it with the number 8 before compiling"
// numbers match the internal factory routing of the Heltec V3 board, linking main CPU pins to the radio module's internal 
//  Slave Select (SS), Reset (RST), Digital I/O 1 (DI01), and Status (BUSY) lines
#define RST   12
#define SS    8   
#define DIO1  14
#define BUSY  13

// custom component pins
// assign the physical hardware pins on the edge of the Heltec board where external components physically plug in
#define PIR_PIN      1  // connect PIR OUT here
#define BUZZER_PIN   2  // connect Piezo positive here
#define LED_PIN      3  // connect red LED anode here

// initialize OLED (SSD1306 128x64 via I2C)
// creates and object named u8g2, tells the libary 
// "We have a 128x64 display driven by an SSD1306 controller, it's oriented right-side up (U8G2_R0), its reset line is pin 21, the clock pin is 18, and the data pin is 17."
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 21, /* clock=*/ 18, /* data=*/ 17);

void setup() {
    // pinMode() configures electrical behavior of a specific GPIO pin
    // INPUT sets the pin to a high-impedance state, allowing it to listen for voltage changes coming from the PIR sensor
    // OUTPUT allows the ESP32 to push 3.3 volts out of those pins to power the LED or the buzzer
    pinMode(PIR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
 
    u8g2.begin(); // powers up OLED screen and clears memory registers
    u8g2.setFont(u8g2_font_ncenB08_tr); // selects text font profile loaded into microcontroller's flash memory

    // initialize LoRa SPI
    SPI.begin(9, 11, 10, 8); // SCK, MISO, MOSI, SS; 
    // tells internal SPI controller which pins represent the clock (SCK), Master-In-Slave-Out (MISO), Master-Out-Slave-In (MOSI), and Slave Select (SS) lanes connected to the radio chip
    LoRa.setPins(SS, RST, DIO1); // gives pin addresses to LoRa library so it knows where to direct its commands

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Booting System..."); // 0, 12 are X, Y coordinates on screen
    u8g2.sendBuffer();

    // if LoRa initialization does not succeed...
    if (!LoRa.begin(915E6)) {
        // LoRa.begin() initializes radio hardware and tunes it to 915 MHz
        u8g2.drawStr(0, 26, "LoRa Init Failed!");
        u8g2.sendBuffer();
        while (1);
    }

    // set spreading factor to 10 for deep canopy penetration, spreads radio pulses wider across time
    LoRa.setSpreadingFactor(10);

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "LoRa Ready (915MHZ)");
    u8g2.drawStr(0, 26, "CAMP GUARD ARMED");
    u8g2.sendBuffer();
 }

 void loop() {
    int motion = digitalRead(PIR_PIN);
    // digitalRead checks voltage status of PIR pin, 
    // if sensor sees a warm hue moving it outputs 3.3V causing digitalRead to return a value of 1 (HIGH)
    // if quiet, returns 0 (LOW)

    if (motion == HIGH) {
        // hardware debounce check: confirm signature is stable
        delay(1000);
        // code checks for movement a second time after 1000 ms
        if(digitalRead(PIR_PIN) == HIGH) {
            u8g2.clearBuffer();
            u8g2.drawStr(0, 12, "!!! INTRUDER !!!"); 
            u8g2.drawStr(0, 26, "Sending Alert...");
            u8g2.sendBuffer();

            // send secure verification packet
            LoRa.beginPacket(); // wipes radio's outgoing memory buffer and flags it to prepare for a new broadcast
            LoRa.print("EF_PIRATE_ALARM_TRIGGERED"); // custom secret authorization passkey string into digital buffer
            LoRa.endPacket(); // tells radio hardware to modulate that string onto the 915 MHz frequency and push it out through the antenna into the air

            // trigger local siren/light deterrent pattern
            // six flashes and beeps
            // TRIGGER LOCAL DETERRENT: Long Flash + Short Chirp
            for(int i = 0; i < 6; i++) {
                digitalWrite(LED_PIN, HIGH);     // Turn the bright LEDs ON
                digitalWrite(BUZZER_PIN, HIGH);  // Turn the buzzer ON
                
                delay(15);                       // Wait just 15ms—enough for a short audio chirp
                digitalWrite(BUZZER_PIN, LOW);   // Turn the buzzer OFF immediately
                
                delay(135);                      // Keep the LEDs ON for the remaining 135ms of the flash
                digitalWrite(LED_PIN, LOW);      // Turn the LEDs OFF
                
                delay(150);                      // Dark period between flashes (keeps the rhythm steady)
            }

            // sleep loop to prevent alert floods while they run away
            // locks transmitter code for 5 seconds after an alert
            delay(5000);

            u8g2.clearBuffer();
            u8g2.drawStr(0, 12, "LoRa Ready (915MHz)");
            u8g2.drawStr(0, 26, "CAMP GUARD ARMED");
            u8g2.sendBuffer();
        }
    }
 }