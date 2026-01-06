#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

// Pins
const int BTN_PP = 0, BTN_NEXT = 1, BTN_PREV = 2;

// State
String title = "", artist = "", status = "", cmdMsg = "";
long pos = 0, dur = 0;
bool isPlaying = false;
unsigned long lastScroll = 0, msgTimer = 0;
int scrollPos = 0;

// --- Custom Characters ---
byte musicNote[8] = {0x02, 0x03, 0x02, 0x0E, 0x1E, 0x0C, 0x00, 0x00};
byte personIcon[8] = {0x0E, 0x0E, 0x04, 0x1F, 0x04, 0x0E, 0x0A, 0x0A};
byte playChar[8] = {0x10, 0x18, 0x1C, 0x1E, 0x1C, 0x18, 0x10, 0x00};
byte pauseChar[8] = {0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x00};
byte nextChar[8] = {0x11, 0x19, 0x1D, 0x1F, 0x1D, 0x19, 0x11, 0x00}; // >>
byte prevChar[8] = {0x11, 0x13, 0x17, 0x1F, 0x17, 0x13, 0x11, 0x00}; // <<

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  lcd.init();
  lcd.backlight();
  
  // Register characters (max 8)
  lcd.createChar(0, musicNote);
  lcd.createChar(1, personIcon);
  lcd.createChar(2, playChar);
  lcd.createChar(3, pauseChar);
  lcd.createChar(4, nextChar);
  lcd.createChar(5, prevChar);

  pinMode(BTN_PP, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
}

void loop() {
  handleButtons();
  if (Serial.available() > 0) {
    parseData(Serial.readStringUntil('\n'));
  }
  updateDisplay();
}

void parseData(String data) {
  int f = data.indexOf('|'), s = data.indexOf('|', f+1), t = data.indexOf('|', s+1), fo = data.indexOf('|', t+1);
  if (f == -1) return;
  status = data.substring(0, f);
  String newT = data.substring(f+1, s);
  String newA = data.substring(s+1, t);
  if (newT != title) { title = newT; artist = newA; scrollPos = 0; }
  pos = data.substring(t+1, fo).toInt();
  dur = data.substring(fo+1).toInt();
  isPlaying = (status == "playing");
}

void updateDisplay() {
  // Line 1: Music Note + Scrolling Title
  lcd.setCursor(0, 0); lcd.write(0); // Music Note
  lcd.setCursor(1, 0); lcd.print(" ");
  lcd.setCursor(2, 0); lcd.print(getScrollText(title, 18));

  // Line 2: Person + Scrolling Artist
  lcd.setCursor(0, 1); lcd.write(1); // Person
  lcd.setCursor(1, 1); lcd.print(" ");
  lcd.setCursor(2, 1); lcd.print(getScrollText(artist, 18));

  // Line 3: Empty
  lcd.setCursor(0, 2); lcd.print("                    ");

  // Line 4 Logic
  if (millis() < msgTimer) {
    // COMMAND MODE (Hide time and play/pause)
    lcd.setCursor(0, 3);
    lcd.print("      "); // Clear start of line
    if (cmdMsg == "NEXT") {
      lcd.print("NEXT "); lcd.write(4); // "NEXT >>"
    } else {
      lcd.write(5); lcd.print(" PREV"); // "<< PREV"
    }
    lcd.print("        "); // Clear until end
  } 
  else {
    // NORMAL MODE
    char timeBuf[16];
    sprintf(timeBuf, "%02ld:%02ld / %02ld:%02ld", pos/60, pos%60, dur/60, dur%60);
    lcd.setCursor(0, 3); lcd.print(fixedLen(timeBuf, 13));
    
    // Play/Pause Icon at Col 19 (index 18)
    lcd.setCursor(18, 3);
    if (isPlaying) lcd.write(2); else lcd.write(3);
  }

  // Spinner always at Col 20 (index 19)
  lcd.setCursor(19, 3);
  if (isPlaying) {
    char anim[] = {'.', ' '};
    lcd.print(anim[(millis()/500) % 2]);
  } else {
    lcd.print(" ");
  }
}

String getScrollText(String text, int width) {
  if (text.length() <= width) {
    String out = text;
    while (out.length() < width) out += " ";
    return out;
  }
  String loopText = text + "    " + text;
  if (millis() - lastScroll > 450) { lastScroll = millis(); scrollPos++; }
  int start = scrollPos % (text.length() + 4);
  return loopText.substring(start, start + width);
}

String fixedLen(String s, int len) {
  while (s.length() < len) s += " ";
  return s.substring(0, len);
}

void handleButtons() {
  if (digitalRead(BTN_PP) == LOW) { Serial.println("PLAYPAUSE"); delay(250); }
  if (digitalRead(BTN_NEXT) == LOW) { 
    Serial.println("NEXT"); 
    cmdMsg = "NEXT"; msgTimer = millis() + 1000;
    delay(250); 
  }
  if (digitalRead(BTN_PREV) == LOW) { 
    Serial.println("PREV"); 
    cmdMsg = "PREV"; msgTimer = millis() + 1000;
    delay(250); 
  }
}