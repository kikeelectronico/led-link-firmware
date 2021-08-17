#define LED_PIN 5
#define NUMBER_PIXELS 2

class Led {
  private:
    Adafruit_NeoPixel* _pixels;
  public:
    Led(Adafruit_NeoPixel* pixels);
    void on(int r, int g, int b);
    void off();
    void blink(int times, int on, int off, int r, int g, int b);
    void blinkChanging(int times, int on, int off, int r, int g, int b);
};
