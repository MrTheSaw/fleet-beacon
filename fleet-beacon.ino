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

#define FB_DEBUG 0

/**********************************************************************************
 * Coding convention: Globals needed by more than one function are define on the common globals area
 * Variables which need to persist between loop iterations are declared static.
 * 
 */

#include <Adafruit_DotStar.h>
#include <LiquidCrystal.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>


/************************************
 * Hardware defined constants.
 * Resist the urge to make these other than literals. The compiler can make literals
 * into the right thing for you.
 */

#define BUTTON 3
#define CLOCK_PIN 5
#define DATA_PIN 4
#define BUTTON_LED 9
//Unnconnected pin floats, used for random seed noise
#define UNCONNECTED_PIN 8
//how many dotstars or equiv
#define PACK_SIZE 8
#define COL_ARR_SIZE 64
//Brightness upper threshold, raise of OVERALL brightness is too low.
//255 is MAX, higher might produce errors
#define HiThr 255
#define LoThr 64


/***********************************
 * Common globals
 * i.e. Globals which are being used by more than one routine
 */

auto pack = Adafruit_DotStar(PACK_SIZE, DATA_PIN, CLOCK_PIN, DOTSTAR_BGR);

#if FB_DEBUG
LiquidCrystal lcd(8,6,9,10,11,12);

void dbg_lcdwrite(uint8_t x, uint8_t y, const char message[]) {
  lcd.setCursor(x,y);
  lcd.print(message);
}

void dbg_lcdspin(uint8_t x, uint8_t y, const char spin_char) {
  lcd.setCursor(x,y);
  lcd.print(spin_char);
}

void dbg_lcdwrite(uint8_t x, uint8_t y, int value) {
  lcd.setCursor(x,y);
  lcd.print(value);
}


#else
//No debug, define these funcations out of existance.
#define dbg_lcdwrite(x,y,z) 
#define dbg_lcdspin(x,y,z) 
//void dbg_lcdwrite(uint8_t x, uint8_t y, const char[] message) {}
//void dbg_lcdwrite(uint8_t x, uint8_t y, int value) {}
#endif


uint32_t loop_count = 0;
unsigned short index = 0;
uint8_t  MWindex = 0;

uint32_t start_time;

volatile uint8_t onP = 0;
volatile uint8_t buttonHit = 0;

uint8_t colors[COL_ARR_SIZE][3];
const uint8_t rgb_array[3][3] = {
  {HiThr,0,0},
  {0,HiThr,0},
  {0,0,HiThr},
};


const uint32_t RED = 0xff0000;
const uint32_t GREEN = 0x00ff00;
const uint32_t BLUE = 0x0000ff;
const uint32_t MAGENTA = 0xff00ff;
const uint32_t YELLOW = 0xffff00;
const uint32_t CYAN = 0x00ffff;
const uint32_t PURPLE = 0x8800ff;
const uint32_t OFF = 0x000000;

unsigned int current_pixel = 0;
bool cleared = true;
uint16_t spacing = 100; //set in setup(), so this may not matter

static uint32_t last_color = 0;


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
      colors[outer][inner] = random(0,4) * 64;
    }
  }
}

//set one of the bytes to the top threshold.
//If "definitely" is true, set the other bytes to 0
//void mkRGB(short color) {
//    if      (colors[color][0] == HiThr) { colors[color][1] = LoThr; colors[color][2] = LoThr;}
//    else if (colors[color][1] == HiThr) { colors[color][0] = LoThr; colors[color][2] = LoThr;}
//    else if (colors[color][2] == HiThr) { colors[color][0] = LoThr; colors[color][1] = LoThr;}
//    else {
//      colors[color][random(0,3)] = HiThr;
//      mkRGB(color);
//    }
//}

uint32_t pickRGB() {
    const uint32_t rgb[3] = { RED, GREEN, BLUE };
    if (random(0,100) == 0) return PURPLE;

    //uint8_t  colorUp = random(0,3);
    //return rgb[colorUp];

    return rgb[random(0,3)];
}

void mkRGB(short color) {
    uint8_t  colorUp = random(0,3);
    colors[color][0] = 0;
    colors[color][1] = 0;
    colors[color][2] = 0;
    colors[color][colorUp] = 0xc0;
}

/*********************************************************************************************
 * Color Mod Functions. These will appear in the loop().
 * Make sure they save state to either a common globals or a reserved global
 */

/**********************
 * Slowly make all colors are either red, green, or blue.
 * Changes one entry randomly on each loop pass.
 */
void hammer(bool ensure) {
  //ensure that a color gets changed all the way to r g or b
  short color = random(0, COL_ARR_SIZE); //0-63 IRL, high number is excluded
  if (ensure) {
    mkRGB(color);
  } else {
    colors[color][random(0,3)] = HiThr;
  }
}

// Make everything Exactly Red, Green or Blue. Should be run exactly once.
void bigBlow() {
  for (short color=0; color < COL_ARR_SIZE; color++) {
    mkRGB(color);
  }
  dbg_lcdwrite(11,1,"BB");
}


//This picks the actual last color to be displayed
void finalBlow(uint32_t color) {
    for (uint8_t  flashes = 5; flashes > 0; flashes--) {
      setPixels(&pack, 0);
      pack.show();
      delay(500);
      setPixels(&pack, color);
      pack.show();
      delay(250);
    }
    delay(3000);
    //onP = false; 
}




//Select next color

//uint32_t next_color() {
//  static short r=0,g=0,b=0;
//  uint8_t  offset = 64;
//  if (b > 255) {
//    b=0;
//    g+=offset;
//  } else b+=offset;
//  if (g > 255) {
//    g=0;
//    r+=offset;
//  }
//  if (r > 255) {
//    r=g=b=0;
//  }
//  return pack.Color(r,g,b);
//}

/**************
 * sets all pixels to the same color
 */
void setPixels (Adafruit_DotStar *stars, uint32_t color) {
  for (int i = 0; i < PACK_SIZE; i++) {
    stars->setPixelColor(i, color);
  }
}



/**********************************************************************
 * LED display routines
 */
void twirl1 (Adafruit_DotStar *stars, uint32_t color, uint16_t delay_time) {
  for (int i = 0; i < PACK_SIZE; i++) {
    stars->setPixelColor((i-1)%PACK_SIZE, 0);
    stars->setPixelColor(i, color);
    stars->show();
    delay(delay_time);
  }
  stars->setPixelColor(PACK_SIZE-1,0);
}

void dance (Adafruit_DotStar *stars, uint32_t color) {
  static int last_pixel = 0;
  //stars->clear();
  int next_pixel = random(0,PACK_SIZE);
  stars->setPixelColor(next_pixel, color);
  stars->show();
  stars->setPixelColor(last_pixel, 0);
  last_pixel = next_pixel;
  delay(64);
}

void twist8 (Adafruit_DotStar *stars) {
  static uint32_t dancers[7] = {RED, GREEN, BLUE, OFF, CYAN, MAGENTA, YELLOW};
  static uint8_t offset = 0;
  //The lumenati has the number 1 pixel in the center, and the rest go around in order.
  for (uint8_t i = 0; i<7; i++) {
    stars->setPixelColor((i+offset)%7+1, dancers[i]);
  }
  stars->setPixelColor(0, dancers[offset%3]);
  stars->show();
  delay(166);
  if (offset == 6) { offset = 0; }
  else { offset++; }
}


void circle8 (Adafruit_DotStar *stars) {
  static uint8_t offset = 0;
  //turning off leds individually might be silly with the lumenati.
  stars->clear();
 
  //turn off the center or not
  if (offset%6 == 0)
    { stars->setPixelColor(0, PURPLE); }
  else
    { stars->setPixelColor(0, OFF); }
  
  //turn on RGB, spaced
  stars->setPixelColor((offset)%7+1, RED);
  stars->setPixelColor((offset+2)%7+1, GREEN);
  stars->setPixelColor((offset+4)%7+1, BLUE);
  stars->show();
  
  delay(166);
  if (offset == 6) { offset = 0; }
  else { offset++; }
}

/********
 * Slowly becomes more orderly
 */
void playMaxWell (uint16_t delay_time) {
  //uint8_t  * col = colors[MWindex];
  const uint8_t * col = rgb_array[MWindex%3];
  //setPixels(&pack, pack.Color(col[0], col[1], col[2]));
  twirl1(&pack, pack.Color(col[0], col[1], col[2]), delay_time);
  MWindex++; //intentionally wrapping around
}


/*******************************************************************
 * Diagnostic helpers
 */

//For a busy spinner
auto spin_frame = ".oO0"; //had to escape the backslash

void flash_diag(short flashes) {
  for (short i = 0; i< flashes; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(60);
    digitalWrite(LED_BUILTIN, LOW);
    delay(60);
  }
  delay(120); //make some spacing
}

void morse_flash(uint8_t  digit) {
  uint8_t  where = 0;
  const short dit = 100;
  const short dah = 3*dit;
  const short numbers[10][6] = {
    {dit, dah, dah, dah, dah, -1}, //1
    {dit, dit, dah, dah, dah, -1},
    {dit, dit, dit, dah, dah, -1},
    {dit, dit, dit, dit, dah, -1},
    {dit, dit, dit, dit, dit, -1}, //5
    {dah, dit, dit, dit, dit, -1},
    {dah, dah, dit, dit, dit, -1},
    {dah, dah, dah, dit, dit, -1},
    {dah, dah, dah, dah, dit, -1}, //9
    {dah, dah, dah, dah, dah, -1}  //0
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

/********************************************
 * Button ISR and helpers
 */

void handleStopStartButton (void) {
  buttonHit = 1;
  //sleep_disable();
}

/**************************
 * Turn on the sequence, but debounce just once
 */

void turnOn() {
  onP = 1;
  deBounce();
  buttonHit = 0;
  start_time = millis();
  //Serial.print("Turn on");
}

void turnOff() {
  onP = 0;
  deBounce();
  buttonHit = 0;
  pack.clear();
  pack.show();
  setup();
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



/*****************************************************************************************
 * ***************************************************************************************
 * Run phase handling. This is where the actions and hooks are dispatched.
 */

//Phase thresholds
uint16_t phase_limits[4] = {10000,20000,30000,50000}; //milliseconds from start
uint8_t phase = 0;



/************************************************
 * phase_hook and phase_action are vectors of lambdas. The number of members
 *  each corresponds to the phases demarcated in the phase_limits.
*/

//To be run once at the start of a phase
void (*(phase_hook[]))() = {
  [] () { },
  [] () { },
  [] () { bigBlow(); },
  [] () { finalBlow(last_color); }
};

//To be run on every loop iteration
void (*(phase_action[]))() = {
  [] () { hammer(false); playMaxWell(spacing); spacing++; },
  [] () { hammer(true); playMaxWell(spacing); spacing++; },
  [] () { playMaxWell(spacing); spacing++; },
  [] () { dance(&pack, last_color); }
};


// Action_list_item says do action_list_tem.action() action_list_item.times
// by Convention, if times = 0, do forever.
struct action_list_item {
  uint16_t times;
  void (*action)();
} action_list[] {
  { 200, [] () { twist8(&pack); } },
  { 300, [] () { circle8(&pack); } },
  { 1, [] () { bigBlow(); }},
  { 200, [] () { playMaxWell(spacing/4); spacing++; }},
  { 1, [] () { finalBlow(last_color); }},
  { 0, [] () { dance(&pack, last_color); }}
};


void action_player () {
  for (auto item: action_list) {
    if (item.times != 0) {
      for (auto i = item.times; i > 0; i--) {
        item.action();
        if (buttonHit == 1) { turnOff(); return;}
      }
    } else {
      //.times == 0, do this forever
      while(true) {
        item.action();
        if (buttonHit == 1) { turnOff(); return;}
      }
    }
  }
}

void setPhase() {
  if (phase == ((sizeof(phase_limits)/(sizeof(*phase_limits))-1))) { return; }
  if (millis() - start_time > phase_limits[phase]) {
    phase++;
    //dbg_lcdwrite(14,0);)
    //DBG(lcd.print((sizeof(phase_limits)/(sizeof(*phase_limits))-1));)
    dbg_lcdwrite(6+phase,0,phase);
    dbg_lcdwrite(1,1,millis() - start_time);
    phase_hook[phase]();
  }
}

//int on = 20;
void button_led_throbber() {
  static int led_on = 20;
  int cycle = 128;
  int bottom = 5;
  static int adder = 1;
  // Serial.print("on: ");
  // Serial.print(led_on);
  // Serial.print(" adder: ");
  // Serial.print(adder);
  // Serial.print("\n");
  led_on += adder;
  if (led_on >= cycle || led_on <= bottom) {
    adder *= -1;
  }

  analogWrite(BUTTON_LED, led_on);
  delay(20);
}

/********************************************
 * SETUP
 */
void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
#if FB_DEBUG
  lcd.begin(16,2,LCD_5x10DOTS);
#endif
  dbg_lcdwrite(0,0,"GO");
  //Serial.begin(115200);

  //start_time = millis();
  pinMode(BUTTON, INPUT_PULLUP); //with pullup, When the signal goes LOW, the button is being pressed
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(BUTTON_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  //dbg_lcdwrite(4,0, onP?"T":"F");
  
  current_pixel = 0;
  MWindex = 0;
  spacing = 100;
  randomSeed(analogRead(UNCONNECTED_PIN));
  randomize_color_array();
  last_color = pickRGB();

  pack.begin();
  pack.setBrightness(250);
  pack.show();
  delay(.5);
  //This is down here to put a tiny delay between setting the pin mode and attaching the interrupt
  attachInterrupt(digitalPinToInterrupt(BUTTON), handleStopStartButton, LOW);
  onP = 0;
  //Serial.print("Going");
}



/*********************************************
 * Hard reset: enables the watchdog timer for a very short time,
 * and never resets it. This causes the WD to do a full reset of the system,
 * clearing registers, and whatnot. Recommended by Atmel staff.
 */ 
void system_reset (void) {
  wdt_enable(WDTO_30MS);
  while(1) {};
} 

void reset() {
    onP = 0;
    dbg_lcdwrite(1,1,start_time);

    phase = 0;
    dbg_lcdwrite(0,1,"R");
    pack.clear();
    pack.show();
    spacing = 5;
    randomize_color_array();
    last_color = pickRGB();
}

short led_on = 20;
short adder = 1;
/*******************************************
 * LOOP
 */
void loop() {

  if (buttonHit == 1) turnOn();

  if (onP != 0) {
    analogWrite(BUTTON_LED, 0);
    //dbg_lcdspin(15,1, spin_frame[loop_count%4]);

    cleared = false;
    action_player();
    //phase_action[phase]();
    //setPhase();
  // If turned off 
  } else {
    //clean up if canceled
    //button_led_throbber();
    if (!cleared) {
      //reset();
      cleared = true;
      flash_diag(2);
      //sleep_enable();
      //sleep_cpu();
    } 
  
    button_led_throbber();
  //short led_on = 20;
  // short cycle = 30;
  // short bottom = 5;
  // Serial.print("on: ");
  // Serial.print(led_on);
  // Serial.print(" adder: ");
  // Serial.print(adder);
  // Serial.print("\n");
  // led_on += adder;
  // if (led_on >= cycle || led_on <= bottom) {
  //   adder *= -1;
  // }

  // digitalWrite(BUTTON_LED, HIGH);
  // delay(led_on);
  // digitalWrite(BUTTON_LED, LOW);
  // delay(cycle - led_on);
  }

  loop_count++;
}
  
