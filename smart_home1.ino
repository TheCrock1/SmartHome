// Умен контрол на температура и осветление за стая
// Автор: Arduino проект
// Описание: Автоматично управление на вентилатор и LED според температура и светлина

// Дефиниции на пиновете
const int TEMP_SENSOR_PIN = A0;    // Температурен сензор (TMP36 или LM35)
const int LDR_PIN = A1;            // Фоторезистор (LDR)
const int FAN_PIN = 9;             // Вентилатор (PWM пин за TIP120)
const int LED_PIN = 6;             // LED осветление (PWM пин)
const int BUTTON_PIN = 2;          // Бутон за ръчно управление
const int BUTTON_FAN_PIN = 3;      // Бутон за ръчно управление на вентилатора
const int BUTTON_LED_PIN = 4;      // Бутон за ръчно управление на LED

// Константи за настройки
const float TEMP_THRESHOLD = 28.0;  // Праг за температура в °C
const int LIGHT_THRESHOLD = 300;    // Праг за светлина (0-1023)
const int FAN_SPEED = 200;          // Скорост на вентилатора (0-255)
const int LED_BRIGHTNESS = 150;     // Яркост на LED (0-255)

// Променливи за състояние
bool fanOn = false;
bool ledOn = false;
bool manualMode = false;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 50;
int buttonFanState = HIGH;
int lastButtonFanState = HIGH;

int buttonLedState = HIGH;
int lastButtonLedState = HIGH;

unsigned long lastFanButtonPress = 0;
unsigned long lastLedButtonPress = 0;

// Променливи за сензори
float temperature = 0.0;
int lightLevel = 0;
int buttonState = HIGH;
int lastButtonState = HIGH;

void setup() {
  // Инициализация на серийната комуникация
  Serial.begin(9600);
  Serial.println("=== Умен контрол на стая ===");
  Serial.println("Стартиране на системата...");
  
  // Настройка на пиновете
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_FAN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_LED_PIN, INPUT_PULLUP);
  
  // Първоначално изключване на всички устройства
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("Системата е готова!");
  Serial.println("Температурен праг: " + String(TEMP_THRESHOLD) + "°C");
  Serial.println("Светлинен праг: " + String(LIGHT_THRESHOLD));
  Serial.println("------------------------");
}

void loop() {
  // Четене на сензорите
  readSensors();
  
  // Проверка на бутона
  checkButton();
  checkManualControlButtons();
  
  // Автоматично управление (ако не е в ръчен режим)
  if (!manualMode) {
    automaticControl();
  }
  
  // Показване на информация
  displayStatus();
  
  // Кратка пауза
  delay(1000);
}

void readSensors() {
  // Четене на температурата
  int tempReading = analogRead(TEMP_SENSOR_PIN);
  
  // Конвертиране за TMP36 сензор
  // TMP36: 10mV/°C с offset 500mV за 0°C
  float voltage = (tempReading * 5.0) / 1024.0;
  temperature = (voltage - 0.5) * 100.0;
  
  // Четене на светлината
  lightLevel = analogRead(LDR_PIN);
}

void checkButton() {
  int reading = digitalRead(BUTTON_PIN);
  
  // Debounce логика
  if (reading != lastButtonState) {
    lastButtonPress = millis();
  }
  
  if ((millis() - lastButtonPress) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // Ако бутонът е натиснат (LOW заради pull-up)
      if (buttonState == LOW) {
        toggleManualMode();
      }
    }
  }
  
  lastButtonState = reading;
}

void checkManualControlButtons() {
  if (manualMode) {
    // --- ВЕНТИЛАТОР БУТОН ---
    int fanReading = digitalRead(BUTTON_FAN_PIN);
    if (fanReading != lastButtonFanState) {
      lastFanButtonPress = millis();
    }
    if ((millis() - lastFanButtonPress) > DEBOUNCE_DELAY) {
      if (fanReading != buttonFanState) {
        buttonFanState = fanReading;
        if (buttonFanState == LOW) {
          if (fanOn) {
            turnOffFan();
          } else {
            turnOnFan();
          }
        }
      }
    }
    lastButtonFanState = fanReading;

    // --- LED БУТОН ---
    int ledReading = digitalRead(BUTTON_LED_PIN);
    if (ledReading != lastButtonLedState) {
      lastLedButtonPress = millis();
    }
    if ((millis() - lastLedButtonPress) > DEBOUNCE_DELAY) {
      if (ledReading != buttonLedState) {
        buttonLedState = ledReading;
        if (buttonLedState == LOW) {
          if (ledOn) {
            turnOffLED();
          } else {
            turnOnLED();
          }
        }
      }
    }
    lastButtonLedState = ledReading;
  }
}

void toggleManualMode() {
  manualMode = !manualMode;
  
  if (manualMode) {
    Serial.println(">>> РЪЧЕН РЕЖИМ АКТИВИРАН <<<");
    // Изключване на всички устройства в ръчен режим
    turnOffFan();
    turnOffLED();
  } else {
    Serial.println(">>> АВТОМАТИЧЕН РЕЖИМ АКТИВИРАН <<<");
  } 
}

void automaticControl() {
  // Управление на вентилатора според температурата
  if (temperature > TEMP_THRESHOLD && !fanOn) {
    turnOnFan();
  } else if (temperature <= (TEMP_THRESHOLD - 2.0) && fanOn) {
    // Хистерезис от 2°C за избягване на често включване/изключване
    turnOffFan();
  }
  
  // Управление на LED според светлината
  if (lightLevel < LIGHT_THRESHOLD && !ledOn) {
    turnOnLED();
  } else if (lightLevel > (LIGHT_THRESHOLD + 50) && ledOn) {
    // Хистерезис за избягване на мигане
    turnOffLED();
  }
}

void turnOnFan() {
  fanOn = true;
  analogWrite(FAN_PIN, FAN_SPEED);
  Serial.println("🌀 Вентилаторът е ВКЛЮЧЕН");
}

void turnOffFan() {
  fanOn = false;
  analogWrite(FAN_PIN, 0);
  Serial.println("🌀 Вентилаторът е ИЗКЛЮЧЕН");
}

void turnOnLED() {
  ledOn = true;
  analogWrite(LED_PIN, LED_BRIGHTNESS);
  Serial.println("💡 LED осветлението е ВКЛЮЧЕНО");
}

void turnOffLED() {
  ledOn = false;
  analogWrite(LED_PIN, 0);
  Serial.println("💡 LED осветлението е ИЗКЛЮЧЕНО");
}

void displayStatus() {
  Serial.println("------------------------");
  Serial.println("Режим: " + String(manualMode ? "РЪЧЕН" : "АВТОМАТИЧЕН"));
  Serial.println("Температура: " + String(temperature, 1) + "°C");
  Serial.println("Светлина: " + String(lightLevel) + " (0-1023)");
  Serial.println("Вентилатор: " + String(fanOn ? "ВКЛ" : "ИЗКЛ"));
  Serial.println("LED: " + String(ledOn ? "ВКЛ" : "ИЗКЛ"));
  
  // Предупреждения
  if (temperature > TEMP_THRESHOLD + 5) {
    Serial.println("⚠️  ВНИМАНИЕ: Много висока температура!");
  }
  
  if (lightLevel < 100) {
    Serial.println("🌙 Много тъмно в помещението");
  }
  
  Serial.println("========================");
}

// Функция за калибриране на сензорите (може да се извика от Serial Monitor)
void calibrateSensors() {
  Serial.println("=== КАЛИБРИРАНЕ НА СЕНЗОРИ ===");
  
  int tempSum = 0;
  int lightSum = 0;
  int samples = 10;
  
  for (int i = 0; i < samples; i++) {
    tempSum += analogRead(TEMP_SENSOR_PIN);
    lightSum += analogRead(LDR_PIN);
    delay(100);
  }
  
  int avgTemp = tempSum / samples;
  int avgLight = lightSum / samples;
  
  Serial.println("Средна стойност температура: " + String(avgTemp));
  Serial.println("Средна стойност светлина: " + String(avgLight));
  Serial.println("===============================");
}