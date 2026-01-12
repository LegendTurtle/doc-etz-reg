// Библиотеки
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Servo.h>

// Пины
#define STEPX1_PIN 6
#define DIRX1_PIN 7
#define STEPX2_PIN 8
#define DIRX2_PIN 9
#define SDA_PIN 14
#define SCL_PIN 15
#define EN_PIN 16
#define SERVO_PIN 12
#define WORK_STATUS_LED_PIN 17
#define S1_PIN 20
#define S2_PIN 21
#define KEY_PIN 22
#define LINE_SENSOR_PIN 26

// Константы
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const short modesNum = 6;

const short mode1Buttons = 3;
const short mode2Buttons = 2;
const short mode3Buttons = 2;

AccelStepper stepperX1(1, STEPX1_PIN, DIRX1_PIN);  // Инициализация шаговых двигателей
AccelStepper stepperX2(1, STEPX2_PIN, DIRX2_PIN);

MultiStepper steppers;  // Инициализация группы шаговых двигателей

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);  // Инициализация дисплея

Servo pen;  // Инициализация сервопривода

// Переменные
bool isRun = false;
bool buttonState = false;
bool lastButtonState = false;
bool isMenu = true;
bool isPressed = false;

short curMode = 0;
short curButton = 0;

int targetsNum = 0;

long encData = 0;
long lastPressTime = 0;
long lastSpinTime = 0;

long targetsLine[4][2];
bool servoLineTargets[4];

long (*targets)[2];
bool *servoTargets;

String modeNames[modesNum] = { "mode 1", "mode 2", "mode 3", "mode 4", "mode 5", "mode 6" };  // Массив названий режимов

void encoder() {
  if (millis() - lastSpinTime > 100) {
    encData += 1 - (digitalRead(S2_PIN) * 2);
    lastSpinTime = millis();
  }
}

void nothing() {}

void startLine() {
}

void penUp() {
  pen.write(115);
}
void penDown() {
  pen.write(80);
}

void leaveMode() {
  isMenu = true;
  encData = 0;
}

void (*servoActions[])() = { penUp, penDown };
void (*mode1Actions[])() = { leaveMode, penUp, penDown };      // Массив действий 1-го режима
void (*mode2Actions[])() = { leaveMode, nothing };             // Массив действий 2-го режима
void (*mode3Actions[])() = { leaveMode, nothing, startLine };  // Массив действий 3-го режима

void mode1() {  // Режим 1
  if (encData > 0) curButton = encData % mode1Buttons;
  else curButton = (encData % mode1Buttons + mode1Buttons) % mode1Buttons;

  display.drawTriangle(96, 28, 104, 12, 112, 28, SH110X_WHITE);
  display.drawTriangle(96, 36, 104, 52, 112, 36, SH110X_WHITE);

  display.setCursor(72, 16);
  display.print("up");

  display.setCursor(64, 40);
  display.print("down");


  if (curButton == 0) {
    display.fillTriangle(0, 8, 16, 16, 16, 0, SH110X_WHITE);
  } else if (curButton == 1) {
    display.fillTriangle(96, 28, 104, 12, 112, 28, SH110X_WHITE);
  } else if (curButton == 2) {
    display.fillTriangle(96, 36, 104, 52, 112, 36, SH110X_WHITE);
  }

  if (isPressed) {
    mode1Actions[curButton]();
  }
}

void mode2() {  // Режим 2
  if (encData > 0) curButton = encData % mode2Buttons;
  else curButton = (encData % mode2Buttons + mode2Buttons) % mode2Buttons;

  display.setTextSize(2);
  display.setCursor(48, 28);
  display.print(analogRead(LINE_SENSOR_PIN));

  if (isPressed) {
    mode2Actions[curButton]();
  }

  if (curButton == 0) {
    display.fillTriangle(0, 8, 16, 16, 16, 0, SH110X_WHITE);
  }
}

void mode3() {  // Режим 3
  if (encData > 0) curButton = encData % mode3Buttons;
  else curButton = (encData % mode3Buttons + mode3Buttons) % mode3Buttons;

  if (isPressed) {
    mode3Actions[curButton]();
  }

  if (curButton == 0) {
    display.fillTriangle(0, 8, 16, 16, 16, 0, SH110X_WHITE);
  } else if (curButton == 1) {
    
  }
}

void mode4() {  // Режим 4
}

void mode5() {  // Режим 5
}

void mode6() {  // Режим 6
}


void (*modes[modesNum])() = {
  // Массив режимов
  mode1,
  mode2,
  mode3,
  mode4,
  mode5,
  mode6,
};

void setup() {
  // Настройка пинов
  pinMode(EN_PIN, OUTPUT);
  pinMode(WORK_STATUS_LED_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);

  // Настройка прерывания для энкодера
  attachInterrupt(digitalPinToInterrupt(S1_PIN), encoder, FALLING);

  // Инициализация Serial порта
  Serial.begin(115200);

  // Настройка пина для сервопривода
  pen.attach(SERVO_PIN);

  // Настройка пинов и инициализация I2C
  Wire1.setSDA(SDA_PIN);
  Wire1.setSCL(SCL_PIN);
  Wire1.begin();

  // Инициализация дисплея
  if (!display.begin(0x3C, true)) {
    digitalWrite(WORK_STATUS_LED_PIN, HIGH);
    while (1) {}
  }

  // Очистка и настройка дисплея
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  // Максимальная скорость моторов
  stepperX1.setMaxSpeed(1000);
  stepperX2.setMaxSpeed(1000);

  // Добавление шаговых двигателей в группу
  steppers.addStepper(stepperX1);
  steppers.addStepper(stepperX2);

  // Выключение моторов
  digitalWrite(EN_PIN, HIGH);
}

void loop() {
  display.clearDisplay();  // Очистка дисплея

  // Обработка нажатия и дребезга кнопки жнкодера
  buttonState = !digitalRead(KEY_PIN);
  isPressed = false;
  if (buttonState and !lastButtonState and millis() - lastPressTime > 50) {  // попробовать вместо ифа просто выражение запихнуть в испресед
    isPressed = true;
    lastPressTime = millis();
  }

  if (isRun) {  // Выполнение действий
    display.setTextSize(2);
    display.setCursor(24, 28);
    display.print("Running...");
    for (int i = 0; i < targetsNum; i++) {
      servoActions[servoTargets[i]];
      steppers.moveTo(targets[i]);
      delay(100);
      while (steppers.run()) {}
    }
  } else {  // Меню
    if (isMenu) {
      buttonState = !digitalRead(KEY_PIN);

      if (isPressed) {
        isMenu = false;
        curButton = 0;
      }

      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(4, 4);

      if (encData > 0) curMode = encData % modesNum;
      else curMode = (encData % modesNum + modesNum) % modesNum;

      display.println(modeNames[curMode]);

      display.setTextSize(1);
      display.setCursor(96, 54);
      display.print(String(curMode + 1) + " / " + String(modesNum));
    } else {
      display.setTextSize(1);
      display.setCursor(24, 4);
      display.print(modeNames[curMode]);

      display.drawTriangle(0, 8, 16, 16, 16, 0, SH110X_WHITE);

      modes[curMode]();
    }
  }

  lastButtonState = buttonState;

  digitalWrite(WORK_STATUS_LED_PIN, isRun);  // Индикация выполнения действий
  digitalWrite(EN_PIN, isRun);               // Включение моторов
  display.display();                         // Обновление картинки
}
