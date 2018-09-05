#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

//pins
#define BTN_INCREMENT_STEP 2
#define BTN_INCREMENT_ROTATIONS 4
#define BTN_DECREMENT_ROTATIONS 6
#define BTN_START 8
#define RELAY_PIN 12
//end pins

//settings

//POSITIONS
#define STEP_POSITION_LINE 0
#define STEP_POSITION_COLUMN 1
#define ROTATION_COUNT_LINE 9
#define ROTATION_COUNT_COLUMN 1
#define LCD_WIDTH 16
//END POSITIONS

#define PRESSED_TOLERANCE 500  //ms
#define MAX_ROTATIONS 3000
#define DISTANCE_BETWEEN_CLONE_DATES 4
//end settings

//variables
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
unsigned short currentStep = 0;
unsigned short currentRotations = 0;
//end variables

//aux variables
long lastMillisIncrementStepPressed = 0;
bool incrementStepPressed = false;

long lastMillisIncrementRotationsPressed = 0;
bool incrementRotationsPressed = false;

long lastMillisDecrementRotationsPressed = 0;
bool decrementRotationsPressed = false;

long lastMillisStartPressed = 0;
bool startPressed = false;

unsigned short lastSecondShown  = 0;
unsigned short dateTimePosition = 0;

bool motorActive = false;
unsigned long motorActiveStartTimeMilis = 0;
unsigned short totalRotatingTimeSeconds = 0;
//end aux variables
void setup() 
{
  Serial.begin(9600);
  Serial.println("Starting...");
  pinMode(BTN_INCREMENT_STEP, INPUT_PULLUP);
  pinMode(BTN_INCREMENT_ROTATIONS, INPUT_PULLUP);
  pinMode(BTN_DECREMENT_ROTATIONS, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);

  //digitalWrite(RELAY_PIN, LOW);
  lcd.begin(16,2);
  lcd.clear(); 

  printCurrentStep();
  printTotalRotations();

  dateTimePosition = random(0,15);
}



void loop() {
   tmElements_t tm;
   incrementStepPressed = digitalRead(BTN_INCREMENT_STEP) == true ? false : true;
   incrementRotationsPressed = digitalRead(BTN_INCREMENT_ROTATIONS) == true ? false : true;
   decrementRotationsPressed = digitalRead(BTN_DECREMENT_ROTATIONS) == true ? false : true;
   startPressed = digitalRead(BTN_START) == true ? false : true;
   long nowMillis = millis();
    
    if(!motorActive && incrementStepPressed && nowMillis - lastMillisIncrementStepPressed > PRESSED_TOLERANCE )
    {
        incrementCurrentStep();
        lastMillisIncrementStepPressed = nowMillis;
    }

    if( !motorActive && incrementRotationsPressed && nowMillis - lastMillisIncrementRotationsPressed > PRESSED_TOLERANCE )
    {
        incrementCurrentRotations();
        lastMillisIncrementRotationsPressed = nowMillis;
    }

    if( !motorActive && decrementRotationsPressed && nowMillis - lastMillisDecrementRotationsPressed > PRESSED_TOLERANCE )
    {
        decrementCurrentRotations();
        lastMillisDecrementRotationsPressed = nowMillis;
    }

    if(startPressed && nowMillis - lastMillisStartPressed > PRESSED_TOLERANCE && !motorActive && currentRotations)
    {
        startRotation();
        lastMillisStartPressed = nowMillis;
    }

    checkIfFinishedRotating();
    checkForForceStopRotating();
    printTimeOnLCD();

    if( motorActive){
      printRemainingRotationTime();
    }
}

void checkForForceStopRotating()
{
  if( motorActive && startPressed && incrementStepPressed ){
    stopRotating();
    delay(300);
  }
}

void checkIfFinishedRotating()
{
  if (motorActive)
  {
    unsigned short timeRotatedUntilNowSeconds = ( millis() - motorActiveStartTimeMilis ) / 1000;
    if(timeRotatedUntilNowSeconds >= totalRotatingTimeSeconds){
      stopRotating();
    }
  }
}

void stopRotating(){
   pinMode(RELAY_PIN, INPUT);
   motorActive = false;
   motorActiveStartTimeMilis = 0;
   clearBottomRow();
   clearTotalRotation();
   clearCurrentStep();
   Serial.println("Motor stopped.");
}

void clearBottomRow(){
  lcd.setCursor(0,1);
  lcd.print("                ");
}

void startRotation()
{
   pinMode(RELAY_PIN, OUTPUT);
   motorActive = true;
   motorActiveStartTimeMilis = millis();
   Serial.println("Current rotations: " + String(currentRotations));
   totalRotatingTimeSeconds = 6 * currentRotations;
   clearBottomRow();
}

void incrementCurrentRotations()
{
  if(currentRotations + currentStep > MAX_ROTATIONS )
  {
    currentRotations = MAX_ROTATIONS;
  }
  else
  {
    currentRotations += currentStep;
  }
  printTotalRotations();
}

void clearTotalRotation()
{
  currentRotations = 0;
  printTotalRotations();
}

void decrementCurrentRotations()
{
  if(currentRotations < currentStep )
  {
    currentRotations = 0;
  }
  else
  {
    currentRotations -= currentStep;
  }
  printTotalRotations();
}

void clearCurrentStep(){
  currentStep = 0;
  printCurrentStep();
}

void incrementCurrentStep()
{
  switch(currentStep){
    case 0:
    case 500:
      currentStep = 50;
      break;
    case 50:
      currentStep = 100;
      break;
    case 100:
      currentStep = 500;
      break;
    default:// if reached here, I was wrong
      currentStep = 0;
    break;
  }
  printCurrentStep();
}

void printTotalRotations(){
  String text = "T:";
  lcd.setCursor(ROTATION_COUNT_LINE, ROTATION_COUNT_COLUMN);
  lcd.print("T:        ");
  if(currentRotations < 100){//2 digits
    text += " ";
    if(currentRotations < 10 ){//1 digit
       text += " ";
    }
  }
  
  text += String(currentRotations);
  lcd.setCursor(ROTATION_COUNT_LINE, ROTATION_COUNT_COLUMN);
  lcd.print(text);
}

void get2DigitsCharArray(unsigned short number, char * out) {

  out[2] = 0;
  if( number == 0 )
  { 
    out[0] = '0';
    out[1] = '0';
    return;
  }
  if( number < 0 || number > 59 ) // shet error
  {
    out[0] = 'X';
    out[1] = 'X';
    return;
  }
  char * toAscii = itoa(number,out,10);
  if( number > 9 ) /// 2 digits
  { 
    out[0] = toAscii[0];
    out[1] = toAscii[1];
  }
  else{ // number < 10, 1 digit
    char auxBuff[2];
    out[0] = '0';
    out[1] = itoa(number,auxBuff,10)[0];
  }
}

void nextDateTimePosition(unsigned short & pos,unsigned short length){
  if( pos == length )
    pos = 1;
  else
    ++pos;
}

void printTimeOnLCD(){
  tmElements_t tm;
  char twoDigitBuffer[3];
  String dateTime = "ERROR";
  if (RTC.read(tm)){
    if(lastSecondShown == tm.Second)
      return;

    lastSecondShown = tm.Second;
    //HH:mm:ss dd/MM/YYYY
    //hour
    get2DigitsCharArray(tm.Hour,twoDigitBuffer);
    dateTime = twoDigitBuffer;

    //minute
    get2DigitsCharArray(tm.Minute,twoDigitBuffer);
    dateTime += ":";
    dateTime += twoDigitBuffer;

    //seconds
    get2DigitsCharArray(tm.Second,twoDigitBuffer);
    dateTime += ":";
    dateTime += twoDigitBuffer;

    //space
    dateTime += " ";

    //day
    get2DigitsCharArray(tm.Day,twoDigitBuffer);
    dateTime += twoDigitBuffer;

    //month
    get2DigitsCharArray(tm.Month,twoDigitBuffer);
    dateTime += "/";
    dateTime += twoDigitBuffer;
    
    //year
    char buffUseless[10];
    unsigned short year = tmYearToCalendar(tm.Year);
    dateTime += "/";
    dateTime += itoa(year,buffUseless,10);
  }
  
  nextDateTimePosition(dateTimePosition, dateTime.length() + DISTANCE_BETWEEN_CLONE_DATES );
  String textShown = dateTime.substring(dateTimePosition);
  short remainingSpace = LCD_WIDTH - textShown.length() - DISTANCE_BETWEEN_CLONE_DATES;

  if(remainingSpace == 12 && dateTimePosition > 18 ){
    for(int i = 0; i< 23 - dateTimePosition; ++i)
      {
        textShown += " ";
      }
       textShown += dateTime.substring(0,dateTimePosition - (23 - dateTimePosition));
  }
  else if ( remainingSpace > 0 )
  {
      for(int i = 0; i< DISTANCE_BETWEEN_CLONE_DATES; ++i)
      {
        textShown += " ";
      }
       textShown += dateTime.substring(0,remainingSpace);
  }

  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(textShown);
}

unsigned short getRemainingRotationTimeSeconds(){ 
  unsigned short motorActiveStartTimeSeconds = motorActiveStartTimeMilis / 1000;
  unsigned long nowMillis = millis();
  unsigned short nowSeconds = nowMillis / 1000;
  unsigned short alreadyPassedSeconds = nowSeconds - motorActiveStartTimeSeconds;
  return totalRotatingTimeSeconds - alreadyPassedSeconds;
}

static unsigned short lastRemainingSecondShown = 0;
void printRemainingRotationTime()
{
  String text = "T left: ";
  char twoDigitBuffer[3];
  //HH
  unsigned short timeLeftInSeconds = getRemainingRotationTimeSeconds();
  
  unsigned short hoursLeft = timeLeftInSeconds >= 3600 ? timeLeftInSeconds/3600 : 0;
  unsigned short minutesLeft = ( timeLeftInSeconds % 3600 ) / 60;
  unsigned short secondsLeft = timeLeftInSeconds - (minutesLeft * 60 ) - ( hoursLeft * 3600 );
  
  if ( secondsLeft == lastRemainingSecondShown)
    return;

  //hours
  get2DigitsCharArray(hoursLeft,twoDigitBuffer);
  text += twoDigitBuffer;

  //minutes
  get2DigitsCharArray(minutesLeft,twoDigitBuffer);
  text += ":";
  text += twoDigitBuffer;
  
  //seconds
  get2DigitsCharArray(secondsLeft,twoDigitBuffer);
  text += ":";
  text += twoDigitBuffer;

  lcd.setCursor(0,1);
  lcd.print(text);
  lastRemainingSecondShown = secondsLeft;

  Serial.println("Showing remaining time: " + text );
}


void printCurrentStep(){
  Serial.println("Current step : " + String(currentStep));
  String text = "R:";
  if(currentStep < 100)
  {
    text += " ";
    if(currentStep < 10 )
      text += " ";
  }
  text += String(currentStep);
  lcd.setCursor(STEP_POSITION_LINE, STEP_POSITION_COLUMN);
  lcd.print(text);

}

