#include <Adafruit_NeoPixel.h>
#include "led.h"

Led::Led(Adafruit_NeoPixel* pixels) {
  this->_pixels = pixels;
  this->_pixels->begin();
}

void Led::on(int r, int g, int b) {
  this->_pixels->setPixelColor(0, this->_pixels->Color(r, g, b));
  this->_pixels->setPixelColor(1, this->_pixels->Color(r, g, b));
  this->_pixels->show();
}

void Led::off() {
  this->_pixels->clear();
}

void Led::blink(int times, int on, int off, int r, int g, int b) {
  for(int i = 0; i < times; i++) {
    this->_pixels->setPixelColor(0, this->_pixels->Color(r, g, b));
    this->_pixels->setPixelColor(1, this->_pixels->Color(r, g, b));
    this->_pixels->show();
    delay(on);
    this->_pixels->clear();
    delay(off);
  }
}

void Led::blinkChanging(int times, int on, int off, int r, int g, int b) {
  for(int i = 0; i < times; i++) {
    this->_pixels->clear();
    this->_pixels->setPixelColor(0, this->_pixels->Color(r, g, b));
    this->_pixels->show();
    delay(on);
    this->_pixels->clear();
    this->_pixels->setPixelColor(1, this->_pixels->Color(r, g, b));
    this->_pixels->show();
    delay(off);
  }
  this->_pixels->clear();
}
