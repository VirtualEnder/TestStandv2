
// Scale Pins
// HX711.DOUT	- pin #9
// HX711.PD_SCK	- pin #8
HX711 scale(9, 8);

// RPM input variables
volatile uint32_t stepCount1 = 0;
volatile uint32_t stepCount2 = 0;
volatile uint64_t stepTime1 = 0;
volatile uint64_t stepTime2 = 0;
volatile uint32_t stepDiff1 = 0;
volatile uint32_t stepDiff2 = 0;
Average<int> avgStepDiff1(AVGSIZE);
int theseRpms;

// analog value variables
uint32_t ulADC0Value[8];
volatile int voltageValue = 0;
volatile int currentValue = 0;
volatile int thrust = 0;

// Misc Variables
char character;
String input;
uint32_t startTime;
uint32_t loopStart;
boolean isTestRunning = false;
uint32_t currentMicros = 0;
boolean isTared = false;

union f_bytes
{
  byte b[4];
  float fval;
}u;

//for average step times
//need more testing to determine optimal number of steps to average
volatile uint32_t step_times[STEP_COUNT];
volatile uint32_t step_idx;
