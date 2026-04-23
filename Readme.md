# 🌱 ESP32 Soil Moisture Monitoring System

A production-style embedded IoT system built on the **ESP32-WROOM-32** using ESP-IDF, featuring real-time sensing, persistent configuration, and a lightweight HTTP interface.


---

## 🚀 Features

* 📡 **Wi-Fi connectivity (STA mode)** with event-driven handling
* 🌐 **Embedded HTTP server**

  * `/data` endpoint returns live soil moisture in JSON
* 📟 **SSD1306 OLED display (I2C)**

  * Custom framebuffer + bitmap font rendering
* 🌡️ **ADC-based soil moisture sensing**

  * Averaged sampling for stability
* 💾 **Non-Volatile Storage (NVS)**

  * Stores Wi-Fi credentials persistently
* ⚙️ **FreeRTOS-based execution**

  * Deterministic periodic sampling loop
* 🧠 **Clean modular architecture**

  * Separation of drivers, networking, storage, and application logic

---

## 🏗️ System Architecture

```
                +----------------------+
                |     Soil Sensor      |
                |   (Analog Output)    |
                +----------+-----------+
                           |
                           v
                +----------------------+
                |   ADC (Oneshot)      |
                |  Averaging Filter    |
                +----------+-----------+
                           |
                           v
                +----------------------+
                |   Application Logic  |
                | (main.c loop)        |
                +----+-----------+-----+
                     |           |
                     v           v
        +----------------+   +------------------+
        | OLED Display   |   | HTTP Server      |
        | (SSD1306 I2C)  |   | /data endpoint   |
        +----------------+   +------------------+
                     |
                     v
             +------------------+
             | Wi-Fi Stack      |
             | (Event Driven)   |
             +------------------+
                     |
                     v
             +------------------+
             | NVS Storage      |
             | (SSID/PASSWORD)  |
             +------------------+
```

---

## 📡 API

### `GET /data`

Returns current soil moisture:

```json
{
  "status": "ok",
  "sensor_value": 0
}
```

---

## ⚙️ Hardware

* ESP32-WROOM-32
* Capacitive Soil Moisture Sensor
* SSD1306 OLED Display (128x64, I2C)
* Breadboard + jumper wires
* Pull up resistors



---

## 🔌 Pin Configuration

| Component   | ESP32 Pin         |
| ----------- | ----------------- |
| Soil Sensor | GPIO34 (ADC1_CH6) |
| OLED SDA    | GPIO21            |
| OLED SCL    | GPIO22            |

---

## 🧠 Key Technical Decisions

### 1. ADC Sampling Strategy

* Uses **oneshot mode** instead of continuous
* 32 samples averaged to reduce noise
* Controlled delay ensures stable readings

### 2. Custom Display Driver

* Full **framebuffer implementation**
* Manual pixel control
* Scaled bitmap font rendering (no external libs)

### 3. Event-Driven Wi-Fi

* Uses **ESP-IDF event loop**
* Handles:

  * Connection
  * Disconnection
  * Auth failures
* Tracks state via **FreeRTOS Event Groups**

### 4. Persistent Storage (NVS)

* SSID & password stored in flash
* Abstracted via `storage.c`
* Safe open/read/write pattern with fallback init

### 5. Lightweight HTTP Server

* Based on `esp_http_server`
* Minimal memory footprint
* Shared state via controlled setter (`set_moisture()`)

---

## 🧵 Runtime Flow

```
Boot
 ├── Initialize NVS
 ├── Store Wi-Fi credentials
 ├── Connect to Wi-Fi
 ├── Initialize ADC
 ├── Initialize OLED
 ├── Start HTTP server
 └── Loop:
      ├── Sample ADC (32 samples)
      ├── Compute moisture %
      ├── Update display
      ├── Update HTTP state
      ├── Maintain Wi-Fi connection
      └── Delay (~3.3s)
```

---

## 💾 Memory Architecture (NVS Focus)

### NVS Namespace: `"storage"`

| Key        | Type   | Description    |
| ---------- | ------ | -------------- |
| `ssid`     | string | Wi-Fi SSID     |
| `password` | string | Wi-Fi password |

### Flow

```
storage_write_string()  → nvs_set_str() → nvs_commit()
storage_read_string()   → nvs_get_str()
```

* Lazy initialization handled internally
* Default fallback values supported
* Clean handle lifecycle (open → use → close)


---

## 🔍 

* ✅ **Direct ESP-IDF usage**
* ✅ **Understanding of embedded memory (NVS, buffers)**
* ✅ **RTOS-aware design patterns**
* ✅ **Driver-level hardware control**
* ✅ **Networking stack integration**

---

## ⚠️ Limitations / Future Improvements

* No HTTPS (currently HTTP only)
* No OTA updates
* No sensor calibration curve (linear approximation used)
* Web UI could be added (currently API only)
* ADC calibration using eFuse could improve accuracy

---

## 🧪 Build & Flash

Using PlatformIO:

```bash
pio run
pio run --target upload
pio device monitor
```

---

## 📸 Demo
<img width="3024" height="4032" alt="IMG_0426" src="https://github.com/user-attachments/assets/da3f9edd-dadb-4ce7-a8cc-366e726cb32b" />
<img width="3024" height="4032" alt="IMG_0427" src="https://github.com/user-attachments/assets/49cb6cb2-8a76-4c4c-91fc-fd015c6ab97a" />
<img width="3024" height="4032" alt="IMG_0428" src="https://github.com/user-attachments/assets/afacc42b-5302-43f9-afa0-cc17b215864c" />
<img width="3024" height="4032" alt="IMG_0429" src="https://github.com/user-attachments/assets/4cbff172-ff37-4981-9f9b-12ddd55b71e0" />

<img width="525" height="530" alt="Screenshot from 2026-04-23 16-19-54" src="https://github.com/user-attachments/assets/7a87e697-d452-49e2-b494-a3e4e1e671d3" />
<img width="525" height="530" alt="Screenshot from 2026-04-23 16-20-13" src="https://github.com/user-attachments/assets/6e386fac-2ab1-4a86-8528-7da24ab6bb9a" />

---

