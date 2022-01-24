#pragma once

#include "Arduino.h"

#define minWPM  5
#define maxWPM  45

typedef struct
{
  const     char       Char;
  const     char       *Code;
} Pattern;

const Pattern  patterns[] = 
{
  {'A',".-"},  {'B',"-..."},{'C',"-.-."},{'D',"-.."}, {'E',"."},
  {'F',"..-."},{'G',"--."}, {'H',"...."},{'I',".."},  {'J',".---"},
  {'K',"-.-"}, {'L',".-.."},{'M',"--"},  {'N',"-."},  {'O',"---"},
  {'P',".--."},{'Q',"--.-"},{'R',".-."}, {'S',"..."}, {'T',"-"},
  {'U',"..-"}, {'V',"...-"},{'W',".--"}, {'X',"-..-"},{'Y',"-.--"},
  {'Z',"--.."},
  {'1',".----"},{'2',"..---"}, {'3',"...--"},{'4',"....-"},{'5',"....."},
  {'6',"-...."},{'7',"--..."}, {'8',"---.."},{'9',"----."},{'0',"-----"},
  {'=',"-...-"},{'?',"..--.."},{'/',"-..-."},
  {' ',""},
  {0,""}
};

class Morse
{
  private:
    int KeyPin      = 14;
    bool ActiveHigh = true;
    int MarkT       = 120;
    int CharGap     = 3;
    bool Keyed      = false;
    unsigned long KeyedTime;
    int MaxKeyedTime = 500;
    void (*KeyIsDown)(void) = NULL;
    void (*KeyIsUp)(void) = NULL;
  public:
    void begin(int pin, bool activehigh) 
    {
      KeyPin = pin;
      ActiveHigh = activehigh;
      pinMode(KeyPin, OUTPUT);
      KeyUp();
    }
    void update(void) 
    {
      if(Keyed)
      {
        if((millis() - KeyedTime) > MaxKeyedTime) KeyUp();
      }
    }
    int wpm(void) { return(1200 / MarkT); }
    void wpm(int w) { MarkT = 1200 / w; }
    void attachKeyDown(void (*fun)(void)) { KeyIsDown = fun; }
    void attachKeyUp(void (*fun)(void)) { KeyIsUp = fun; }
    void detachKeyDown(void) { KeyIsDown = NULL; }
    void detachKeyUp(void) { KeyIsUp = NULL; }
    void KeyDown(void)
    {
      if(ActiveHigh) digitalWrite(KeyPin, HIGH);
      else digitalWrite(KeyPin, LOW);
      Keyed = true;
      KeyedTime = millis();
      if(KeyIsDown != NULL) KeyIsDown();
    }
    void KeyUp(void)
    {
      if(ActiveHigh) digitalWrite(KeyPin, LOW);
      else digitalWrite(KeyPin, HIGH);
      Keyed = false;
      if(KeyIsUp != NULL) KeyIsUp();
    }
    void check(void)
    {
      if(Keyed)
      {
        if(millis() > (KeyedTime + MaxKeyedTime)) KeyUp();
      }
    }
    void Dit(void)
    {
      KeyDown();
      delay(MarkT);
      KeyUp();
      delay(MarkT);  
    }
    void Dash(void)
    {
      KeyDown();
      delay(MarkT * CharGap);
      KeyUp();
      delay(MarkT);    
    }
    void SendMorseChar(char c)
    {
      for(int i=0; patterns[i].Char != 0; i++)
      {
        if(c == patterns[i].Char)
        {
          for(int j=0; patterns[i].Code[j] != 0; j++)
          {
            if(patterns[i].Code[j] == '.') Dit();
            else if(patterns[i].Code[j] == '-') Dash();
          }
          delay(MarkT * CharGap);
          return;
        }
      }
    }
    void SendMorseString(char *c) { for(int i=0; c[i] != 0; i++) SendMorseChar(c[i]); }
};
