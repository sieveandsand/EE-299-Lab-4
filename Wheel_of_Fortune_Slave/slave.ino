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
    // spin wheel
    state = readWheelInput();
  } else if (state == 1) {
    // display wheel
    lcd.clear();
    spinWheel(wheelVal, strength);
    delay(1000);
    state = 2;
  } else if (state == 2) {
    // Get player info and word
    state = readWordAndInfo();
  } else if (state == 3) {
    // print hangman game and player info
    lcd.clear();
    printHangman(guessWord);
    printInfo(player, score);

    // checks if game is complete or not
    if (hangmanFinished(guessWord)) {
      state = 4;
    } else {
      state = 2;
    }
  } else if (state == 4) {
    // Get end game info
    state = readGameEnd();
  } else if (state == 5) {
    // Prints game over state
    printEndGame(p1Total, p2Total);
    state = 6;
  }
}

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

int readWordAndInfo() {
   if (Serial.available() == 4) {  // 2 times 2 bytes
      player = readInt();
      score  = readInt();
   }
    if (Serial.available()) {
      guessWord += String((char)Serial.read());
      return 3;
   } else {
      return 2;
   }
}

int readGameEnd() {
  if (Serial.available() == 4) {  // 2 times 2 bytes
      p1Total = readInt();
      p2Total = readInt();
      return 5;
   } else {
      return 4;
   }
}

void spinWheel(int num, int strength) {
  String number = String(num);
  String printStr = "";
  int i = 0;
  int count = 0;
  while (printStr.substring(0, number.length()) != number || strength > count) {
    printStr = dispalyWheelPos(i);
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

String dispalyWheelPos(int num) {
  if (num + SCREEN_LENGTH < wheelDisplay.length()) {
    return wheelDisplay.substring(num, num + SCREEN_LENGTH);
  } else {
    return wheelDisplay.substring(num, wheelDisplay.length()) + wheelDisplay.substring(0, SCREEN_LENGTH);
  }
}

void printInfo(int player, int score) {
  lcd.setCursor(0, 1);
  lcd.print("P");
  lcd.print(player);
  lcd.print(": Score $");
  lcd.print(score);
}

void printHangman(String str) {
  lcd.setCursor(0, 0);
  lcd.print(str));
}

bool hangmanFinished(String str) {
  for (int i = 0; i < str.length(); i++) {
    if (str.charAt(i) == "*") {
      return false;
    }
  }
  return true;
}

void printEndGame(int p1Total, int p2Total) {
  lcd.setCursor(0, 0);
  lcd.print("Player 1 won $");
  lcd.print(p1Total);
  lcd.setCursor(0, 1);
  lcd.print("Player 2 won $");
  lcd.print(p2Total);
}

int readInt() {
  byte b1 = Serial.read();
  byte b2 = Serial.read();
  int number = b1 | b2 << 8;
  return number;
}
