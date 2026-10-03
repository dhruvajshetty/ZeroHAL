# Phase 2: The "Hello World" of Hardware
**Arduino, 8-bit Microcontrollers, and Your First Real Circuits**

This guide takes you from understanding electrons on paper to physically making them do useful work. By the end, you will understand how a microcontroller reads the physical world, processes data, and controls real devices.

---

## 1. What Is a Microcontroller?

A microcontroller (MCU) is a tiny, self-contained computer on a single chip. Unlike your PC, which has a separate CPU, RAM, hard drive, and motherboard, an MCU squeezes everything into one piece of silicon:

| Component | Your PC | Microcontroller (ATmega328P) |
|---|---|---|
| CPU | Intel i7 (4+ GHz, 64-bit) | AVR (16 MHz, 8-bit) |
| RAM | 16 GB DDR5 | 2 KB SRAM |
| Storage | 1 TB SSD | 32 KB Flash |
| I/O | USB, HDMI, Ethernet | 23 GPIO Pins |
| Power | 300W PSU | 0.1W (runs on a coin battery) |
| Cost | \$500+ | \$2 |

**Why use something so weak?** Because your toaster, car key fob, TV remote, and wristwatch don't need to run a web browser. They need to do one job reliably, cheaply, and on almost no power.

---

## 2. The Arduino Platform

Arduino is a beginner-friendly development board built around the **ATmega328P** microcontroller. It hides the complex setup (clock configuration, bootloader, programmer) behind a simple USB cable and a friendly IDE.

### What's on an Arduino Uno Board?

```
                    +--[USB Port]--+
                    |              |
              [LED] o    ATmega   |
                    |    328P     |
   [Power Jack] o---+   (DIP-28) |
                    |    ┌────┐   |
        [5V] o------+----│    │---+---o [Digital Pin 13]
        [3.3V] o----+----│    │---+---o [Digital Pin 12]
        [GND] o-----+----│    │---+---o ...
        [GND] o-----+----│    │---+---o [Digital Pin 0 (RX)]
        [Vin] o-----+----│    │---+---o [Digital Pin 1 (TX)]
                    |    └────┘   |
        [A0] o------+            |
        [A1] o------+            |
        ...         |            |
        [A5] o------+            |
                    +---------+--+
                              |
                        [Reset Button]
```

*   **Digital Pins (0-13):** Can be set to `HIGH` (5V) or `LOW` (0V). They speak in binary — on or off.
*   **Analog Pins (A0-A5):** Can *read* a range of voltages (0V to 5V) using a built-in **ADC (Analog-to-Digital Converter)**, giving you a number from `0` to `1023`.
*   **Power Pins:** Provide regulated `5V` and `3.3V` to power external components.
*   **GND (Ground):** The `0V` reference. Every circuit *must* share a common ground.

### Why Arduino First?
Arduino's magic is abstraction. The function `digitalWrite(13, HIGH);` hides roughly 15 lines of raw register manipulation (the exact kind you wrote in ZeroHAL). This lets you focus on *circuit design* and *logic* before worrying about silicon internals.

---

## 3. Digital I/O: Blinking an LED

This is the "Hello, World!" of embedded systems. You will control a physical LED using code.

### The Circuit

```
    Arduino Pin 13
         │
         │
        ┌┴┐
        │ │  330Ω Resistor (Orange-Orange-Brown)
        │ │  Purpose: Limits current to ~10mA so the LED doesn't burn
        └┬┘
         │
         ▼   LED (Light Emitting Diode)
        ─┬─  Long leg = Anode (+), Short leg = Cathode (-)
         │
         │
        GND (0V)
```

### Why the Resistor is Mandatory

Without a resistor, the LED acts like a near-zero resistance wire. Using Ohm's Law:

$$I = \frac{V}{R} = \frac{5V}{0\Omega} = \infty \text{ Amps (short circuit!)}$$

The MCU pin will try to dump as much current as possible. The LED burns instantly, and you might damage the microcontroller's output driver.

With a `330Ω` resistor:

$$I = \frac{5V - 2V_{LED\:drop}}{330\Omega} = \frac{3V}{330\Omega} \approx 9\text{mA (safe!)}$$

### The Code

```c
// Arduino "Blink" — The Hello World of Hardware
void setup() {
    // Configure Pin 13 as an OUTPUT (we are sending voltage out)
    pinMode(13, OUTPUT);
}

void loop() {
    digitalWrite(13, HIGH);  // Send 5V to Pin 13 → LED turns ON
    delay(1000);             // Wait 1000 milliseconds (1 second)
    digitalWrite(13, LOW);   // Send 0V to Pin 13 → LED turns OFF
    delay(1000);             // Wait 1 second
}
```

**What happens physically:**
1. `digitalWrite(13, HIGH)` → CPU writes a `1` into the `PORTB` register at bit 5 → silicon transistor inside the chip connects Pin 13 to the internal 5V rail → current flows through the resistor → through the LED → photons emit → **you see light**.
2. `digitalWrite(13, LOW)` → CPU writes a `0` → transistor disconnects Pin 13 → current stops → LED goes dark.

---

## 4. Digital Input: Reading a Button

Now the MCU reads the physical world instead of just controlling it.

### The Circuit (with Pull-Down Resistor)

```
        5V
         │
         │
       ┌─┴─┐
       │   │  Tactile Push Button
       │   │  (Normally Open — no connection until pressed)
       └─┬─┘
         │
         ├──────────── Arduino Pin 2 (Digital Input)
         │
        ┌┴┐
        │ │  10kΩ Pull-Down Resistor
        │ │  Purpose: Holds Pin 2 at 0V when button is NOT pressed
        └┬┘
         │
        GND
```

### Why the Pull-Down Resistor?

**Without it:** When the button is *not* pressed, Pin 2 is connected to... *nothing*. It's "floating". The wire acts like a tiny antenna, picking up electromagnetic noise from your phone, lights, and the 50/60Hz mains wiring in your walls. The MCU will randomly read `HIGH` and `LOW` — this is called a **floating pin** and it's one of the most common beginner bugs.

**With a 10kΩ pull-down:** When the button is open, Pin 2 is gently tied to GND through 10kΩ. It reliably reads `LOW (0V)`. When you press the button, Pin 2 gets directly connected to `5V` (much stronger than the 10kΩ pull to ground), so it reads `HIGH`.

### The Code

```c
void setup() {
    pinMode(13, OUTPUT);    // LED
    pinMode(2, INPUT);      // Button
}

void loop() {
    int buttonState = digitalRead(2);  // Read voltage on Pin 2

    if (buttonState == HIGH) {
        // Button is pressed: 5V is on Pin 2
        digitalWrite(13, HIGH);  // Turn LED ON
    } else {
        // Button is released: 0V on Pin 2 (via pull-down)
        digitalWrite(13, LOW);   // Turn LED OFF
    }
}
```

### Switch Bouncing (A Real-World Problem)

When you press a physical button, the metal contacts don't make a clean single connection. They physically *bounce* for 1-5 milliseconds, rapidly opening and closing dozens of times. The MCU is fast enough to detect every single bounce. To the CPU, one press looks like 20 presses.

**Software Debouncing Fix:**
```c
int lastState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;  // 50ms debounce window

void loop() {
    int reading = digitalRead(2);

    if (reading != lastState) {
        lastDebounceTime = millis(); // Reset timer on any change
    }

    // Only accept the new state if it has been stable for 50ms
    if ((millis() - lastDebounceTime) > debounceDelay) {
        // This is a real, confirmed press
        if (reading == HIGH) {
            digitalWrite(13, !digitalRead(13)); // Toggle LED
        }
    }
    lastState = reading;
}
```

---

## 5. Analog Input: Reading a Potentiometer (ADC)

Digital pins only see `HIGH` or `LOW`. But the real world isn't binary — temperature, light, and sound are *continuous* values. The **ADC (Analog-to-Digital Converter)** bridges this gap.

### How an ADC Works

The ATmega328P has a 10-bit ADC. It samples the voltage on an analog pin and converts it to a number:

| Input Voltage | ADC Reading | Percentage |
|---|---|---|
| 0.00V | 0 | 0% |
| 1.25V | 256 | 25% |
| 2.50V | 512 | 50% |
| 3.75V | 768 | 75% |
| 5.00V | 1023 | 100% |

**Formula:**

$$V_{input} = \frac{ADC_{reading}}{1023} \times V_{ref}$$

Where $V_{ref}$ is the reference voltage (usually 5V on Arduino Uno).

### The Circuit (Voltage Divider with Potentiometer)

A potentiometer is a variable resistor with 3 pins. Turning the knob moves a wiper along a resistive strip, changing the ratio of resistance above and below the wiper.

```
        5V
         │
         ┌─── Pin 1 (one end of resistive strip)
         │
    ┌────┤
    │    │◄── Wiper (Pin 2) ──── Arduino A0
    │    │
    └────┤
         │
         └─── Pin 3 (other end of resistive strip)
         │
        GND
```

As you turn the knob:
*   Full counter-clockwise: Wiper is near GND → A0 reads ~0V → ADC = 0
*   Middle: Wiper is halfway → A0 reads ~2.5V → ADC = 512
*   Full clockwise: Wiper is near 5V → A0 reads ~5V → ADC = 1023

### The Code

```c
void setup() {
    Serial.begin(9600);  // Start UART at 9600 baud for debug output
}

void loop() {
    int sensorValue = analogRead(A0);    // Read 10-bit ADC value (0-1023)

    // Convert to actual voltage
    float voltage = sensorValue * (5.0 / 1023.0);

    Serial.print("ADC: ");
    Serial.print(sensorValue);
    Serial.print("  Voltage: ");
    Serial.print(voltage, 2);  // 2 decimal places
    Serial.println("V");

    delay(200);
}
```

---

## 6. Analog Output: PWM (Pulse Width Modulation)

Microcontrollers are digital — their output pins can only be `HIGH (5V)` or `LOW (0V)`. They cannot output `2.5V`. So how do you dim an LED to 50% brightness?

**PWM (Pulse Width Modulation)** fakes it by switching the pin ON and OFF extremely fast (hundreds or thousands of times per second). Your eye (and most devices) can't see the individual flickers — they just perceive the *average* voltage.

### Duty Cycle

The **duty cycle** is the percentage of time the signal is HIGH during one period:

```
  100% Duty (always ON):
  ──────────────────────────  = Full brightness (5V average)

  75% Duty:
  ████████████████────────── = 75% brightness (3.75V average)

  50% Duty:
  ████████████──────────────  = 50% brightness (2.5V average)

  25% Duty:
  ██████──────────────────── = 25% brightness (1.25V average)

  0% Duty (always OFF):
  ──────────────────────────  = Off (0V average)
```

### The Code (LED Dimmer with Potentiometer)

```c
// Reads a potentiometer and uses PWM to dim an LED proportionally
void setup() {
    pinMode(9, OUTPUT);  // Pin 9 supports hardware PWM (marked with ~ on the board)
}

void loop() {
    int potValue = analogRead(A0);        // Read pot (0-1023)
    int brightness = potValue / 4;        // Scale to 0-255 (8-bit PWM range)
    analogWrite(9, brightness);           // Output PWM signal
    delay(15);
}
```

*   `analogWrite(9, 0)` → 0% duty → LED OFF
*   `analogWrite(9, 127)` → ~50% duty → LED at half brightness
*   `analogWrite(9, 255)` → 100% duty → LED at full brightness

---

## 7. Serial Communication (UART)

You've already built a UART driver from scratch in ZeroHAL! Now let's understand the protocol more deeply.

### How UART Works at the Wire Level

UART (Universal Asynchronous Receiver/Transmitter) sends data one bit at a time over a single wire. Both sides must agree on the **baud rate** (bits per second) beforehand, since there is no shared clock wire.

**Sending the ASCII character 'A' (0x41 = 0b01000001) at 9600 baud:**

```
Idle ──┐
       │  ┌─Start Bit (always 0, tells receiver "data incoming!")
  5V   │  │
       │  │   D0  D1  D2  D3  D4  D5  D6  D7  Stop
  HIGH ───┘   ┌───────────────────┐   ┌───┐   ┌───── Idle
              │                   │   │   │   │
  LOW    ─────┘   1   0   0   0   └───┘   └───┘
              ▲                               ▲
              │   Least Significant Bit       │
              │   first: 1,0,0,0,0,0,1,0     │
              │   = 0b01000001 = 'A'          │
              │                               │
              104µs per bit (1/9600 baud)     Stop Bit (always 1)
```

**Key Parameters:**
*   **Baud Rate:** 9600 means 9600 bits per second. Each bit lasts $\frac{1}{9600} = 104\mu s$.
*   **Data Bits:** Usually 8 (one byte per frame).
*   **Stop Bits:** Usually 1. Marks end of frame.
*   **Parity:** Optional error-detection bit (Even, Odd, or None).
*   **Common shorthand:** `9600 8N1` = 9600 baud, 8 data bits, No parity, 1 stop bit.

### Using Serial on Arduino

```c
void setup() {
    Serial.begin(115200);   // Open UART at 115200 baud
    Serial.println("System booted!");
}

void loop() {
    if (Serial.available() > 0) {
        char received = Serial.read();   // Read one byte from PC
        Serial.print("You sent: ");
        Serial.println(received);        // Echo it back
    }
}
```

Open the **Serial Monitor** in the Arduino IDE (Tools → Serial Monitor), set it to `115200 baud`, and type characters. They travel from your PC → USB → UART → MCU, and then back.

---

## 8. Communication Protocols: I2C and SPI

When you need to connect multiple chips together (sensors, displays, memory), you use standardized bus protocols.

### I2C (Inter-Integrated Circuit) — The Classroom

Uses only **2 wires** to connect up to 127 devices:
*   **SDA (Serial Data):** Bidirectional data line.
*   **SCL (Serial Clock):** Clock signal generated by the Master.

```
                     ┌────────────────────────────────────────┐
         3.3V ───┬───┤ Pull-up       Pull-up                 │
                 │   │ (4.7kΩ)       (4.7kΩ)                 │
                 │   │   │              │                     │
   Arduino ──────┼───┤ SDA ─────────── SDA ──── SDA ──── SDA │
   (Master)      │   │                                        │
                 └───┤ SCL ─────────── SCL ──── SCL ──── SCL │
                     │                                        │
                     │  Sensor 1      Sensor 2    OLED Display│
                     │  (Addr: 0x48)  (Addr: 0x68)  (0x3C)   │
                     └────────────────────────────────────────┘
```

**How it works:**
1. Master sends a **START condition** (SDA goes LOW while SCL is HIGH).
2. Master sends the **7-bit address** of the device it wants to talk to (e.g., `0x48`).
3. Only the device with that address responds with an **ACK** (acknowledgment).
4. Data bytes are exchanged, each followed by an ACK/NACK.
5. Master sends a **STOP condition**.

**Arduino Example (Reading temperature from an I2C sensor):**
```c
#include <Wire.h>  // Arduino's built-in I2C library

#define TEMP_SENSOR_ADDR  0x48  // TMP102 sensor I2C address

void setup() {
    Wire.begin();           // Join I2C bus as Master
    Serial.begin(9600);
}

void loop() {
    Wire.requestFrom(TEMP_SENSOR_ADDR, 2);  // Ask for 2 bytes

    if (Wire.available() >= 2) {
        uint8_t msb = Wire.read();   // High byte
        uint8_t lsb = Wire.read();   // Low byte

        // TMP102: 12-bit temperature, 0.0625°C per count
        int16_t raw = (msb << 4) | (lsb >> 4);
        float tempC = raw * 0.0625;

        Serial.print("Temperature: ");
        Serial.print(tempC, 1);
        Serial.println(" °C");
    }
    delay(1000);
}
```

### SPI (Serial Peripheral Interface) — The Highway

Uses **4 wires** and is much faster than I2C (up to 80 MHz vs I2C's typical 400 kHz):

| Wire | Name | Direction | Purpose |
|---|---|---|---|
| MOSI | Master Out, Slave In | Master → Slave | Data from MCU to device |
| MISO | Master In, Slave Out | Slave → Master | Data from device to MCU |
| SCK | Serial Clock | Master → Slave | Clock signal |
| CS/SS | Chip Select | Master → Slave | LOW = this device is selected |

```
                  ┌──────────────────────────────┐
                  │                              │
   Arduino ───────┤ MOSI ──────────── MOSI       │
   (Master)       │ MISO ──────────── MISO       │  SD Card
                  │ SCK  ──────────── SCK        │  Module
                  │ Pin 10 (CS) ───── CS         │
                  │                              │
                  └──────────────────────────────┘
```

**Key Difference from I2C:** Each SPI device needs its own dedicated CS (Chip Select) wire. The Master pulls CS LOW to select a specific device before sending data. This means more wires, but much faster communication.

### I2C vs SPI — When to Use Which?

| Feature | I2C | SPI |
|---|---|---|
| Wires needed | 2 (shared bus) | 4 + 1 CS per device |
| Speed | Slow-Medium (100-400 kHz) | Fast (1-80 MHz) |
| Max devices | 127 (addressed) | Limited by CS pins |
| Best for | Sensors, small config chips | SD cards, displays, high-speed ADCs |
| Complexity | Medium (addressing, ACK/NACK) | Simple (just shift data in/out) |

---

## 9. The C Language Essentials for Embedded

### Bitwise Operations (The Bread and Butter)

In embedded, you manipulate individual bits inside hardware registers. These six operators are essential:

```c
// AND (&) — Read/mask specific bits
// "Is bit 5 set in the status register?"
if (status_reg & (1 << 5)) {
    // Yes, bit 5 is 1
}

// OR (|) — Set specific bits WITHOUT touching others
// "Turn ON bit 3"
control_reg |= (1 << 3);

// AND-NOT (& ~) — Clear specific bits WITHOUT touching others
// "Turn OFF bit 3"
control_reg &= ~(1 << 3);

// XOR (^) — Toggle specific bits
// "Flip bit 5 (if 1 → 0, if 0 → 1)"
output_reg ^= (1 << 5);

// Left Shift (<<) — Create a bitmask
// (1 << 5)  = 0b00100000 = 0x20 = 32
// (1 << 0)  = 0b00000001 = 0x01 = 1

// Right Shift (>>) — Extract bits
// "Get the upper 4 bits of a byte"
uint8_t upper = (byte >> 4) & 0x0F;
```

### The `volatile` Keyword

```c
volatile uint32_t *status = (volatile uint32_t *)0x40004400;
```

*   **Without `volatile`:** The compiler says *"I already read this memory address and it was 0. Why would I read it again? I'll just use 0."* This is a valid optimization for normal RAM.
*   **With `volatile`:** You tell the compiler *"This memory location can change at any time because it's connected to physical hardware. ALWAYS re-read it from the actual address."*
*   **Rule:** Every hardware register pointer and every variable shared with an ISR (Interrupt Service Routine) **must** be declared `volatile`.

### Structs for Hardware Registers

Instead of dozens of `#define` macros, professional embedded code maps an entire peripheral's registers to a C struct:

```c
typedef struct {
    volatile uint32_t SR;    // Offset 0x00: Status Register
    volatile uint32_t DR;    // Offset 0x04: Data Register
    volatile uint32_t BRR;   // Offset 0x08: Baud Rate Register
    volatile uint32_t CR1;   // Offset 0x0C: Control Register 1
    volatile uint32_t CR2;   // Offset 0x10: Control Register 2
    volatile uint32_t CR3;   // Offset 0x14: Control Register 3
} USART_TypeDef;

// Cast the base address to a pointer to this struct
#define USART2 ((USART_TypeDef *)0x40004400)

// Now access registers cleanly:
USART2->CR1 |= (1 << 13);  // Enable USART
USART2->DR = 'A';           // Transmit character 'A'
```

This works because C structs are laid out sequentially in memory, matching the hardware register layout perfectly.

---

## 10. Putting It All Together: A Complete Project

**Project: Smart Plant Monitor**

Build a device that reads soil moisture, displays the value over Serial, and automatically turns on a water pump when the soil is dry.

### Components Needed
*   Arduino Uno (\$5-15)
*   Soil moisture sensor (capacitive, ~\$2)
*   5V relay module (~\$2) — acts as a MOSFET switch for the pump
*   Small water pump (\$3)
*   Breadboard and jumper wires (\$5)

### Circuit

```
                    Arduino Uno
                   ┌───────────┐
        5V ────────┤ 5V        │
        GND ───────┤ GND       │
                   │           │
  Soil Sensor ─────┤ A0        │   (Analog: 0=wet, 1023=dry)
                   │           │
  Relay Signal ────┤ Pin 7     │   (Digital: HIGH=pump ON)
                   │           │
                   └───────────┘

  Relay Module:
  ┌─────────┐
  │ VCC ────┤── 5V
  │ GND ────┤── GND
  │ IN  ────┤── Arduino Pin 7
  │         │
  │ COM ────┤── 5V Power Supply (+)
  │ NO  ────┤── Water Pump (+)
  └─────────┘
  Water Pump (-) ── 5V Power Supply (-)
```

### The Code

```c
#define MOISTURE_PIN   A0
#define RELAY_PIN      7
#define DRY_THRESHOLD  700    // ADC value above which soil is "dry"
#define WET_THRESHOLD  400    // ADC value below which soil is "wet enough"

void setup() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);  // Pump OFF at boot
    Serial.begin(9600);
    Serial.println("[PlantBot] System Online.");
}

void loop() {
    int moisture = analogRead(MOISTURE_PIN);

    Serial.print("Soil Moisture: ");
    Serial.print(moisture);

    if (moisture > DRY_THRESHOLD) {
        Serial.println(" → DRY! Watering...");
        digitalWrite(RELAY_PIN, HIGH);  // Turn pump ON
    } else if (moisture < WET_THRESHOLD) {
        Serial.println(" → Moist. Pump OFF.");
        digitalWrite(RELAY_PIN, LOW);   // Turn pump OFF
    } else {
        Serial.println(" → OK.");
    }

    delay(2000);  // Check every 2 seconds
}
```

### What You Learn from This Project
1. **ADC** — Reading an analog sensor value from the physical world.
2. **Digital Output** — Controlling a relay (which controls a motor).
3. **Thresholds & Hysteresis** — Using two thresholds (`DRY` and `WET`) instead of one prevents the pump from rapidly toggling on/off at the boundary.
4. **Serial Debugging** — Using UART to monitor sensor values in real-time.
5. **Power Management** — The relay isolates the MCU from the high-current pump circuit.

---

## 11. Common Beginner Mistakes

| Mistake | What Happens | Fix |
|---|---|---|
| Forgetting a current-limiting resistor on an LED | LED burns out instantly | Always use 220Ω-1kΩ in series |
| Floating input pin (no pull-up/pull-down) | Random readings, "ghost" button presses | Use `INPUT_PULLUP` or add external resistor |
| Powering motors directly from MCU pins | MCU resets or burns the pin driver | Use a MOSFET or relay to switch motor power |
| Not sharing GND between devices | Communication fails completely | ALL devices must share a common ground wire |
| Using `delay()` for everything | MCU can't do anything else while waiting | Use `millis()` for non-blocking timing (like your ZeroHAL scheduler!) |
| Ignoring switch bounce | One press registers as 10+ presses | Add 50ms software debounce |

---

## 12. What to Buy (Starter Kit)

| Item | Approx. Cost | Purpose |
|---|---|---|
| Arduino Uno (clone is fine) | \$5-10 | The brain |
| Breadboard (830 tie-points) | \$3 | Prototyping without soldering |
| Jumper wire kit (M-M, M-F) | \$3 | Connecting components |
| LED assortment (red, green, blue) | \$2 | Visual output |
| Resistor kit (220Ω, 1kΩ, 10kΩ) | \$3 | Current limiting, pull-ups |
| Tactile push buttons (x10) | \$1 | Digital input |
| Potentiometer (10kΩ, x3) | \$2 | Analog input |
| Digital Multimeter | \$10-15 | Measuring and debugging |
| USB-A to USB-B cable | \$2 | Programming the Arduino |
| **Total** | **~\$30-40** | |

---

## 13. Resources

*   **Arduino Official Tutorials:** [arduino.cc/en/Tutorial/HomePage](https://www.arduino.cc/en/Tutorial/HomePage) — Start with "Built-In Examples."
*   **Falstad Circuit Simulator:** [falstad.com/circuit](https://falstad.com/circuit/) — Free browser-based simulator. Draw circuits and watch current flow in real-time.
*   **YouTube - Ben Eater:** Builds an entire 8-bit computer from scratch on breadboards. The absolute best for understanding how CPUs actually work at the gate level.
*   **YouTube - GreatScott!:** Practical, project-based electronics tutorials. Excellent for learning to solder and build real circuits.
*   **Book:** *"Make: Electronics" by Charles Platt* — Hands-on, experiment-first approach. Great for absolute beginners.
*   **Book:** *"The Art of Electronics" by Horowitz & Hill* — The definitive electronics reference. Dense but comprehensive.

---

**Next Step:** Once you're comfortable reading sensors and controlling outputs with Arduino, move to **Phase 3: Removing the Training Wheels** — where you ditch the Arduino libraries and write bare-metal register code on 32-bit ARM chips (exactly what your ZeroHAL project does!).
