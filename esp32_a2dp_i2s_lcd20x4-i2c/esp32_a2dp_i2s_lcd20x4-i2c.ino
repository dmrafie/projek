#include "ESP_I2S.h"
#include "BluetoothA2DPSink.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- PINS (Right Side) ---
const uint8_t I2S_BCK = 18, I2S_WS = 17, I2S_DIN = 5;
const int I2C_SDA = 23, I2C_SCL = 22;
const int BTN_PP = 16, BTN_NEXT = 4, BTN_PREV = 19;

// --- CONFIGURATION ---
String bt_name = "ESP32 A2DP"; 

I2SClass i2s;
BluetoothA2DPSink a2dp_sink(i2s);
LiquidCrystal_I2C lcd(0x27, 20, 4);

// --- VARIABLES ---
String title = "Ready to Pair";
String artist = bt_name;
bool isPlaying = false;
bool wasConnected = false; 
uint32_t current_sec = 0;
uint32_t total_sec = 0;
unsigned long last_time_update = 0;
unsigned long lastScroll = 0, msgTimer = 0;
int scrollPos = 0;
String cmdMsg = "";

// --- NEW: BLINKING LOGIC ---
bool blinkState = true;
unsigned long lastBlink = 0;

// Custom Characters
byte musicNote[8] = {0x02, 0x03, 0x02, 0x0E, 0x1E, 0x0C, 0x00, 0x00};
byte personIcon[8] = {0x0E, 0x0E, 0x04, 0x1F, 0x04, 0x0E, 0x0A, 0x0A};
byte playChar[8]  = {0x10, 0x18, 0x1C, 0x1E, 0x1C, 0x18, 0x10, 0x00};
byte pauseChar[8] = {0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x00};
// NEW: Bluetooth Icon
byte btIcon[8] = {0x04, 0x16, 0x0A, 0x04, 0x0A, 0x16, 0x04, 0x00};

// --- CALLBACKS ---

void avrcp_position_callback(uint32_t pos) {
    current_sec = pos / 1000;
    last_time_update = millis();
}

void avrcp_metadata_callback(uint8_t id, const uint8_t *text) {
    String val = (char*)text;
    if (id == ESP_AVRC_MD_ATTR_TITLE && val.length() > 0) {
        if (title != val) { 
            title = val; scrollPos = 0; current_sec = 0; total_sec = 0; lcd.clear(); 
        }
    } else if (id == ESP_AVRC_MD_ATTR_ARTIST && val.length() > 0) {
        artist = val;
    } else if (id == ESP_AVRC_MD_ATTR_PLAYING_TIME) {
        total_sec = val.toInt() / 1000; 
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, musicNote);
    lcd.createChar(1, personIcon);
    lcd.createChar(2, playChar);
    lcd.createChar(3, pauseChar);
    lcd.createChar(4, btIcon);

    pinMode(BTN_PP, INPUT_PULLUP);
    pinMode(BTN_NEXT, INPUT_PULLUP);
    pinMode(BTN_PREV, INPUT_PULLUP);

    i2s.setPins(I2S_BCK, I2S_WS, I2S_DIN);
    i2s.begin(I2S_MODE_STD, 44100, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH);

    a2dp_sink.set_avrc_metadata_callback(avrcp_metadata_callback);
    a2dp_sink.set_avrc_rn_play_pos_callback(avrcp_position_callback, 1);
    a2dp_sink.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_PLAYING_TIME);
    
    a2dp_sink.start(bt_name.c_str());
}

void loop() {
    bool currentlyConnected = a2dp_sink.is_connected();
    isPlaying = (a2dp_sink.get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED);

    // --- BLINK TIMER ---
    if (millis() - lastBlink >= 500) {
        blinkState = !blinkState;
        lastBlink = millis();
    }

    // --- RESET LOGIC ON DISCONNECT ---
    if (!currentlyConnected && wasConnected) {
        title = "Ready to Pair";
        artist = bt_name;
        total_sec = 0;
        current_sec = 0;
        lcd.clear();
    }
    wasConnected = currentlyConnected;

    if (isPlaying) {
        if (millis() - last_time_update >= 1000) {
            current_sec++;
            last_time_update = millis();
        }
    }

    // Buttons
    if (digitalRead(BTN_PP) == LOW) {
        if (isPlaying) a2dp_sink.pause(); else a2dp_sink.play();
        delay(400); 
    }
    if (digitalRead(BTN_NEXT) == LOW) { 
        a2dp_sink.next(); 
        cmdMsg = "NEXT"; msgTimer = millis()+1000; 
        total_sec = 0; current_sec = 0;
        delay(400); 
    }
    if (digitalRead(BTN_PREV) == LOW) { 
        a2dp_sink.previous(); 
        cmdMsg = "PREV"; msgTimer = millis()+1000; 
        total_sec = 0; current_sec = 0;
        delay(400); 
    }

    updateDisplay();
}

void lcdPrintTime(uint32_t sec) {
    uint16_t m = sec / 60;
    uint16_t s = sec % 60;
    if (m < 10) lcd.print("0"); lcd.print(m);
    lcd.print(":");
    if (s < 10) lcd.print("0"); lcd.print(s);
}

void updateDisplay() {
    bool isConn = a2dp_sink.is_connected();

    // --- LINE 1: Title or Blinking Ready ---
    lcd.setCursor(0, 0);
    if (isConn) {
        lcd.write(0); // Music Note
        lcd.print(" ");
        lcd.print(getScrollText(title, 18));
    } else {
        // Center "Ready to Pair" (13 chars) in 20 chars = 3 spaces padding
        if (blinkState) {
            lcd.print("   Ready to Pair    ");
        } else {
            lcd.print("                    ");
        }
    }
    
    // --- LINE 2: Artist or BT Name ---
    lcd.setCursor(0, 1);
    if (isConn) {
        lcd.write(1); // Artist Icon
    } else {
        lcd.write(4); // Bluetooth Icon
    }
    lcd.print(" ");
    lcd.print(getScrollText(artist, 18));

    // Line 3: Empty
    lcd.setCursor(0, 2); lcd.print("                    "); 

    // Line 4: Status / Time
    lcd.setCursor(0, 3);
    if (millis() < msgTimer) {
        lcd.print("Action: " + cmdMsg + "      ");
    } else {
        if (!isConn) {
            lcd.print("Disconnected       ");
        } else {
            lcdPrintTime(current_sec);
            lcd.print(" / ");
            lcdPrintTime(total_sec);
            lcd.print("     ");
        }
        
        lcd.setCursor(18, 3);
        if (isPlaying && isConn) {
            char anim[] = {'.', ' '};
            lcd.write(2); 
            lcd.print(anim[(millis()/500) % 2]);
        } else if (isConn) {
            lcd.write(3); 
            lcd.print(" ");
        } else {
            lcd.print("  ");
        }
    }
}

String getScrollText(String text, int width) {
    if (text.length() <= width) {
        String out = text; while (out.length() < width) out += " ";
        return out;
    }
    String loopText = text + "    " + text;
    if (millis() - lastScroll > 450) { lastScroll = millis(); scrollPos++; }
    int start = scrollPos % (text.length() + 4);
    return loopText.substring(start, start + width);
}