#ifndef button_h
#define button_h

#include "Arduino.h"

class Button
{
  private:
    uint8_t btn;
    uint16_t state;
  public:
    void begin(uint8_t button) 
    {
      btn = button;
      state = 0;
      pinMode(btn, INPUT_PULLUP);
    }
    bool pressed() 
    {
      state = (state<<1) | digitalRead(btn) | 0xfe00;
      return (state == 0xff00);
    }
    bool released()
    {
      state = (state<<1) | digitalRead(btn) | 0xfe00;
      return (state == 0xfeff);       
    }
    bool down() 
    {
      state = (state<<1) | digitalRead(btn) | 0xfe00;
      return (state == 0xfe00);             
    }
    bool up() 
    {
      state = (state<<1) | digitalRead(btn) | 0xfe00;
      return (state == 0xffff);                   
    }
};
#endif
