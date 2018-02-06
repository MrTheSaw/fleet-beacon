#include <Adafruit_NeoPixel.h>

#define DATA_PIN 0
#define BUTTON 2
#define UNCONNECTED_PIN 4
#define COL_ARR_SIZE 64
#define HiThr 192

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, DATA_PIN, NEO_GRB + NEO_KHZ800);
uint32_t roygbiv[7];
unsigned short index = 0;
byte MWindex = 0;

uint32_t off;
byte colors[COL_ARR_SIZE][3];
byte phase = 0;

int brt(short color_segment) {
  return (int)(color_segment*.75);
}

//Sets the bytes to random values within a threhold to get a variety of colors
void randomize_color_array() {
  for (short outer = 0; outer <= COL_ARR_SIZE - 1; outer++) {
    for (short inner = 0; inner <= 2; inner++) {
      colors[outer][inner] = random(64,193);
    }
  }
}

//set one of the bytes to the top threshold.
//If "definitely" is true, set the other bytes to 0
void mkRGB(short color) {
    if (colors[color][0] = HiThr) { colors[color][1] = 0; colors[color][2] = 0;}
    else if (colors[color][1] = HiThr) { colors[color][0] = 0; colors[color][2] = 0;}
    else if (colors[color][2] = HiThr) { colors[color][0] = 0; colors[color][1] = 0;}
    else {
      colors[color][random(0,3)] = HiThr;
      mkRGB(color);
    }
}

//Slowly make all colors either red, green, or blue
void hammer(_Bool ensure) {
  //ensure that a color gets changed all the way to r g or b
  short color = random(0,64);
  if (ensure) {
    mkRGB(color);
  } else {
    colors[color][random(0,3)] = HiThr;
  }
}

void bigHammer() {
  for (short i=0; i<=COL_ARR_SIZE;i++) {
    
  }
}
void setup() {
  randomSeed(analogRead(UNCONNECTED_PIN));
  randomize_color_array();
  pinMode(DATA_PIN, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  float bright = .75;
  roygbiv[0] = strip.Color(brt(0xff), 0, 0);
  roygbiv[1] = strip.Color(brt(0xff), brt(0x80), 0);
  roygbiv[2] = strip.Color(brt(0xff), brt(0xff) ,0);
  roygbiv[3] = strip.Color(0, brt(0xff), 0);
  roygbiv[4] = strip.Color(0, 0, brt(0xff));
  roygbiv[5] = strip.Color(brt(0x68), brt(0x14), brt(0xff));
  roygbiv[6] = strip.Color(brt(0xb3), 0, brt(0xff));

  
  off = strip.Color(0,0,0);
  strip.begin();
  strip.show();
}

//Select next color
static short r=0,g=0,b=0;
uint32_t next_color() {
  byte offset = 64;
  if (b > 255) {
    b=0;
    g+=offset;
  } else b+=offset;
  if (g > 255) {
    g=0;
    r+=offset;
  }
  if (r > 255) {
    r=g=b=0;
  }
  return strip.Color(r,g,b);
}

void playList (short delay_time) {
    if (index > 6) index = 0;
  strip.setPixelColor(0, next_color());
  strip.show();
  delay(delay_time);
  index++;
}


void playMaxWell (short delay_time) {
  byte * col = colors[MWindex];
  strip.setPixelColor(0, col[0],col[1], col[2]);
  strip.show();
  delay(delay_time);
  MWindex++; //intentionally wrapping around
}

void all_off() {
  strip.setPixelColor(0, strip.Color(0,0,0));
  strip.show();
}

_Bool isPressed(short pin) {
  if (digitalRead(pin) == LOW) return true;
}

_Bool onP = false;
int button_time = 0;
_Bool oldButtonState = HIGH;
short spacing = 100;

void loop() {
  if ((digitalRead(BUTTON) == LOW) && (oldButtonState == HIGH)) {
      onP = !onP;
      digitalWrite(1,HIGH);
      oldButtonState = LOW;
    } else if ((digitalRead(BUTTON) == HIGH) && (oldButtonState == LOW)) {
      digitalWrite(1,LOW);
      oldButtonState = HIGH;
    }
    delay(5);

  if (onP) {
    playMaxWell(spacing);
    spacing++;
    if (spacing > 200) {
      hammer(true);
    }
    else if (spacing > 300) {
      bigHammer();
    }
    else {
      hammer(false);
    }
  } else {
    all_off();
    spacing = 100;
    randomize_color_array();
  }
}
