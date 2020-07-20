
// Scale Pins
// HX711.DOUT  - pin #38
// HX711.PD_SCK - pin #19
HX711 scale(38, 19);

// RPM input variables
volatile uint32_t stepCount[] = {0,0,0,0,0};
volatile uint64_t stepTime[] = {0,0,0,0,0};
volatile uint32_t stepDiff[] = {0,0,0,0,0};
Average<int> avgStepDiff[] = { 0,
                              Average<int> (AVGSIZE),
                              Average<int> (AVGSIZE),
                              Average<int> (AVGSIZE),
                              Average<int> (AVGSIZE)};
int theseRpms[5];
void countRPMs(int inputID = 0);

// analog value variables
uint32_t ulADC0Value[8];
volatile int voltageValue = 0;
volatile int currentValue = 0;
volatile int thrust = 0;

//PWM Output Variables
void updatePWM(unsigned pulseWidth, unsigned pwmOutput = 0);


// Misc Variables
char character;
String input;
uint64_t startTime;
uint64_t loopStart;
boolean isTestRunning = false;
uint32_t currentMicros = 0;
boolean isTared = false;
uint32_t loopDelay = 4000;

union f_bytes
{
  byte b[4];
  float fval;
}u;

//for average step times
//need more testing to determine optimal number of steps to average
volatile uint32_t step_times[5][STEP_COUNT];
volatile uint32_t step_idx[] = {0,0,0,0,0};

uint16_t pwmMultiplier = 1000;
