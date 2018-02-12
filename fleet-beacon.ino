/**********************************************************************************
 * Coding convention: Globals needed by more than one function are define on the common globals area
 * Variables which need to persist between loop iterations are declared static.
 * 
 */

#include <Adafruit_DotStar.h>
#include <avr/power.h>

/************************************
 * Hardware defined constants.
 * Resist the urge to make these other than literals. The compiler can make literals
 * into the right thing for you.
 */

#define BUTTON 3
#define CLOCK_PIN 5
#define DATA_PIN 4
//Unnconnected pin floats, used for random seed noise
#define UNCONNECTED_PIN 8
//how many dotstars or equiv
#define PACK_SIZE 8
#define COL_ARR_SIZE 64
//Brightness upper threshold, raise of OVERALL brightness is too low. 255 is MAX, higher might produce errors
#define HiThr 192
#define LoThr 64

//Phase thresholds
byte phase_limits[4] = {150,175,200,250};
typedef enum { PHASE1 = 0, PHASE2, PHASE3, PHASE4 } phase_t;
phase_t phase = PHASE1;


/***********************************
 * Common globals
 * i.e. Globals which are being used by more than one routine
 */

Adafruit_DotStar pack = Adafruit_DotStar(PACK_SIZE, DATA_PIN, CLOCK_PIN, DOTSTAR_BRG);

unsigned short index = 0;
byte MWindex = 0;

_Bool onP = false;
uint32_t off;
byte colors[COL_ARR_SIZE][3];
unsigned int current_pixel = 0;


/************************
 * utility functions
 */
 
//For future brightness adjustment
int brt(short color_segment) {
  return (int)(color_segment*.75);
}

//Sets the bytes to random values within a threhold to get a variety of colors
void randomize_color_array() {
  for (short outer = 0; outer <= COL_ARR_SIZE - 1; outer++) {
    for (short inner = 0; inner <= 2; inner++) {
      colors[outer][inner] = random(64,190);
    }
  }
}

//set one of the bytes to the top threshold.
//If "definitely" is true, set the other bytes to 0
void mkRGB(short color) {
    if (colors[color][0] = HiThr) { colors[color][1] = LoThr; colors[color][2] = LoThr;}
    else if (colors[color][1] = HiThr) { colors[color][0] = LoThr; colors[color][2] = LoThr;}
    else if (colors[color][2] = HiThr) { colors[color][0] = LoThr; colors[color][1] = LoThr;}
    else {
      colors[color][random(0,3)] = HiThr;
      mkRGB(color);
    }
}

/*******************************
 * Color Mod Functions. These will appear in the loop().
 * Make sure they save state to either a common globals or a reserved global
 */

/**********************
 * Slowly make all colors either red, green, or blue
 */
void hammer(_Bool ensure) {
  //ensure that a color gets changed all the way to r g or b
  short color = random(0,COL_ARR_SIZE); //0-63 IRL, high number is excluded
  if (ensure) {
    mkRGB(color);
  } else {
    colors[color][random(0,3)] = HiThr;
  }
}

/*****************************
 * Make everything Exactly Red, Green or Blue
 */
void bigHammer() {
  for (short color=0; color<=COL_ARR_SIZE;color++) {
    mkRGB(color);
  }
}

void finalBlow() {
    byte colorUp = random(0,3);
    uint32_t color = 0xff << 8*colorUp;
    for (byte d = 5; d > 0; d--) {
      pack.clear();
      delay(120);
      setPixels(&pack, color);
      delay(120);
    }
    delay(5000);
    onP = false; 
}

//Select next color

uint32_t next_color() {
  static short r=0,g=0,b=0;
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
  return pack.Color(r,g,b);
}

/******
void playList (short delay_time) {
    if (index > 6) index = 0;
  setPixels(pack, next_color());
  pack.show();
  delay(delay_time);
  index++;
}
******/

/********
 * Slowly becomes more orderly
 */
void playMaxWell (short delay_time) {
  byte * col = colors[MWindex];
  setPixels(&pack, pack.Color(col[0], col[1], col[2]));
  pack.show();
  delay(delay_time);
  MWindex++; //intentionally wrapping around
}



/**************
 * sets all pixels to the same color
 */
void setPixels (Adafruit_DotStar *stars, uint32_t color) {
  for (int i = 0; i < PACK_SIZE; i++) {
    stars->setPixelColor(i, color);
  }
}




int button_time = 0;
_Bool oldButtonState = HIGH;
short spacing = 100;

void flash_diag(short flashes) {
  for (short i = 0; i< flashes; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(60);
    digitalWrite(LED_BUILTIN, LOW);
    delay(60);
  }
  delay(120); //make some spacing
}

void handleStopStartButton (void) {
  onP = !onP;
}


void deBounce ()
{
  const int debounceTime = 20;
  
  unsigned long now = millis ();
  do
  {
    // on bounce, reset time-out
    if (digitalRead (BUTTON) == LOW)
      now = millis ();
  } 
  while (digitalRead (BUTTON) == LOW ||
    (millis () - now) <= debounceTime);

}  // end of deBounce


phase_t setPhase(short spacing) {
  if (spacing > 0 && spacing < phase_limits[PHASE1]) return PHASE1;
  if (spacing > phase_limits[PHASE1] && spacing < phase_limits[PHASE2]) return PHASE2;
  if (spacing > phase_limits[PHASE2] && spacing < phase_limits[PHASE3]) return PHASE3;
  if (spacing > phase_limits[PHASE3] && spacing < phase_limits[PHASE4]) return PHASE4;
}

/********************************************
 * SETUP
 */
void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  onP = false;
  attachInterrupt(digitalPinToInterrupt(BUTTON), handleStopStartButton, FALLING);
  randomSeed(analogRead(UNCONNECTED_PIN));
  randomize_color_array();
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  float bright = .75;
  
  off = pack.Color(0,0,0);
  pack.begin();
  pack.show();
}

void reset() {
    pack.clear();
    pack.show();
    spacing = 100;
    randomize_color_array();
}



_Bool cleared = true;
/*******************************************
 * LOOP
 */
void loop() {

  //REPLACE with interrupt
/*  if ((digitalRead(BUTTON) == LOW) && (oldButtonState == HIGH)) {
      onP = !onP;
      digitalWrite(LED_BUILTIN, HIGH);
      oldButtonState = LOW;
    } else if ((digitalRead(BUTTON) == HIGH) && (oldButtonState == LOW)) {
      digitalWrite(LED_BUILTIN, LOW);
      oldButtonState = HIGH;
    }
    delay(5);
*/
  if (onP) {
    deBounce();
    cleared = false;
    playMaxWell(spacing);
    spacing++;
    phase = setPhase(spacing);
    switch (phase) {
      case PHASE2: hammer(true);
                   break;
      case PHASE3: bigHammer();
                   flash_diag(5);
                   break;
      case PHASE4: finalBlow();
                   break;
      default: hammer(false); //PHASE1
               break;
    }
  // If not on  
  } else {
    //clean up if canceled
    if (!cleared) {
      reset();
      cleared = true;
      flash_diag(10);
    }
  }
}
  
