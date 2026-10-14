# Air Mouse ESP32 + MPU6050

Mouse de aire controlado por movimiento, con conexión Bluetooth BLE HID.
Apuntás con el mando y el cursor se mueve en la pantalla.
Doble botón (izquierdo/derecho) + deep sleep automático.

![Diagrama de conexiones](diagrama.png)

---

## Qué es este proyecto

Un mouse inalámbrico DIY que se controla moviendo la mano, como si fuera un control remoto de TV o un puntero láser.

Apuntás el mando hacia donde querés que vaya el cursor, y este se mueve en esa dirección. Los dos botones funcionan como click izquierdo y click derecho. Se conecta por Bluetooth BLE sin necesidad de drivers ni dongles USB.

---

## Características

- Mouse BLE HID (sin drivers, funciona en Windows / Mac / Linux / Android)
- Modo puntero: Yaw → X, Pitch → Y
- Click izquierdo y derecho
- Recalibración del centro (mantener ambos botones)
- Deep sleep a los 2 minutos de inactividad
- Wake-up por movimiento (pin INT del MPU6050)
- Batería recargable vía TP4056 con USB

---

## Hardware

- ESP32 DevKit 38 pines: 1
- MPU6050 (GY-521): 1
- Pushbutton: 2
- Switch deslizante SPDT: 1
- Batería LiPo 3.7V (400–800 mAh): 1
- Módulo TP4056 (con protección): 1
- Placa perforada: 1
- Cables finos: varios

---

## Conexiones

### MPU6050 → ESP32

| MPU6050 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| INT | GPIO 27 |
| AD0 | GND |

### Botones → ESP32

| Componente | ESP32 |
|---|---|
| Botón Izquierdo | GPIO 32 → GND |
| Botón Derecho | GPIO 33 → GND |

### Alimentación

| Desde | Hacia |
|---|---|
| Batería + (rojo) | TP4056 B+ |
| Batería − (negro) | TP4056 B− |
| TP4056 OUT+ | Switch (pata central) |
| Switch (pata lateral) | ESP32 5V/VIN |
| TP4056 OUT− | ESP32 GND |

IMPORTANTE: el cable INT del MPU6050 debe estar conectado al GPIO 27. Sin él, el wake-up por movimiento no funciona.

---

## Instalación paso a paso

### Paso 1: Preparar Arduino IDE

1. Descargá e instalá Arduino IDE desde arduino.cc/en/software
2. Abrí Arduino IDE
3. Ve a Archivo → Preferencias
4. En "Gestor de URLs Adicionales de Tarjetas", pegá:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
5. Click en Aceptar

### Paso 2: Instalar soporte para ESP32

1. Ve a Herramientas → Placa → Gestor de Tarjetas
2. Buscá "esp32"
3. Instalá "esp32 by Espressif Systems" (última versión)

### Paso 3: Instalar librerías

Ve a Programa → Incluir Librería → Gestionar Bibliotecas y buscá una por una:

- ESP32 BLE Combo (koko-dev)
- Adafruit MPU6050 (Adafruit)
- Adafruit Unified Sensor (Adafruit)

Click en INSTALL en cada una.

### Paso 4: Abrir el código

1. Descargá el archivo air_mouse.ino de este repositorio
2. Abrilo con Arduino IDE

### Paso 5: Configurar la placa

1. Ve a Herramientas → Placa → ESP32 Arduino
2. Seleccioná "ESP32 Dev Module"
3. Conectá el ESP32 por USB
4. Ve a Herramientas → Puerto y seleccioná el COMx correspondiente

### Paso 6: Subir el firmware

1. Click en el botón Subir (flecha derecha)
2. Esperá a que compile y suba
3. Deberías ver "Done uploading" abajo

### Paso 7: Verificar con el Monitor Serie

1. Ve a Herramientas → Monitor Serie
2. Configurá la velocidad a 115200 baudios
3. Deberías ver:

   === Air Mouse ESP32 + MPU6050 (BLE) ===
   [OK] MPU6050 detectado
   [CAL] Calibrando giroscopio... NO MOVER
   [CAL] Bias rad/s: 0.0012, -0.0003, 0.0008
   [BLE] Iniciando AirMouse...
   [BLE] Listo. Emparejá desde tu PC/Android.

### Paso 8: Emparejar por Bluetooth

1. En tu PC o Android, andá a Configuración → Bluetooth
2. Buscar dispositivos nuevos
3. Aparecerá un dispositivo BLE nuevo
4. Conectalo

### Paso 9: Listo para usar

- Mové el mando: el cursor se mueve
- Botón izquierdo: click izquierdo
- Botón derecho: click derecho
- Ambos botones: recalibrar centro

---

## Cómo se usa

| Acción | Resultado |
|---|---|
| Mover el mando | Mover cursor |
| Botón izquierdo | Click izquierdo (mantener = arrastrar) |
| Botón derecho | Click derecho |
| Ambos botones juntos | Recalibrar centro de pantalla |
| 2 min sin uso | Entra en deep sleep |
| Mover el mando dormido | Despertar y reconectar |

---

## Ajustes y personalización

### Sensibilidad del cursor

En el código, modificá estas líneas:

#define MAX_YAW_DEG    60.0f    // Menor = más sensible
#define MAX_PITCH_DEG  40.0f    // Menor = más sensible

### Velocidad del cursor

Dentro del loop(), cambiá el 30.0f:

int moveX = (int)((dYaw / MAX_YAW_DEG) * 30.0f);   // más alto = más rápido
int moveY = (int)(-(dPitch / MAX_PITCH_DEG) * 30.0f);

### Tiempo de deep sleep

#define SLEEP_TIMEOUT_MS   120000UL   // 120000 ms = 2 minutos

### Zona muerta

#define DEADZONE_DEG   1.0f    // Ángulo mínimo para mover el cursor

---

## Limitaciones conocidas

- Deriva del Yaw: el MPU6050 no tiene magnetómetro, así que el eje horizontal deriva con el tiempo. Recalibrá con ambos botones cuando el cursor se desvíe solo.
- Wake-up tras deep sleep: el ESP32 se reinicia (no continúa donde estaba). Es normal en deep sleep.
- Alcance BLE: aproximadamente 10 metros en interiores.
- Autonomía: aproximadamente 6 horas de uso continuo o varios días en deep sleep (con batería de 400 mAh).

---

## Solución de problemas

| Problema | Solución |
|---|---|
| MPU6050 no detectado | Verificá que SDA → GPIO 21 y SCL → GPIO 22 estén bien conectados |
| No aparece el dispositivo en Bluetooth | Reiniciá el ESP32, esperá 5 segundos y volvé a buscar |
| El cursor se mueve al revés | Cambiá el signo de moveX o moveY en el código |
| El cursor deriva solo | Presioná ambos botones para recalibrar |
| No despierta del deep sleep | Verificá el cable INT → GPIO 27 |

---

## Estructura del proyecto

air-mouse-esp32/
├── air_mouse.ino     Código fuente del firmware
├── diagrama.png      Diagrama de conexiones
└── README.md         Este archivo

---

## Licencia

Este proyecto está bajo licencia MIT.
Podés usarlo, modificarlo y distribuirlo libremente.

---

## Créditos

- Librería BLE: kokoded/ESP32-BLE-Combo
- Librería MPU6050: Adafruit MPU6050
- Inspirado en proyectos open-source de Air Mouse con ESP32
