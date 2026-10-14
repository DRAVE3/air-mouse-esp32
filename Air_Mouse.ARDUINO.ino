/*
 * Air Mouse ESP32 + MPU6050 — Hardware Real
 * 
 * Librerías:
 *  - ESP32 BLECombo (koko-dev) → Mouse BLE HID
 *  - Adafruit MPU6050 → Lectura del sensor
 *  - Adafruit Unified Sensor → Dependencia
 *  - Wire → I2C (incluida en el core)
 * 
 * Funciones:
 *  - Cursor por orientación (Yaw → X, Pitch → Y)
 *  - Click izquierdo/derecho
 *  - Recalibrar centro: mantener ambos botones
 *  - Deep sleep a los 2 min sin uso
 *  - Wake-up por movimiento (pin INT del MPU6050)
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP32BLECombo.h>

// ============ CONFIGURACIÓN ============
#define PIN_BTN_LEFT   32
#define PIN_BTN_RIGHT  33
#define PIN_MPU_INT    27

#define MAX_YAW_DEG    60.0f
#define MAX_PITCH_DEG  40.0f
#define DEADZONE_DEG   1.0f

#define SEND_INTERVAL_MS   20       // 50 Hz
#define SLEEP_TIMEOUT_MS   120000UL // 2 minutos

// ============ SENSOR Y BLE ============
Adafruit_MPU6050 mpu;
ESP32BLECombo bleCombo;

// ============ ÁNGULOS ============
float pitchAngle = 0.0f;
float rollAngle  = 0.0f;
float yawAngle   = 0.0f;

// ============ REFERENCIA ============
float refPitch = 0.0f;
float refYaw   = 0.0f;

// ============ BOTONES ============
bool lastBtnL = HIGH;
bool lastBtnR = HIGH;
unsigned long tBtnL = 0, tBtnR = 0;
const unsigned long DEBOUNCE = 30;

// ============ TIMING ============
unsigned long lastSend = 0;
unsigned long lastActivity = 0;
unsigned long lastMicros = 0;

// ============ BIAS DEL GIROSCOPIO ============
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;

// ============================================================
// CONFIGURAR MPU6050 PARA WAKE-ON-MOTION
// ============================================================
void enableMotionInterrupt() {
  // 1. Despertar el MPU6050
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);      // PWR_MGMT_1
  Wire.write(0x00);
  Wire.endTransmission();
  delay(10);

  // 2. Configurar acelerómetro
  Wire.beginTransmission(0x68);
  Wire.write(0x1C);      // ACCEL_CONFIG
  Wire.write(0x00);
  Wire.endTransmission();

  // 3. ACCEL_CONFIG2 para WOM (low power)
  Wire.beginTransmission(0x68);
  Wire.write(0x1D);
  Wire.write(0x07);
  Wire.endTransmission();

  // 4. INT_PIN_CFG: activo alto, push-pull
  Wire.beginTransmission(0x68);
  Wire.write(0x37);
  Wire.write(0x20);
  Wire.endTransmission();

  // 5. Habilitar WOM interrupt
  Wire.beginTransmission(0x68);
  Wire.write(0x38);
  Wire.write(0x40);      // bit 6 = WOM
  Wire.endTransmission();

  // 6. Umbral de movimiento
  Wire.beginTransmission(0x68);
  Wire.write(0x1F);
  Wire.write(100);       // ~400mg
  Wire.endTransmission();

  // 7. Duración mínima del movimiento
  Wire.beginTransmission(0x68);
  Wire.write(0x20);
  Wire.write(40);        // 40ms
  Wire.endTransmission();

  // 8. LP_WAKE_CTRL + deshabilitar gyro X/Y
  Wire.beginTransmission(0x68);
  Wire.write(0x6C);
  Wire.write(0xC7);
  Wire.endTransmission();
}

// ============================================================
// CALIBRACIÓN DEL GIROSCOPIO
// ============================================================
void calibrateGyro() {
  Serial.println("[CAL] Calibrando giroscopio... NO MOVER");
  const int N = 300;
  float sx = 0, sy = 0, sz = 0;

  sensors_event_t a, g, temp;
  for (int i = 0; i < N; i++) {
    mpu.getEvent(&a, &g, &temp);
    sx += g.gyro.x;
    sy += g.gyro.y;
    sz += g.gyro.z;
    delay(3);
  }
  gyroBiasX = sx / N;
  gyroBiasY = sy / N;
  gyroBiasZ = sz / N;

  Serial.print("[CAL] Bias rad/s: ");
  Serial.print(gyroBiasX, 4); Serial.print(", ");
  Serial.print(gyroBiasY, 4); Serial.print(", ");
  Serial.println(gyroBiasZ, 4);
}

// ============================================================
// FILTRO COMPLEMENTARIO
// ============================================================
void updateAngles(float dt) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Ángulos desde acelerómetro (rad → deg)
  float accelPitch = atan2(-a.acceleration.x,
                     sqrt(a.acceleration.y * a.acceleration.y +
                          a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
  float accelRoll  = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;

  // Restar bias del gyro (rad/s → deg/s)
  float gx = (g.gyro.x - gyroBiasX) * 180.0 / PI;
  float gy = (g.gyro.y - gyroBiasY) * 180.0 / PI;
  float gz = (g.gyro.z - gyroBiasZ) * 180.0 / PI;

  // Filtro complementario
  const float alpha = 0.96f;
  pitchAngle = alpha * (pitchAngle + gy * dt) + (1 - alpha) * accelPitch;
  rollAngle  = alpha * (rollAngle  - gx * dt) + (1 - alpha) * accelRoll;
  yawAngle  += gz * dt;
}

// ============================================================
// BOTONES CON BLE MOUSE
// ============================================================
void handleButtons() {
  bool rl = digitalRead(PIN_BTN_LEFT);
  bool rr = digitalRead(PIN_BTN_RIGHT);
  unsigned long now = millis();

  if (rl != lastBtnL && (now - tBtnL) > DEBOUNCE) {
    tBtnL = now;
    if (rl == LOW) {
      bleCombo.mousePress(ESP32BLECombo::MOUSE_LEFT);
      lastActivity = now;
    } else {
      bleCombo.mouseRelease(ESP32BLECombo::MOUSE_LEFT);
    }
    lastBtnL = rl;
  }

  if (rr != lastBtnR && (now - tBtnR) > DEBOUNCE) {
    tBtnR = now;
    if (rr == LOW) {
      bleCombo.mousePress(ESP32BLECombo::MOUSE_RIGHT);
      lastActivity = now;
    } else {
      bleCombo.mouseRelease(ESP32BLECombo::MOUSE_RIGHT);
    }
    lastBtnR = rr;
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Air Mouse ESP32 + MPU6050 (BLE) ===");

  pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_MPU_INT, INPUT_PULLUP);

  Wire.begin(21, 22);
  Wire.setClock(400000);

  if (!mpu.begin()) {
    Serial.println("[ERROR] MPU6050 no detectado. Revisar I2C.");
    while (1) delay(1000);
  }
  Serial.println("[OK] MPU6050 detectado");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  delay(100);
  calibrateGyro();

  refPitch = pitchAngle;
  refYaw   = yawAngle;

  // ----- Configurar BLE -----
  Serial.println("[BLE] Iniciando AirMouse...");
  bleCombo.begin();
  Serial.println("[BLE] Listo. Emparejá 'AirMouse' desde tu PC/Android.");

  lastActivity = millis();
  lastMicros = micros();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  unsigned long nowMicros = micros();
  float dt = (nowMicros - lastMicros) / 1000000.0f;
  lastMicros = nowMicros;
  if (dt > 0.1f) dt = 0.1f;

  // Solo procesar si BLE está conectado
  if (bleCombo.isConnected()) {
    updateAngles(dt);
    handleButtons();

    unsigned long now = millis();

    if (now - lastSend >= SEND_INTERVAL_MS) {
      lastSend = now;

      float dYaw   = yawAngle - refYaw;
      float dPitch = pitchAngle - refPitch;

      if (fabs(dYaw)   < DEADZONE_DEG) dYaw = 0;
      if (fabs(dPitch) < DEADZONE_DEG) dPitch = 0;

      // Movimiento relativo al cursor
      int moveX = (int)((dYaw / MAX_YAW_DEG) * 30.0f);
      int moveY = (int)(-(dPitch / MAX_PITCH_DEG) * 30.0f);

      if (moveX != 0 || moveY != 0) {
        bleCombo.mouseMove(moveX, moveY);
      }

      if (fabs(dYaw) > 0.5f || fabs(dPitch) > 0.5f) lastActivity = now;
    }

    // Recalibrar con ambos botones
    if (digitalRead(PIN_BTN_LEFT) == LOW &&
        digitalRead(PIN_BTN_RIGHT) == LOW) {
      refPitch = pitchAngle;
      refYaw   = yawAngle;
      lastActivity = now;
    }
  }

  // Deep sleep por inactividad
  if (millis() - lastActivity > SLEEP_TIMEOUT_MS) {
    Serial.println("[SLEEP] Entrando en deep sleep...");

    enableMotionInterrupt();
    delay(50);

    // Wake-up por INT del MPU6050 (activo alto → nivel HIGH)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_MPU_INT, 1);

    Serial.println("[SLEEP] Mueve el mando para despertar...");
    Serial.flush();
    delay(100);

    esp_deep_sleep_start();
  }
}