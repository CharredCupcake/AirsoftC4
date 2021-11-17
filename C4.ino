#include <Keypad.h>
#include <TM1637Display.h>
#include <arduino-timer.h>

///Global Var START
//Keypad
static const byte ROWS = 4; // Define the number of rows on the keypad
static const byte COLS = 3; // Define the number of columns on the keypad
char keys[ROWS][COLS] = { // Matrix defining character to return for each key
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
//Keypad init
byte rowPins[ROWS] = {14, 15, 16, 17}; //connect to the row pins (R0-R3) of the keypad
byte colPins[COLS] = {2, 3, 4}; //connect to the column pins (C0-C2) of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS ); //initialize an instance of class

//Buzzer
static const byte buzzer = 11; //buzzer to arduino pin 11
static const byte soundLength = 50;
static const int clickSound = 1000;
static const int explosionSound = 2000;

//Display
static const byte CLK = 19;
static const byte DIO = 18;
static const byte underline[] = {SEG_D, SEG_D, SEG_D, SEG_D};
static const byte midline[] = {SEG_G, SEG_G, SEG_G, SEG_G};
static const byte topline[] = {SEG_A, SEG_A, SEG_A, SEG_A};
static const byte xxxx[] = {SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, SEG_B | SEG_C | SEG_E | SEG_F | SEG_G};
//Display init
TM1637Display mainDisplay = TM1637Display(CLK, DIO);

//LEDs
static const byte redLed = 12;
bool isRedOn = false;//if red is on green will be off and vice versa
static const byte greenLed = 13;

//Passwords
static const byte PASSWORD_LENGTH = 4;
static const byte SETUP_PASSWORD[PASSWORD_LENGTH] = {1, 2, 3, 4};//change password for set up
static const byte DEFUSE_PASSWORD[PASSWORD_LENGTH] = {5, 6, 7, 8};//change password for defuseing
byte inputPass[PASSWORD_LENGTH];
char inputKey = NO_KEY;
bool isDefused = false;

//Time
byte minutesLeft = 1;//change between 0-99
byte secondsLeft = 0;//change between 0-99
static const byte DEFUSER_TIMEOUT_SEC = 15;//seconds until password input is canceled 0-255
byte defuseTimeoutSecLeft = DEFUSER_TIMEOUT_SEC;
//Timers init
Timer<3, millis> timerEverySec;
Timer<1, millis> timerDefuseTimeout;
Timer<1, millis> timerDisplayRefresh;
///Global Var END

///Input help functions START
byte ChB(const char& ch)//char to int
{
  return ch - '0';
}

byte waitInput()//wait until key is pressed
{
  inputKey = NO_KEY;
  do
  {
    inputKey = keypad.getKey();
  }while(inputKey == NO_KEY);
  return ChB(inputKey);
}
byte waitInputWithTimer()//wait until key is pressed, timer keeps refreshing
{
  inputKey = NO_KEY;
  do
  {
    timerEverySec.tick();
    timerDefuseTimeout.tick();
    inputKey = keypad.getKey();
  }while(inputKey == NO_KEY && defuseTimeoutSecLeft > 0);
  return ChB(inputKey);
}
void keySound(char& key)//offsets the default key press sound
{
  if(key != NO_KEY)
  {
    switch (key)
    {
    case '1':
      tone(buzzer, clickSound - 500);
      break;
    case '2':
      tone(buzzer, clickSound - 400);
      break;
    case '3':
      tone(buzzer, clickSound - 300);
      break;
    case '4':
      tone(buzzer, clickSound - 200);
      break;
    case '5':
      tone(buzzer, clickSound - 100);
      break;
    case '6':
      tone(buzzer, clickSound);
      break;
    case '7':
      tone(buzzer, clickSound + 100);
      break;
    case '8':
      tone(buzzer, clickSound + 200);
      break;
    case '9':
      tone(buzzer, clickSound + 300);
      break;
    case '0':
      tone(buzzer, clickSound + 400);
      break;
    case '*':
      tone(buzzer, clickSound + 500);
      break;
    case '#':
      tone(buzzer, clickSound + 600);
      break;
      default: break;
    }
    delay(soundLength);
    noTone(buzzer);
    key = NO_KEY;
  }
}
int arrToInt(const byte* str, byte strSize)//array of bytes to int
{
  int result = 0;
  int temp;
  for(byte i = 0; i < strSize; i++)
  {
    temp = str[i];
    for(byte j = strSize - i - 1; j > 0; j--)
    {
      temp *= 10;
    }
    result += temp;
  }
  return result;
}
///Input help functions END

///Bool checks START
bool checkPassword(const byte* pass1, const byte* pass2)//compares 2 password strings
{
  for(byte i = 0; i < PASSWORD_LENGTH; i++)
  {
    if(pass1[i] != pass2[i]) return false;
  }
  return true;
}

bool checkPasswordResponse(bool isCorrect)
{
  if(isCorrect)
  {
    for(byte i = 0; i < 3; i++)
    {
      delay(250);
      mainDisplay.clear();
      delay(250);
      mainDisplay.showNumberDec(arrToInt(inputPass, PASSWORD_LENGTH));
    }
  }
  else
  {
    delay(250);
    mainDisplay.setSegments(topline);
    delay(250);
    mainDisplay.setSegments(midline);
    delay(250);
    mainDisplay.setSegments(underline);
  }
  return isCorrect;
}

bool isTimeLeft()
{
  return (secondsLeft > 0 || minutesLeft > 0);
}
///Bool checks END

///Password input START
void setUpPassword()
{
  do
  {
    mainDisplay.setSegments(underline);
    for (byte i = 0; i < PASSWORD_LENGTH; i++)
    {
      inputPass[i] = waitInput();
      mainDisplay.showNumberDec(inputPass[i], false, 1, i);
      keySound(inputKey);
      inputKey = NO_KEY;
    }
  }while(!checkPasswordResponse(checkPassword(inputPass, SETUP_PASSWORD)));
}

bool defusePassword()
{
  do
  {
    timerEverySec.tick();
    timerDefuseTimeout.tick();
    if(defuseTimeoutSecLeft <= 0)
    {
      defuseTimeoutSecLeft = DEFUSER_TIMEOUT_SEC;
      return false;
    }
    mainDisplay.setSegments(underline);
    for (byte i = 0; i < PASSWORD_LENGTH; i++)
    {
      inputPass[i] = waitInputWithTimer();
      mainDisplay.showNumberDec(inputPass[i], false, 1, i);
      keySound(inputKey);
      inputKey = NO_KEY;
    }
  }while(!checkPasswordResponse(checkPassword(inputPass, DEFUSE_PASSWORD)));
  return true;
}
///Password input END

///Timer functions START
bool toggleLed(void *)
{
  if(isRedOn)
  {
    digitalWrite(redLed, LOW);
    digitalWrite(greenLed, HIGH);
    isRedOn = false;
  }
  else
  {
    digitalWrite(redLed, HIGH);
    digitalWrite(greenLed, LOW);
    isRedOn = true;
  }
  return (minutesLeft > 0) || (secondsLeft > 0);
}

bool beep(void *)
{
  tone(buzzer, explosionSound);
  delay(soundLength);
  noTone(buzzer);
  return (minutesLeft > 0) || (secondsLeft > 0);
}

bool changeTime(void *)
{
  if(secondsLeft == 0)
  {
    if(minutesLeft == 0)
    {
      return false;
    }
    else
    {
      minutesLeft--;
      secondsLeft = 60;
    }
  }
  else
  {
    secondsLeft--;
  }
  return (minutesLeft > 0) || (secondsLeft > 0);
}

bool refreshDisplay(void *)
{
  mainDisplay.showNumberDecEx(minutesLeft, 0b01000000, true, 2, 0);
  mainDisplay.showNumberDecEx(secondsLeft, 0b01000000, true, 2, 2);
  return true;
}

bool changeTimeoutTime(void *)
{
  if(defuseTimeoutSecLeft > 0)
  {
    defuseTimeoutSecLeft--;
  }
  return true;
}
///Timer functions END

//Main
void setup() {
  //Pin setup
  pinMode(buzzer, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);

  //Timers setup
  timerEverySec.every(1000, toggleLed);
  timerEverySec.every(1000, beep);
  timerEverySec.every(1000, changeTime);
  timerDefuseTimeout.every(1000, changeTimeoutTime);
  timerDisplayRefresh.every(1000, refreshDisplay);

  //Display setup
  mainDisplay.clear();
  mainDisplay.setBrightness(0);

  setUpPassword();
}

void loop() {
  //Refresh timers
  timerEverySec.tick();
  timerDisplayRefresh.tick();
  
  //Try to get input
  inputKey = NO_KEY;
  inputKey = keypad.getKey();
  if(inputKey == '#' || inputKey == '*')
  {
    if(defusePassword())
    {
      digitalWrite(redLed, LOW);
      digitalWrite(greenLed, HIGH);
      mainDisplay.setSegments(xxxx);
      while(true){};//loop forever
    }
  }
  
  //Check time left
  if(!isTimeLeft())
  {
    digitalWrite(redLed, HIGH);
    mainDisplay.showNumberDecEx(0, 0b01000000, true);
    tone(buzzer, explosionSound);
    while(true){};//loop forever
  }
}
