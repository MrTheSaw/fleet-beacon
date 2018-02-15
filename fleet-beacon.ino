/*******************************************
 * User story: User turns on device. User triggers device.
 * Phase1: Devices flashes a variety of colors, with flashes getting slower over time.
 * Phase2: Also, as time goes on, the colors flashed narrow down to red, green or blue.
 * Phase3: Colors are only red green or blue. Flashes are about 1 second long.
 * Phase4: Final color is chosen. It flashes a number of times, and then comes on
 * {solid|dancing} until the user hits the button again.
 * 
 * Total run time: 2-3 minutes.
 * Hitting the button at anytime during the run resets the run.
 * 
 */


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


/***********************************
 * Common globals
 * i.e. Globals which are being used by more than one routine
 */

Adafruit_DotStar pack = Adafruit_DotStar(PACK_SIZE, DATA_PIN, CLOCK_PIN, DOTSTAR_BRG);

unsigned short index = 0;
byte MWindex = 0;

_Bool onP = false;
_Bool buttonHit = false;
uint32_t off;
byte colors[COL_ARR_SIZE][3];
unsigned int current_pixel = 0;
_Bool cleared = true;


/*************************************
 * Run phase handling
 */

//Phase thresholds
byte phase_limits[4] = {150,175,200,250};
typedef enum { PHASE1 = 0, PHASE2, PHASE3, PHASE4 } phase_t;
phase_t phase = PHASE1;

phase_t setPhase(short spacing) {
  for (byte i = 0; i<4; i++) {
    if (phase_limits[i] == spacing) morse_flash(i);
  }
  if (spacing > 0 && spacing < phase_limits[PHASE1]) return PHASE1;
  if (spacing > phase_limits[PHASE1] && spacing < phase_limits[PHASE2]) return PHASE2;
  if (spacing > phase_limits[PHASE2] && spacing < phase_limits[PHASE3]) return PHASE3;
  if (spacing > phase_limits[PHASE3]) return PHASE4;
}

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
    if (colors[color][0] == HiThr) { colors[color][1] = LoThr; colors[color][2] = LoThr;}
    else if (colors[color][1] == HiThr) { colors[color][0] = LoThr; colors[color][2] = LoThr;}
    else if (colors[color][2] == HiThr) { colors[color][0] = LoThr; colors[color][1] = LoThr;}
    else {
      colors[color][random(0,3)] = HiThr;
      mkRGB(color);
    }
}

/**********************************************************
 * Color Mod Functions. These will appear in the loop().
 * Make sure they save state to either a common globals or a reserved global
 */

/**********************
 * Slowly make all colors either red, green, or blue
 */
void hammer(_Bool ensure) {
  //ensure that a color gets changed all the way to r g or b
  short color = random(0, COL_ARR_SIZE); //0-63 IRL, high number is excluded
  if (ensure) {
    mkRGB(color);
  } else {
    colors[color][random(0,3)] = HiThr;
  }
}

// Make everything Exactly Red, Green or Blue
void bigHammer() {
  for (short color=0; color<=COL_ARR_SIZE;color++) {
    mkRGB(color);
  }
}


//This picks the actual last color to be displayed
void finalBlow() {
    uint32_t rgb [3] = { 0xc00000, 0x00c000, 0x0000c0 };
    byte colorUp = 0; // random(0,3);
    for (byte flashes = 5; flashes > 0; flashes--) {
      setPixels(&pack, 0);
      delay(500);
      setPixels(&pack, rgb[colorUp]);
      delay(250);
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

/**************
 * sets all pixels to the same color
 */
void setPixels (Adafruit_DotStar *stars, uint32_t color) {
  for (int i = 0; i < PACK_SIZE; i++) {
    stars->setPixelColor(i, color);
  }
}


void twirl1 (Adafruit_DotStar *stars, uint32_t color) {
  for (int i = 0; i < PACK_SIZE; i++) {
    stars->setPixelColor((i-1)%PACK_SIZE, 0);
    stars->setPixelColor(i, color);
    stars->show();
    delay(32);
  }
  stars->setPixelColor(PACK_SIZE-1,0);
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
  //setPixels(&pack, pack.Color(col[0], col[1], col[2]));
  twirl1(&pack, pack.Color(col[0], col[1], col[2]));
  //pack.show();
  //delay(delay_time);
  MWindex++; //intentionally wrapping around
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

void morse_flash(byte digit) {
  byte where = 0;
  const short dit = 200;
  const short dah = 3*dit;
  const short numbers[10][6] = {
    {dit, dah, dah, dah, dah, -1},
    {dit, dit, dah, dah, dah, -1},
    {dit, dit, dit, dah, dah, -1},
    {dit, dit, dit, dit, dah, -1},
    {dit, dit, dit, dit, dit, -1},
    {dah, dit, dit, dit, dit, -1},
    {dah, dah, dit, dit, dit, -1},
    {dah, dah, dah, dit, dit, -1},
    {dah, dah, dah, dah, dit, -1},
    {dah, dah, dah, dah, dah, -1}
  };

  while (numbers[digit][where] != -1) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(numbers[digit][where]);
    digitalWrite(LED_BUILTIN, LOW);
    where++;
    delay(dah);
  }
  delay(2*dah);
}

void handleStopStartButton (void) {
  buttonHit = true;
}

void turnOn() {
  onP = !onP;
  deBounce();
  buttonHit = false;
}

void deBounce ()
{
  const int debounceTime = 50;
  
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




/********************************************
 * SETUP
 */
void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  onP = false;
  current_pixel = 0;
  MWindex = 0;
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


/*******************************************
 * LOOP
 */
void loop() {

  if (buttonHit == true) turnOn();

  if (onP) {
    cleared = false;
    playMaxWell(spacing);
    spacing++;
    phase = setPhase(spacing);
    switch (phase) {
      case PHASE2: hammer(true);
                   break;
      case PHASE3: bigHammer();
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
      morse_flash(9);
    }
  }
}
  
