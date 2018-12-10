#include <LiquidCrystal.h>
LiquidCrystal lcd(2, 3, 4, 5, 6, 7, 8);  // bus 1

// Analog/digital port connections
int buttonPin = 10;          // please avoid using pin 8 when possible, might have interference with lcd
int tiltPin = 9;
int buttonState = 0;
int tiltState = 0;
int rotationSensor = 1;

// Wheel and spin properties
#define WHEEL_SIZE 15
int wheel[WHEEL_SIZE];
unsigned int spinStrength = 0;
#define INT_MAX 65535
#define MAX_SPIN 5000
bool nextRoll = true;
int spinNumber = 0;
int divFactor = 0;

// inputting characters and words through lcd
// and rotation sensor
int cursorIndex = 0;
int prevCursorIndex = -1;
int page = 1;                   // 1 for the first screen and 2 for second screen, initialized to 1
const int asciiStart = 97;
const int asciiEnd = 122;
const int cursorStep = 40;      // cursor moves for every increment of 40 out of 1024 rotation sensor values
unsigned long firstTime = 0;
int wordLength = 0;
int nClick = 0;

String phrase;
String updatedPhrase;

struct Player {
  char guess;
  int guessValue;
  int state;
  int score;
};
struct Player p1 = {0, 100, 0, 0};
struct Player p2 = {0, 100, 0, 0};
/*
 * Key for the player state
 * 0 = player shakes tilt sensor to spin the wheel
 * 1 = player waits while the wheel scrolls
 * 2 = player enters a letter to guess
 * 3 = checks the guess with the phrase
 * 4 = checks the game state and determines if game is over
 * 5 = clears the serial buffer
*/

int gameState = 0;
/*
 * Key for the game state
 * 0 = game start and setup
 * 1 = player 1's turn
 * 2 = player 2's trun
 * 3 = game if complete
 * 4 = hold state
*/

int spinNum = 0; // Number of spins the wheel goes through

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(tiltPin, INPUT);
  makeWheel();
  lcd.begin(16, 2);
//  printWheel(); for debugging
  lcdDisplay("Welcome to the Wheel of Fourtune");
  lcdDisplay("Enter in phrase");
}

void loop() {  
//  Serial.print("gameState = "); for debugging
//  Serial.println(gameState);
   divFactor = (analogRead(rotationSensor)/10) + 1;
//  Serial.println(divFactor); for debugging
  if (gameState == 0) {
    inputWord();
    lcdDisplay(phrase);
    createUpdatedPhrase();
    gameState = 1;
    p1.state = 0;
    lcdDisplay("Player 1's turn");
    lcdDisplay("The state of the word:");
    lcdDisplay(updatedPhrase);
    delay(2000);  // so that button state read is not tangled together
  } else if (gameState == 1) {
    // player 1's turn
    // spin wheel and guess letter or whole phrase
    // count money
    // send player 1's stats to slave LCD
    // if all words guessed gamestate = 4
    // else gamestate = 3 back to player 2
    if (p1.state == 0) { // player 1 spins the wheel
      spinNum = spin();
      if (buttonState) {
        p1.state = 1;
      }
    } else if (p1.state == 1) { // Scrolls through the array
      p1.guessValue = scrollArray();
      lcdDisplay(p1.guessValue);
      Serial.write(p1.guessValue);  // sending data to slave
      Serial.write(spinStrength);   // sending data to slave
      lcdDisplay("Player 1 guess a letter");
      p1.state = 2;
    } else if (p1.state == 2) { // Player enters a guess
      char temp = pickCharacter();
      delay(100);  // please don't remove, otherwise the cursor on lcd updates too fast and is not going to show
      if (temp != NULL) {
        p1.guess = temp;
        lcdDisplay("You guessed:    ");
        lcdDisplay(String((char)p1.guess));
        Serial.write(1);           // sending data to slave
        Serial.write(p1.score);    // sending data to slave
        char* cString = (char*) malloc(sizeof(char)*(phrase.length() + 1));
        phrase.toCharArray(cString, phrase.length() + 1);
        Serial.write(cString);    // sending data to slave
//        Serial.print("available = ");
//        Serial.println(Serial.available());      
        spinNum = 0;
        p1.state = 3;
      }
    } else if (p1.state == 3) { // Checks the guess and updates the word and score
      int correct = checkGuess(p1.guess);
      p1.score += (correct * p1.guessValue);
      lcdDisplay("Player 1's score is: ");
      lcdDisplay(p1.score);
      p1.state = 4;
    } else if (p1.state == 4) { // Checks the game state and moves to player 2's turn or the game is over
      updateGuess(p1.guess);
      lcdDisplay("The state of the word:");
      lcdDisplay(updatedPhrase);
      bool game = gameOver();
      if (game) {
        gameState = 3;
      } else {
        lcdDisplay("Player 2's turn");
        spinStrength = 0;
        gameState = 2;
        p2.state = 0;
      }
    }
  } else if (gameState == 2) {
    // player 2's turn
    // spin wheel and guess letter or whole phrase
    // count money
    // send player 1's stats to slave LCD
    // if all words guessed gamestate = 4
    // else gamestate = 2 back to player 1
    if (p2.state == 0) {
      spinNum = spin();
      if (buttonState) {
        p2.state = 1;
      }
    } else if (p2.state == 1) {
      p2.guessValue = scrollArray();
      Serial.write(p2.guessValue);  // sending data to slave
      Serial.write(spinStrength);   // sending data to slave
      lcdDisplay("Player 2 guess a letter");
      p2.state = 2;
    } else if (p2.state == 2) {
      char temp = pickCharacter();
      delay(100);  // please don't remove, otherwise the cursor on lcd updates too fast and is not going to show
      if (temp != NULL) {           
        p2.guess = temp;
        lcdDisplay("You guessed:    ");
        lcdDisplay(String((char)p2.guess));
        Serial.write(2);           // sending data to slave
        Serial.write(p2.score);    // sending data to slave
        char* cString = (char*) malloc(sizeof(char)*(phrase.length() + 1));
        phrase.toCharArray(cString, phrase.length() + 1);
        Serial.write(cString);    // sending data to slave
//        Serial.print("available = ");
//        Serial.println(Serial.available());
        spinNum = 0;
        p2.state = 3;
      }
    } else if (p2.state == 3) {
      int correct = checkGuess(p2.guess);
      p2.score += (correct * p2.guessValue);
      lcdDisplay("Player 2's score is: ");
      lcdDisplay(p2.score);
      p2.state = 4;
    } else if (p2.state == 4) {
      updateGuess(p2.guess);
      lcdDisplay("The state of the word:");
      lcdDisplay(updatedPhrase);
      bool game = gameOver();
      if (game) {
        gameState = 3;
      } else {
        lcdDisplay("Player 1's turn");
        spinStrength = 0;
        gameState = 1;
        p1.state = 0;
      }
    }
  } else if (gameState == 3) {
    // game over
    // declare winner and money won
    // start new game?
    lcdDisplay("The game is over");
    lcdDisplay("Player 1's final score: ");
    lcdDisplay(p1.score);
    lcdDisplay("Player 2's final score: ");
    lcdDisplay(p2.score);
    Serial.write(p1.score);    // sending data to slave
    Serial.write(p2.score);    // sending data to slave
    gameState = 4;
  } else if (gameState == 4) {
    // holding state
  }
}

// Creates the array with the values of the spins
void makeWheel() {
  for (int i = 0; i < WHEEL_SIZE; i++) {
    wheel[i] = (i + 1) * 100;
  }
}

// Prints the values in the wheel array, used for debugging
void printWheel() {
  for (int i = 0; i < WHEEL_SIZE; i++) {
    lcdDisplay(wheel[i]);
  }
}

// Spins the wheel based on the number of tilt shakes
int spin() {
  buttonState = digitalRead(buttonPin);
  tiltState = digitalRead(tiltPin);
  if (spinStrength < MAX_SPIN) {
      if (nextRoll && tiltState) {
      nextRoll = false;
      spinStrength++;
    } else if (!tiltState) {
      nextRoll = true;
    }
  }
  if (buttonState) {
    spinNumber = wheelSlots();
//    Serial.println(spinStrength);
//    Serial.println(spinNumber);
  }
  return spinNumber;
}

// Determines the number of slots the wheel spins through
int wheelSlots() {
  return spinStrength / divFactor;
}

// Scrolls through the array based on the spin number and
// prints the values to the serial monitor
int scrollArray() {
  int i = 0;
  int j = 0;
  while (i < spinNum) {
    // Serial.println(wheel[j]);
    i++;
    j++;
    delay(100);
    if (j >= WHEEL_SIZE) {
      j = 0;
    }
  }
  lcdDisplay(wheel[j]);
  return wheel[j];
}

// Checks the phrase with the guess to determine and return the number of occurances of
// the guess. Returns 0 if the letter has already been guessed
int checkGuess(char guess) {
  int i = 0;
  int result = 0;
  while (i < updatedPhrase.length()) {
    if (updatedPhrase.charAt(i) == guess) {
      return 0;
    }
    i++;
  }
  i = 0;
  while (i < phrase.length()) {
    if (phrase.charAt(i) == guess) {
      result++;
    }
    i++;
  }
  return result;
}

// Creates a string of asterisks with the same number of characters
// as the string that is to be guessed
void createUpdatedPhrase() {
  // Serial.println(phrase.length());
  for (int i = 0; i < phrase.length(); i++) {
    updatedPhrase.concat("*");
  }
}

// Updates the updatedPhrase with the acctual letter if it was guessed correctly
void updateGuess(char guess) {
  for (int i = 0; i < phrase.length(); i++) {
    if (phrase.charAt(i) == guess) {
      updatedPhrase.setCharAt(i, guess);
    }
  }
}

// Determines if the game is over based on the number of unguessed words
bool gameOver() {
  int i = 0;
  int unguessedLetters = 0;
  while (i < updatedPhrase.length()) {
    if (updatedPhrase.charAt(i) == '*') {
      unguessedLetters++;
    }
    i++;
  }
  if (unguessedLetters) {
    return false;
  } else {
    return true;
  }
}

// word length is limited to 16 characters
// user scrolls through characters
// single click for selecting current character
// double click to finish inputting word
void inputWord() {
  do {
    int rotationVal = analogRead(rotationSensor) / cursorStep;
    int displayVal = rotationVal + asciiStart;
    lcd.setCursor(wordLength, 0);
    lcd.print((char)displayVal);
    lcd.setCursor(wordLength, 1);
    lcd.print("^");
    if (wordLength != 0) {
      lcd.setCursor(wordLength - 1, 1);
      lcd.print(" ");
    }

    nClick = buttonDoubleClick();
    if (nClick == 1) {
      phrase += String((char)displayVal);
      wordLength++;
    }
  } while (nClick != 2);
  // clears the cursor after finishing
  lcd.setCursor(wordLength, 1);
  lcd.print(" ");
}

// displays the alphabet on the screen for the player to choose
// returns the character chosen by the player
char pickCharacter() {
  if (prevCursorIndex != -1) {
    if (page == 1) {
      lcd.setCursor(prevCursorIndex, 1);
    } else {
      lcd.setCursor(prevCursorIndex - 16, 1);  
    }
    lcd.print(" ");
  }

  if (page == 1) {
    for (int i = asciiStart; i < asciiStart + 16; i++) {
      lcd.setCursor(i - asciiStart, 0);
      lcd.print((char)i);
    }
  } else {
    for (int i = asciiStart + 16; i <= asciiEnd + 6; i++) {
      lcd.setCursor(i - asciiStart - 16, 0);
      if (i > asciiEnd) {  // clears the any character displayed beyond 'z'
        lcd.print(" ");
      } else {
      lcd.print((char)i);
      }
    }
  }

  cursorIndex = analogRead(rotationSensor) / cursorStep;
  prevCursorIndex = cursorIndex;

  if (cursorIndex <= 15) {
    page = 1; 
    lcd.setCursor(cursorIndex, 1);
    lcd.print("^");
  } else {
    page = 2;
    lcd.setCursor(cursorIndex - 16, 1);
    lcd.print("^");
  }

  if (digitalRead(buttonPin) == HIGH) {
    return cursorIndex + asciiStart;
  } else {
    return NULL;
  }
}

// returns 2 if button is pressed twice within 2 seconds
// returns 1 if button is pressed once within 2 seconds
// returns 0 if button is not pressed
int buttonDoubleClick() {
  if (digitalRead(buttonPin) == HIGH) {
    firstTime = millis();
    delay(200);
    while (millis() - firstTime <= 2000) {
      if (digitalRead(buttonPin) == HIGH) {
        return 2;
      }
    }
    return 1;
  }
  return 0;
}

void lcdDisplay(String input) {
  lcd.home();
  if (input.length() <= 16) {
    lcd.print(input);
  } else {
    lcd.print(input.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(input.substring(16, input.length()));
  }
  delay(2000);
  lcd.clear();
}

void lcdDisplay(int input) {
  lcd.home();
  lcd.print(input);
  delay(2000);
  lcd.clear();
}
