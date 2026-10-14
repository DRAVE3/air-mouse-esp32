# 🎮 Air Mouse ESP32 + MPU6050

Mouse inalámbrico controlado por movimiento, con conexión Bluetooth BLE HID.
Apuntás con el mando y el cursor se mueve en la pantalla.

![Diagrama de conexiones](diagrama.png)

---

## ✨ Características

- 🖱️ Mouse BLE HID (sin drivers, funciona en Windows/Mac/Linux/Android)
- 🎯 Modo puntero: Yaw → X, Pitch → Y
- 🔘 Click izquierdo y derecho
- 🎯 Recalibración del centro (mantener ambos botones)
- 💤 Deep sleep a los 2 minutos de inactividad
- ⚡ Wake-up por movimiento (pin INT del MPU6050)

---

## 🧰 Hardware

| Componente | Cantidad |
|---|---|
| ESP32 DevKit 38 pines | 1 |
| MPU6050 (GY-521) | 1 |
| Pushbutton | 2 |
| Switch deslizante SPDT | 1 |
| Batería LiPo 400-800 mAh 3.7V | 1 |
| TP4056 (con protección) | 1 |
| Placa perforada | 1 |

---

## 🔌 Conexiones

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
| Batería + | TP4056 B+ |
| Batería − | TP4056 B− |
| TP4056 OUT+ | Switch (pata central) |
| Switch (pata lateral) | ESP32 5V/VIN |
| TP4056 OUT− | ESP32 GND |

---

## 🚀 Instalación

### 1. Instalar Arduino IDE + soporte ESP32

Seguir la [guía oficial de Espressif](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

### 2. Instalar librerías

En **Programa → Incluir Librería → Gestionar Bibliotecas**:

- `ESP32 BLE Combo` (koko-dev)
- `Adafruit MPU6050` (Adafruit)
- `Adafruit Unified Sensor` (Adafruit)

### 3. Subir el firmware

1. Abrir `air_mouse/air_mouse.ino` con Arduino IDE
2. Seleccionar **Placa:** ESP32 Dev Module
3. Seleccionar **Puerto:** COMx
4. Click en **Subir** (botón →)

### 4. Emparejar

1. En tu PC/Android: **Bluetooth → Buscar dispositivos**
2. Buscar el dispositivo BLE y conectar
3. Mover el mando → el cursor se mueve

---

## 🎮 Controles

| Acción | Función |
|---|---|
| Mover el mando | Mover cursor |
| Botón izquierdo | Click izquierdo |
| Botón derecho | Click derecho |
| Ambos botones a la vez | Recalibrar centro |
| Inactividad 2 min | Deep sleep |
| Mover el mando (dormido) | Despertar |

---

## ⚙️ Ajustes de sensibilidad

Si el cursor se mueve muy rápido o muy lento, modificá estas líneas en el código:

```cpp
#define MAX_YAW_DEG    60.0f    // Menor = más sensible
#define MAX_PITCH_DEG  40.0f    // Menor = más sensible