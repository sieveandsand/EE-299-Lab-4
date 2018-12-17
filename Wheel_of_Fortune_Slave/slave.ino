// EE 299 Lab 4
// Feifan Qiao, Mitchell Szeto, Bert Zhao
// This is the code for slave device. On receiving transmission from the master device,
// the slave device spins the wheel and determins who much each guess is worth.
// It also displays the amount of money each player have.
// Last modified: 12/11/2018

#include <LiquidCrystal.h>
LiquidCrystal lcd(2, 3, 4, 5, 6, 7, 8); // bus 1

#define SCREEN_LENGTH 16
String wheelDisplay = "100|200|300|400|500|600|700|800|900|1000|1100|1200|1300|1400|1500|";
#define BUZZER 10  // one of the physical buzzer is broken, remember to check

int state = 0;
int wheelVal = 0;
int strength = 0;
int player = 0;
int score = 0;
int p1Total = 0;
int p2Total = 0;
String guessWord = "";

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER, OUTPUT);
  lcd.begin(16, 2);
  lcd.display();
}

void loop() {
  if (state == 0) {
    state = readWheelInput();
  } else if (state == 1) {
    spinWheel(wheelVal, strength);
    state = 0;
  }
}

// receives and reads the wheel value strength from the master
int readWheelInput() {
   if (Serial.available() == 4) { // 2 times 2 bytes
      wheelVal = readInt();
      strength = readInt();
      Serial.println(wheelVal);
      Serial.println(strength);
      return 1;
   } else {
      return 0;
   }
}

// spins the wheel and displays it on the lcd
void spinWheel(int num, int strength) {
  String number = String(num);
  String printStr = "";
  int i = 0;
  int count = 0;
  while (printStr.substring(0, number.length()) != number || strength > count) {
    printStr = displayWheelPos(i);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(printStr);
    lcd.setCursor(0, 1);
    lcd.print('^');
    delay(50 / strength);
    i++;
    if (i == wheelDisplay.length()) {
      i = 0;
      count += 10;
    }
  }
  tone(BUZZER, 261);
  delay(100);
  noTone(BUZZER); 
}

// shows the spinning of the wheel
String displayWheelPos(int num) {
  if (num + SCREEN_LENGTH < wheelDisplay.length()) {
    return wheelDisplay.substring(num, num + SCREEN_LENGTH);
  } else {
    return wheelDisplay.substring(num, wheelDisplay.length()) + wheelDisplay.substring(0, SCREEN_LENGTH);
  }
}

// reads bytes from the master devices 
// and converts them to integers
int readInt() {
  byte b1 = Serial.read();
  byte b2 = Serial.read();
  int number = b1 | b2 << 8;
  return number;
}
