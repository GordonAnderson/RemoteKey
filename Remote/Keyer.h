// Keyer.h - Morse code iambic keyer library
//
//

#pragma once

#include <Arduino.h>

#include "Button.h"

// Defaults
#define defaultDitPin          14
#define defaultDahPin          12
#define defaultSidetonePin     13
#define defaultKeyPin          15
#define defaultStraightKeyPin  4

#define defaultSidetoneFreq    700;

#define defaultWPM             25

#define defaultMode            ModeNonIambic

// Limits
#define maxWPM                 60
#define minWPM                 10

enum KeyerModes
{
  ModeNone,
  ModeNonIambic,
  ModeIambicA,
  ModeIambicB,
  ModeUltimatic
};

class Keyer
{
    public:
        Keyer();
        void begin();
        void begin(int straightKey, int dit, int dah, int key, int sidetone);

        void process(void);
        int  getSpeed(void);
        void setSpeed(int newWPM);
        bool getDDmode(void) { return(DDmode); }
        void setDDmode(bool md) { DDmode=md; }
        int  getSidetoneFreq();
        void enableSidetone(bool state) { STenable = state; }
        void setSidetoneFreq(int newFreq);

        void attachKeyDownCallBack(void (*fun)(void)) { KeyIsDown = fun; }
        void attachKeyUpCallBack(void (*fun)(void)) { KeyIsUp = fun; }
        void attachSendingDitCallBack(void (*fun)(void)) { SendingDit = fun; }
        void attachSendingDahCallBack(void (*fun)(void)) { SendingDah = fun; }
    private:       
        // Call backs
        void (*KeyIsDown)(void);
        void (*KeyIsUp)(void);
        void (*SendingDit)(void);
        void (*SendingDah)(void);
        
        int WPM;
 
        KeyerModes Mode;
        bool STenable;
        bool insertDit;
        bool insertDah;
        bool activeLow;
        bool isDown;
        bool DDmode;
        int  ditTime;
        
        Button ditPin;
        Button dahPin;
        Button straightKeyPin;
        int keyPin;
        int sidetonePin;

        int sidetoneFreq;

        uint32_t endTime;

        void iambic(void);
        void delayAndWatchKey(int duration);
        void sendDit(void);
        void sendDah(void);
        void keyDown(bool noSend = false);
        void keyUp(bool noSend = false);
};
