//  keyer.cpp - Iambic keyer
//
// Keyer iambic modes:
//  A - When both paddles are released the current dit or dah is finished.
//  B - When both paddles are released the current dit or dah is finished and then
//      the opposite is sent. 
//  Ultimatic - When squeezed the last paddle detected defines the repeating element
/*
 * 
Non iambic mode

  If dit paddle pressed
    - generate dit and loop while pressed
  if dah paddle pressed
    - generate dah and loop while pressed

Iambic A mode

  If both paddles are released then cancel the insert
  If dit paddle pressed or insert dit
    - generate dit
      - during dit generation look for dah, set insert dah if detected
  If dah paddle pressed or insert dah
    - generate dah
      - during dit generation look for dah, set insert dit if detected

Iambic B mode

  If dit paddle pressed or insert dit
    - generate dit
      - during dit generation look for dah, set insert dah if detected
  If dah paddle pressed or insert dah
    - generate dah
      - during dit generation look for dah, set insert dit if detected

Ultamatic mode

  if both paddles pressed and insert dit set
    - generate dir
  if both paddles pressed and insert dah set
    - generate dah
  If dit paddle pressed
    - generate dit and loop while pressed
    - If dah paddle is pressed exit and set insert dah
  if dah paddle pressed
    - generate dah and loop while pressed
    - if dit paddle is pressed exit and set insert dit * 
 * 
 */

#include "Keyer.h"

Keyer::Keyer()
{
    KeyIsDown = KeyIsUp = SendingDit = SendingDah = NULL;
    insertDit = insertDah = false;
    activeLow = false;
    STenable = true;
    DDmode = false;
}

int Keyer::getSpeed(void)
{
    return WPM;
}

void Keyer::setSpeed(int newWPM)
{
    WPM = newWPM;
    ditTime = 1200 / WPM;
}

int Keyer::getSidetoneFreq()
{
    return sidetoneFreq;
}

void Keyer::setSidetoneFreq(int freq)
{
    sidetoneFreq = freq;
}

// Initialize the I/O pins used by the keyer
//
void Keyer::begin(int straightKey, int dit, int dah, int key, int sidetone)
{
    ditPin.begin(dit);
    dahPin.begin(dah);
    straightKeyPin.begin(straightKey);
    keyPin      = key;
    sidetonePin = sidetone;

    pinMode(keyPin, OUTPUT);
    pinMode(sidetonePin, OUTPUT);

    WPM   = defaultWPM;
    Mode  = defaultMode;
    ditTime = 1200 / WPM;
    sidetoneFreq = defaultSidetoneFreq;

    keyUp();
}

void Keyer::begin()
{
    begin(defaultStraightKeyPin, defaultDitPin, defaultDahPin, defaultKeyPin, defaultSidetonePin);
}

void Keyer::delayAndWatchKey(int duration)
{
    if (duration > 0)
    {
        endTime = millis() + duration;
        while (millis() < endTime)
        {
          if(ditPin.down()) insertDit = true;
          if(dahPin.down()) insertDah = true;
          delay(1);
        }
    }
}

void Keyer::sendDit()
{
    if(SendingDit != NULL) SendingDit();
    keyDown(DDmode);
    delayAndWatchKey(ditTime);
    keyUp(DDmode);
    delayAndWatchKey(ditTime);
}

void Keyer::sendDah()
{
    if(SendingDah != NULL) SendingDah();
    keyDown(DDmode);
    delayAndWatchKey(3 * ditTime);
    keyUp(DDmode);
    delayAndWatchKey(ditTime);
}

void Keyer::keyDown(bool noSend)
{
    if (activeLow) digitalWrite(keyPin, LOW);
    else digitalWrite(keyPin, HIGH);
    isDown = true;
    if((KeyIsDown != NULL) && (!DDmode)) KeyIsDown();
    if(STenable) tone(sidetonePin, sidetoneFreq);
}

void Keyer::keyUp(bool noSend)
{
    if (activeLow) digitalWrite(keyPin, HIGH);
    else digitalWrite(keyPin, LOW);
    isDown = false;
    if((KeyIsUp != NULL) && (!DDmode)) KeyIsUp();
    noTone(sidetonePin);
}

void Keyer::iambic(void)
{
    switch (Mode)
    {
      case ModeNonIambic:
        if(insertDit || ditPin.down()) 
        {
          sendDit();
          insertDit = false;
        }
        if(insertDah || dahPin.down())
        {
          sendDah();
          insertDah = false;
        }
        //if(ditPin.down()) sendDit();
        //else if(dahPin.down()) sendDah();
        //insertDit = insertDah = false;
        break;
      default:
        break;
    }
}

void Keyer::process(void)
{
    if(straightKeyPin.down()) if(!isDown) keyDown();
    if(straightKeyPin.up()) if(isDown) keyUp();
    iambic();
    delay(1);
}
