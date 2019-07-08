#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#define TFT_CS         8
#define TFT_RST        6 
#define TFT_DC         7
#define TFT_BL         9
#define CHARGE_MOD_PIN 2
#define MOD_BUTTON     3
#define BEEP_PIN       5


#define TFT_WIDTH 128
#define TFT_HEIGHT 160
#define CHART_TOP 32
#define VALUE_CENTER 64
#define VOL_COLOR ST77XX_GREEN
#define ELEC_COLOR ST77XX_ORANGE
#define COLOR_NORMAL 0xB63B
#define COLOR_BUSY ST77XX_MAGENTA

#define CHARGE_MOD_DIRECT 1
#define CHARGE_MOD_INDIRECT 2


#define DISPLAY_OFF_LOOP_TIMES 10
#define BUSY_VOL 10.0

static const int START_BEEP[] = {50}; 
static const int COMPLETE_BEEP[] = {20, 100, 20};


typedef unsigned long uint32;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

int pointPosition = 0;
int chartVol = 64;
int chartElec = 64;

char targetStr[20];  
char tmpStr[20];  

uint16_t statusColor = COLOR_BUSY;

struct {
  float vol;
  float elec;
  float power;
  float amount;
  float co2;
  float temp;
  float freq;
} resultData;

//计数 实际使用时删掉
int mCount = 0;

int loopTimes = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");
  initPinMode();
  beep(START_BEEP, 2);
  initTFT();
  Serial.println("Startup!");
  beep(COMPLETE_BEEP, 3);
}

void loop() {  
  noInterrupts();
  if(loopTimes <= DISPLAY_OFF_LOOP_TIMES){
    loopTimes++;  
  }
  readMeasureData();
  refreshStatusColor();
  showResultDataChart();
  showResultDataText();
  setTFTBL(loopTimes < DISPLAY_OFF_LOOP_TIMES);
  interrupts();
  delay(1000);
}

void initTFT(){
  setTFTBL(false);
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  initDisplay();
  setTFTBL(true);
  tft.drawLine(0, VALUE_CENTER+CHART_TOP, TFT_WIDTH, VALUE_CENTER+CHART_TOP, ST77XX_CYAN);
}

void initPinMode(){
  pinMode(TFT_BL, OUTPUT);
  pinMode(CHARGE_MOD_PIN, OUTPUT);
  pinMode(MOD_BUTTON, INPUT);
  pinMode(BEEP_PIN, OUTPUT);
  digitalWrite(BEEP_PIN, HIGH);
  attachInterrupt(digitalPinToInterrupt(MOD_BUTTON), onModButtonClick, LOW);
}

void onModButtonClick(){
  loopTimes = 0;
  setTFTBL(true);
}

void initDisplay(){
   // 可绘制区域最高
   int yMax = TFT_HEIGHT - TFT_WIDTH - 2;
   //初始化曲线区域上部的横线
   tft.drawLine(0, yMax+1, TFT_WIDTH, yMax + 1, ST77XX_WHITE);
}

void readMeasureData(){
  chartVol = randomData(chartVol, TFT_WIDTH, 0);
  chartElec = randomData(chartElec, TFT_WIDTH, 0);
  resultData.elec = 1.2521;
  resultData.power = 24.2012;
  resultData.amount = 1054.5487;
  resultData.co2 = 20.4021;
  resultData.temp = 20.41;
  resultData.freq = 56.25;

  mCount++;
  if(mCount % 3 == 0){
    if(resultData.vol > BUSY_VOL){
      resultData.vol = 7.1;
    }else{
      resultData.vol = 13.6;  
    }
  }
}

void refreshStatusColor(){
  if(resultData.vol > BUSY_VOL){
    setChargeMod(CHARGE_MOD_DIRECT);
    statusColor = COLOR_NORMAL;  
  }else{
    setChargeMod(CHARGE_MOD_INDIRECT);
    statusColor = COLOR_BUSY;  
  }
  tft.fillRect(100, 0, 28, 30, statusColor);  
}

void showResultDataChart(){
  int vy = chartVol + CHART_TOP;
  int cy = chartElec + CHART_TOP;
  tft.drawLine(pointPosition, CHART_TOP, pointPosition, TFT_HEIGHT, ST77XX_BLACK);
  tft.drawLine(pointPosition + 1, CHART_TOP, pointPosition + 1, TFT_HEIGHT, statusColor);
  tft.drawPixel(pointPosition, VALUE_CENTER + CHART_TOP, ST77XX_CYAN);
  tft.drawPixel(pointPosition, vy, VOL_COLOR);
  tft.drawPixel(pointPosition, cy, ELEC_COLOR);
  pointPosition++;
  if(pointPosition>=TFT_WIDTH){
    pointPosition = 0;  
  }
}

void showResultDataText(){
  tft.fillRect(0, 0, 100, 31, ST77XX_BLACK);
  tft.fillRect(0, CHART_TOP + 1, 70, 20, ST77XX_BLACK);
  tft.fillRect(80, 140, 48, 20, ST77XX_BLACK);
  
  char* volStr = asText(resultData.vol, 7, 4, "V");
  drawText(0, CHART_TOP + 2, volStr, VOL_COLOR);
  char* elecStr = asText(resultData.elec, 7, 4, "A");
  drawText(0, CHART_TOP + 12, elecStr, ELEC_COLOR);
  char* powerStr = asText(resultData.power, 8, 4, "W");
  drawText(0, 0, powerStr, ST77XX_RED);
  char* amountStr = asText(resultData.amount, 9, 4, "Wh");
  drawText(0, 10, amountStr, ST77XX_YELLOW);
  char* co2Str = asText(resultData.co2, 7, 4, "Kg CO2");
  drawText(0, 20, co2Str, ST77XX_WHITE);
  char* tempStr = asText(resultData.temp, 5, 2, "C");
  drawText(80, 140, tempStr, ST77XX_WHITE);
  char* freqStr = asText(resultData.freq, 5, 2, "Hz");
  drawText(80, 150, freqStr, ST77XX_MAGENTA);
}

char* asText(double num, int width, int prec, char* suffix){
  char* str = dtostrf(num, width, prec, tmpStr);
  sprintf(targetStr, "%s %s", tmpStr, suffix);
  return targetStr;
}

void drawText(int x, int y, char *text, uint16_t color) {
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void setTFTBL(bool stat){
  if(stat){
    digitalWrite(TFT_BL, HIGH);
  }else{
    digitalWrite(TFT_BL, LOW);
  }
}

void setChargeMod(char mod){
  switch(mod){
    case CHARGE_MOD_DIRECT:
      digitalWrite(CHARGE_MOD_PIN, HIGH);
      break;
    case CHARGE_MOD_INDIRECT:
      digitalWrite(CHARGE_MOD_PIN, LOW);
      break;
  }
}

int randomData(int init, int maxNum, int minNum){
  int offset = random(-1, 2);
  init += offset;
  if(init >= maxNum){
    init  = maxNum; 
  }
  if(init <= minNum){
    init = minNum;
  }
  return init;
}

void beep(int timeQuene[], int len){
   bool beepState = true;
   for(int i = 0;i<len; i++){
     if(beepState){
       digitalWrite(BEEP_PIN, LOW);
     }else{
       digitalWrite(BEEP_PIN, HIGH);
     }
     beepState = !beepState;
     int delayTime = timeQuene[i];
     delay(delayTime);
     Serial.print("time:");
     Serial.println(delayTime);
   }
   digitalWrite(BEEP_PIN, HIGH);
}
