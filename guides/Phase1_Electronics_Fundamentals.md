# Phase 1: Electronics & Electrical Fundamentals
**The Complete Physics Foundation for Embedded Systems Engineers**

Everything in embedded systems — every line of code, every blinking LED, every sensor reading — ultimately reduces to the controlled movement of electrons through materials. This document is your complete reference.

---

## Part I: The Atom and Electric Charge

### What Is Electricity?

All matter is made of atoms. Every atom has:

```
                    Atom
              ┌──────────────┐
              │   Nucleus    │
              │  ┌────────┐  │
              │  │ P+ P+  │  │   P+ = Proton  (positive charge)
              │  │ N  N   │  │   N  = Neutron  (no charge)
              │  └────────┘  │
              │              │
          e-  ·    ·    ·  e-│   e- = Electron (negative charge)
              │              │
              └──────────────┘
```

*   **Protons (+):** Live in the nucleus. They are stuck in place and don't move.
*   **Electrons (-):** Orbit the nucleus. In conductive materials (metals like copper), the outermost electrons are loosely bound and can drift freely between atoms. These are called **free electrons**.
*   **Electricity** is the flow of these free electrons from one atom to the next, pushed along by an electric field (voltage).

### Conductors, Insulators, and Semiconductors

| Material Type | What Happens | Examples |
|---|---|---|
| **Conductor** | Electrons move freely. Minimal resistance. | Copper, Gold, Aluminum, Silver |
| **Insulator** | Electrons are tightly bound. No flow. | Rubber, Glass, Plastic, Air |
| **Semiconductor** | Conductivity can be *controlled*. This is the basis of all modern electronics. | Silicon, Germanium |

**Why Silicon is special:** Pure silicon is a poor conductor. But by adding tiny amounts of other elements ("doping"), we can precisely control how and where electrons flow. This is how transistors, diodes, and every chip in your computer are made.

---

## Part II: The Four Fundamental Quantities

### 1. Charge (Coulombs / Q)

*   Charge is the fundamental property that causes electromagnetic interaction.
*   One electron carries a charge of $1.6 \times 10^{-19}$ Coulombs.
*   **1 Coulomb** = approximately $6.24 \times 10^{18}$ electrons (6.24 billion billion electrons).
*   Charge is measured in **Coulombs (C)**.

### 2. Voltage (Volts / V)

*   **Definition:** The electrical pressure (or potential difference) between two points. It is the *force* that pushes electrons through a conductor.
*   **Formula:** $V = \frac{W}{Q}$ (Voltage = Energy per unit Charge, measured in Joules per Coulomb).
*   **Analogy:** The height difference of a waterfall. Water at the top has potential energy. When it falls, that energy is converted to kinetic energy. Similarly, electrons at a higher voltage have more potential energy.

```
    Battery
    ┌──────┐
    │ +  9V│ ← High potential (top of waterfall)
    │      │
    │      │    Voltage = 9V - 0V = 9V difference
    │      │
    │ -  0V│ ← Low potential / Ground (bottom of waterfall)
    └──────┘
```

*   **Key insight:** Voltage is ALWAYS measured *between two points*. Saying "this wire is 5V" actually means "this wire is 5V *relative to Ground (0V)*."
*   A **battery** creates voltage by using a chemical reaction to physically separate positive and negative charges to opposite terminals, creating pressure.

### 3. Current (Amperes / I)

*   **Definition:** The *rate* of flow of electric charge past a point per second.
*   **Formula:** $I = \frac{Q}{t}$ (Current = Charge ÷ Time).
*   **1 Ampere** = 1 Coulomb of charge flowing past a point in 1 second.
*   **Analogy:** The gallons per minute of water flowing through a pipe.

**Conventional Current vs Electron Flow:**

```
    Battery (+) ────────────────────── Battery (-)

    Conventional Current ──────────→  (Positive to Negative)
    (Used in all circuit diagrams)

    Actual Electron Flow  ←──────────  (Negative to Positive)
    (What physically happens)
```

Benjamin Franklin guessed wrong in the 1700s — he assumed positive charges moved. By the time we discovered electrons, all the math and conventions were already built around "conventional current" flowing from + to -. It still works perfectly; just know that the actual electrons move the opposite direction.

**Common Current Magnitudes:**

| Value | Abbreviation | Example |
|---|---|---|
| 1 µA (microamp) | $10^{-6}$ A | Sleep-mode MCU |
| 1 mA (milliamp) | $10^{-3}$ A | Single LED |
| 20 mA | 0.02 A | Max output of one Arduino pin |
| 500 mA | 0.5 A | USB 2.0 port max |
| 1 A | 1 A | Small DC motor |
| 10+ A | 10 A | Car starter motor |
| 100+ mA through your body | — | **Potentially lethal** |

### 4. Resistance (Ohms / Ω)

*   **Definition:** The opposition to current flow in a material.
*   **Analogy:** A narrow pipe. The narrower the pipe, the harder it is for water to flow, and the more pressure (voltage) you need to maintain the same flow rate (current).
*   Every material has some resistance. Copper wire has very low resistance. Rubber has extremely high resistance.
*   **A Resistor** is a component specifically designed to provide a precise, known amount of resistance.

---

## Part III: Ohm's Law — The Golden Rule

### The Formula

$$\boxed{V = I \times R}$$

This can be rearranged three ways depending on what you're solving for:

```
         ┌─────┐
         │  V  │        V = I × R   (Find Voltage)
         ├──┬──┤
         │ I│ R│        I = V / R   (Find Current)
         └──┴──┘
                        R = V / I   (Find Resistance)
```

### Worked Examples

**Example 1: How much current flows through a 1kΩ resistor connected to 5V?**

$$I = \frac{V}{R} = \frac{5V}{1000\Omega} = 0.005A = 5\text{ mA}$$

**Example 2: What resistor do I need to limit an LED to 15mA on a 3.3V MCU pin?**

An LED has a forward voltage drop of approximately 2V. The resistor must absorb the remaining voltage:

$$V_{resistor} = V_{supply} - V_{LED} = 3.3V - 2.0V = 1.3V$$

$$R = \frac{V}{I} = \frac{1.3V}{0.015A} = 86.7\Omega$$

You'd pick the nearest standard value: **100Ω**.

**Example 3: How much power does that resistor dissipate?**

$$P = V \times I = 1.3V \times 0.015A = 0.0195W \approx 20\text{ mW}$$

A standard 1/4W (250mW) resistor handles this easily.

---

## Part IV: Power and Energy

### Power (Watts / W)

Power is the *rate* at which energy is used or converted. A device that consumes 1 Watt uses 1 Joule of energy every second.

**Three equivalent formulas for power:**

$$P = V \times I \quad\quad P = I^2 \times R \quad\quad P = \frac{V^2}{R}$$

Use whichever formula matches the values you already know.

### Energy (Watt-hours / Wh)

Energy is power consumed over time:

$$E = P \times t$$

*   A device consuming 0.5W for 10 hours uses $0.5 \times 10 = 5\text{ Wh}$ of energy.
*   **Battery Capacity** is measured in **milliamp-hours (mAh)**. A 2000mAh battery can supply 2000mA for 1 hour, or 200mA for 10 hours, or 20mA for 100 hours.
*   **Embedded relevance:** If your MCU consumes 20mA at 3.3V, and your battery is 500mAh, your device runs for $\frac{500}{20} = 25\text{ hours}$.

### Heat Dissipation

Wasted power turns into **heat**. This is critical in embedded:
*   A voltage regulator converting 12V to 3.3V at 500mA wastes $(12 - 3.3) \times 0.5 = 4.35W$ as heat. That's hot enough to burn your finger. This is why switching regulators (which waste very little) are preferred over linear regulators in most designs.

---

## Part V: Circuit Topologies

### Series Circuits

Components are connected end-to-end. Current has only ONE path.

```
    Battery (+)
         │
        ┌┴┐
        │ │ R1 = 100Ω
        └┬┘
         │
        ┌┴┐
        │ │ R2 = 200Ω
        └┬┘
         │
        ┌┴┐
        │ │ R3 = 300Ω
        └┬┘
         │
    Battery (-)
```

**Rules for Series:**

| Property | Rule |
|---|---|
| Current | **Same** through all components: $I_{total} = I_1 = I_2 = I_3$ |
| Voltage | **Divides** across components: $V_{total} = V_1 + V_2 + V_3$ |
| Resistance | **Adds up**: $R_{total} = R_1 + R_2 + R_3 = 100 + 200 + 300 = 600\Omega$ |

### Parallel Circuits

Components are connected side-by-side. Current has MULTIPLE paths.

```
    Battery (+) ──────┬──────┬──────┐
                      │      │      │
                     ┌┴┐    ┌┴┐    ┌┴┐
                     │ │    │ │    │ │
                     │R1│    │R2│    │R3│
                     │ │    │ │    │ │
                     └┬┘    └┬┘    └┬┘
                      │      │      │
    Battery (-) ──────┴──────┴──────┘
```

**Rules for Parallel:**

| Property | Rule |
|---|---|
| Voltage | **Same** across all components: $V_{total} = V_1 = V_2 = V_3$ |
| Current | **Divides** among paths: $I_{total} = I_1 + I_2 + I_3$ |
| Resistance | **Reciprocal sum**: $\frac{1}{R_{total}} = \frac{1}{R_1} + \frac{1}{R_2} + \frac{1}{R_3}$ |

**Shortcut for two parallel resistors:**

$$R_{total} = \frac{R_1 \times R_2}{R_1 + R_2}$$

**Example:** Two 10kΩ resistors in parallel:

$$R_{total} = \frac{10k \times 10k}{10k + 10k} = \frac{100M}{20k} = 5k\Omega$$

**Key insight:** Parallel resistance is always LESS than the smallest individual resistor. Adding more parallel paths makes it easier for current to flow.

---

## Part VI: Kirchhoff's Laws

### Kirchhoff's Voltage Law (KVL)

> *The sum of all voltages around any closed loop in a circuit equals zero.*

In simpler terms: the voltage the battery provides must be entirely consumed by the components in the loop.

**Example: LED Circuit Analysis**

```
    9V Battery (+)
         │
        ┌┴┐
        │ │ R = 330Ω     ← Voltage across R = ?
        └┬┘
         │
         ▼
        ─┬─ LED           ← Voltage across LED = 2V (given)
         │
    9V Battery (-)
```

Applying KVL around the loop:

$$V_{battery} - V_R - V_{LED} = 0$$
$$9V - V_R - 2V = 0$$
$$V_R = 7V$$

Now find the current using Ohm's Law:

$$I = \frac{V_R}{R} = \frac{7V}{330\Omega} = 21.2\text{ mA}$$

### Kirchhoff's Current Law (KCL)

> *The total current entering a node (junction) must equal the total current leaving it.*

Electrons don't appear or disappear — what goes in must come out.

```
         5mA (entering)
          │
          ▼
    ──────●────── → 3mA (leaving through R1)
          │
          ▼
          2mA (leaving through R2)
```

$I_{in} = I_{out1} + I_{out2}$ → $5\text{mA} = 3\text{mA} + 2\text{mA}$ ✓

---

## Part VII: Voltage Dividers

One of the most common and important circuits in all of electronics. Two resistors in series create a predictable fraction of the input voltage at their junction.

### The Circuit

```
    Vin (e.g., 5V)
         │
        ┌┴┐
        │ │ R1
        └┬┘
         │
         ├──── Vout (the divided voltage)
         │
        ┌┴┐
        │ │ R2
        └┬┘
         │
        GND
```

### The Formula

$$V_{out} = V_{in} \times \frac{R_2}{R_1 + R_2}$$

### Worked Example

$R_1 = 10k\Omega$, $R_2 = 10k\Omega$, $V_{in} = 5V$:

$$V_{out} = 5V \times \frac{10k}{10k + 10k} = 5V \times 0.5 = 2.5V$$

Equal resistors divide the voltage exactly in half.

### Embedded Uses

*   **Level Shifting:** A 5V sensor output needs to connect to a 3.3V MCU input. Use a voltage divider ($R_1 = 10k\Omega$, $R_2 = 20k\Omega$) to scale 5V down to 3.3V.
*   **Battery Monitoring:** Read a 12V car battery with a 3.3V ADC by dividing the voltage down.
*   **Potentiometers:** A potentiometer IS a variable voltage divider. Turning the knob changes the ratio of $R_1$ to $R_2$.

**⚠ Warning:** Voltage dividers are NOT power supplies. If you draw significant current from $V_{out}$, the voltage drops because the divider has high output impedance. Use a voltage regulator when you need to power a circuit at a specific voltage.

---

## Part VIII: Passive Components (Deep Dive)

### Resistors

**Color Code (4-Band):**

```
    ┌────────────────────────────────────────┐
    │  Band 1    Band 2    Band 3    Band 4  │
    │  (1st      (2nd      (Multi-   (Tole-  │
    │   digit)    digit)    plier)    rance)  │
    └────────────────────────────────────────┘

    Color       Digit   Multiplier   Tolerance
    ─────────   ─────   ──────────   ─────────
    Black       0       ×1
    Brown       1       ×10          ±1%
    Red         2       ×100         ±2%
    Orange      3       ×1,000
    Yellow      4       ×10,000
    Green       5       ×100,000     ±0.5%
    Blue        6       ×1,000,000   ±0.25%
    Violet      7
    Grey        8
    White       9
    Gold                ×0.1         ±5%
    Silver              ×0.01        ±10%
```

**Example:** Brown-Black-Orange-Gold = 1, 0, ×1000 = **10,000Ω (10kΩ) ±5%**

**Standard Resistor Values (E12 Series):**
The E12 series gives 12 values per decade. Memorize these — they are the values you'll actually buy:

$1.0,\ 1.2,\ 1.5,\ 1.8,\ 2.2,\ 2.7,\ 3.3,\ 3.9,\ 4.7,\ 5.6,\ 6.8,\ 8.2$

These repeat at every power of 10: 10Ω, 12Ω, 15Ω... 100Ω, 120Ω, 150Ω... 1kΩ, 1.2kΩ, 1.5kΩ... etc.

**Types of Resistors:**

| Type | Use |
|---|---|
| Through-Hole (THT) | Breadboard prototyping. Has wire leads. |
| SMD (Surface Mount) | Professional PCBs. Tiny (0402 = 1mm × 0.5mm). |
| Potentiometer | Variable resistor. Knob or slider. |
| Thermistor | Resistance changes with temperature. Used as temp sensors. |
| LDR (Light Dependent Resistor) | Resistance changes with light. Used in light sensors. |
| Pull-up / Pull-down | Holds a digital pin at a defined voltage level. |

### Capacitors

A capacitor stores energy in an **electric field** between two conductive plates separated by an insulating material (dielectric).

**Capacitance** is measured in **Farads (F)**. 1 Farad is enormous. In practice, we use:

| Unit | Value | Typical Use |
|---|---|---|
| pF (picofarad) | $10^{-12}$ F | RF circuits, crystal oscillator load caps |
| nF (nanofarad) | $10^{-9}$ F | Signal filtering |
| µF (microfarad) | $10^{-6}$ F | Decoupling, power supply filtering |
| mF (millifarad) | $10^{-3}$ F | Energy storage, motor start |

**Charging and Discharging:**

When you connect a capacitor to a voltage source, it charges up. The voltage across it rises exponentially, not instantly:

```
    Voltage across capacitor
    │
  V ├─────────────────────── ← Fully charged (approaches V asymptotically)
    │        ╱────────────
    │      ╱
    │    ╱
    │  ╱
    │╱
    ├──────────────────────── Time
    0    τ    2τ    3τ   5τ

    τ (tau) = R × C = Time Constant
    At 1τ: 63.2% charged
    At 3τ: 95.0% charged
    At 5τ: 99.3% charged (effectively "fully" charged)
```

**The Time Constant ($\tau = R \times C$):**

This is critically important. It tells you how fast a capacitor charges or discharges.

*   $R = 10k\Omega$, $C = 100\mu F$: $\tau = 10000 \times 0.0001 = 1\text{ second}$
*   $R = 1k\Omega$, $C = 0.1\mu F$: $\tau = 1000 \times 0.0000001 = 0.1\text{ ms}$

**Types of Capacitors:**

| Type | Characteristics | Use |
|---|---|---|
| Ceramic (MLCC) | Small, cheap, no polarity, 1pF–10µF | Decoupling, filtering. The most common in embedded. |
| Electrolytic | Large capacitance (1µF–10,000µF), **HAS POLARITY** (+ and - legs) | Bulk power filtering. **Inserting backwards can cause explosion.** |
| Tantalum | Like electrolytic but smaller and more stable. **HAS POLARITY.** | Space-constrained power filtering. |
| Supercapacitor | Very large (0.1F–100F+), slow charge/discharge | Backup power, energy harvesting. |

**The Critical Embedded Use — Decoupling:**

Every digital IC (including your STM32) MUST have a `0.1µF` (100nF) ceramic capacitor placed as physically close as possible to its power pin (VDD/VCC). Here's why:

When a digital chip switches millions of transistors simultaneously, it creates sudden spikes of current demand. The wire from the power supply has inductance (even a few nanohenries), which resists sudden current changes. This causes the supply voltage to momentarily dip ("droop"), which can crash the MCU or cause data corruption.

The decoupling cap acts as a tiny local battery. It provides the burst of current instantly from its stored charge, while the main power supply catches up through the wires.

```
    Power Supply ──── Long Wire (has inductance) ──── VDD Pin
                                                        │
                                                      ┌─┴─┐
                                                      │0.1µF│ ← Decoupling Cap
                                                      └─┬─┘
                                                        │
                                                       GND Pin
```

**Rule of thumb:** One 0.1µF cap per VDD pin, plus one 10µF bulk cap per power domain.

### Inductors

An inductor stores energy in a **magnetic field** created by current flowing through a coil of wire.

**Inductance** is measured in **Henrys (H)**. In practice: µH (microhenries) and mH (millihenries).

**Key Behavior:** An inductor resists *changes* in current. If current is flowing and you try to suddenly stop it, the inductor generates a voltage spike (back-EMF) to try to keep the current going. This can be destructive if not handled properly.

**Embedded Use — Switching Power Supplies (Buck Converter):**

A buck converter efficiently steps down voltage (e.g., 12V → 3.3V) using a switch (MOSFET), an inductor, a diode, and a capacitor. It rapidly switches the input on and off, and the inductor smooths the resulting pulses into a steady output voltage. Efficiency is typically 85-95%, vs 30-50% for a linear regulator.

```
    12V ──[MOSFET Switch]──┬──[Inductor]──┬── 3.3V Output
                           │              │
                        [Diode]        [Cap]
                           │              │
                          GND            GND
```

---

## Part IX: Active Components (Semiconductors)

### Diodes

A diode is a one-way valve for current. It is made of a **PN junction** — a piece of silicon with one half doped to have excess electrons (N-type) and the other half doped to have "holes" where electrons are missing (P-type).

```
    Anode (+)  ──▶|──  Cathode (-)

    Forward Bias (current flows):
    (+) ──▶|── (-)     Anode is more positive than Cathode. Current flows.
                        There is a forward voltage drop (Vf ≈ 0.7V for silicon).

    Reverse Bias (current blocked):
    (-) ──▶|── (+)     Cathode is more positive than Anode. No current flows.
                        (Up to the breakdown voltage, then it conducts destructively)
```

**Types of Diodes:**

| Type | Forward Drop | Purpose |
|---|---|---|
| Standard Silicon | ~0.7V | Rectification, reverse polarity protection |
| Schottky | ~0.2-0.3V | Low-loss rectification, power supplies |
| Zener | Specified (e.g., 3.3V) | Voltage regulation/clamping (conducts in reverse at a precise voltage) |
| LED | ~1.8-3.3V (varies by color) | Light emission |
| TVS (Transient Voltage Suppressor) | Varies | ESD (Electrostatic Discharge) protection |

**LED Colors and Forward Voltage:**

| Color | Approximate Forward Voltage |
|---|---|
| Infrared | 1.2V |
| Red | 1.8V |
| Orange | 2.0V |
| Yellow | 2.1V |
| Green | 2.2V |
| Blue | 3.0V |
| White | 3.2V |

### Transistors

A transistor is an electronically controlled switch (or amplifier). It is the most important invention of the 20th century and the foundation of all digital computing.

**There are two main families:**

#### BJT (Bipolar Junction Transistor)

Current-controlled. A small current into the Base controls a large current between Collector and Emitter.

```
    NPN BJT:

         Collector (C) ← High-current path (top)
              │
              ├── Base (B) ← Small control current (~1mA)
              │
         Emitter (E) ← High-current path (bottom, connects to GND)


    Usage: MCU pin (3.3V) → 1kΩ Resistor → Base
           This lets ~3mA flow into Base.
           The BJT amplifies this by its gain (hFE ≈ 100-300):
           Collector current = 3mA × 100 = 300mA → enough to drive a motor!
```

**BJT Types:**
*   **NPN:** Current flows from Collector to Emitter when Base is driven HIGH. Most common.
*   **PNP:** Current flows from Emitter to Collector when Base is driven LOW. Used for high-side switching.

#### MOSFET (Metal-Oxide-Semiconductor Field-Effect Transistor)

Voltage-controlled. A voltage on the Gate controls current between Drain and Source. **This is what is used inside every modern CPU and MCU — billions of them.**

```
    N-Channel MOSFET:

         Drain (D)  ← High-current path (connects to load)
              │
              ├── Gate (G) ← Control voltage (draws almost zero current!)
              │
         Source (S) ← High-current path (connects to GND)
```

**Key Advantages over BJT:**
*   Gate draws virtually **zero current** — perfect for MCU pins.
*   Can switch very high currents (10A, 50A, 100A+) with minimal losses.
*   Switches much faster than BJTs (important for PWM).

**MOSFET Parameters to Know:**

| Parameter | Meaning |
|---|---|
| $V_{GS(th)}$ (Gate Threshold Voltage) | Minimum Gate-Source voltage to turn ON. For a 3.3V MCU, you need a "logic-level" MOSFET with $V_{GS(th)}$ < 2.5V. |
| $R_{DS(on)}$ (On-Resistance) | Resistance when fully ON. Lower is better. 10-50mΩ is common. |
| $I_D$ (Max Drain Current) | Maximum continuous current through the MOSFET. |
| $V_{DS}$ (Max Drain-Source Voltage) | Maximum voltage across the MOSFET when OFF. |

**Practical MOSFET Circuit (Driving a 12V Motor from a 3.3V MCU):**

```
         12V
          │
      ┌───┴───┐
      │ Motor  │  (draws 2A)
      └───┬───┘
          │
          Drain (D)
          │
     ┌────┤ IRLZ44N (N-Channel Logic-Level MOSFET)
     │    │
     │    Gate (G) ─── 1kΩ ─── MCU Pin (3.3V or 0V)
     │    │
     │    Source (S)
     │    │
     │   GND ───────────────── MCU GND (MUST share common ground!)
     │
     │   Flyback Diode
     └────▶|──── 12V  (protects MOSFET from motor's back-EMF spike)
```

**Why the Flyback Diode?** Motors are inductors. When you turn them OFF, the collapsing magnetic field generates a massive reverse voltage spike (back-EMF) that can be 10-100× the supply voltage. Without the diode, this spike destroys the MOSFET. The diode provides a safe path for this energy to dissipate.

---

## Part X: Power Supply Design Basics

Every embedded system needs a stable, clean power supply. Garbage power = garbage behavior (random resets, corrupted data, noise on ADC readings).

### Linear Voltage Regulators

Takes a higher voltage and drops it to a lower voltage. The excess energy is wasted as heat.

```
    Vin (7-12V) ──┬──[ LM7805 ]──┬── Vout (5V, stable)
                  │               │
               [Cap 0.33µF]   [Cap 0.1µF]
                  │               │
                 GND             GND
```

*   **Advantages:** Simple, cheap, low noise, no switching artifacts.
*   **Disadvantages:** Extremely wasteful. Power wasted = $(V_{in} - V_{out}) \times I_{out}$.
*   **Example:** 12V in, 5V out, 500mA → Wasted: $(12-5) \times 0.5 = 3.5W$ of pure heat.
*   **Dropout Voltage:** The minimum difference between Vin and Vout for the regulator to work. For LM7805, it's ~2V (so Vin must be ≥ 7V). **LDO (Low Dropout) regulators** can work with as little as 0.1-0.3V difference.

### Switching Regulators (Buck Converter)

Uses rapid switching (100kHz-2MHz) and an inductor to efficiently convert voltage.

*   **Advantages:** 85-95% efficient. Very little heat.
*   **Disadvantages:** More complex circuit, generates electrical noise (EMI), more expensive.
*   **When to use:** Whenever efficiency matters (battery-powered devices, high current, large voltage difference).

### Battery Technologies for Embedded

| Battery | Voltage | Rechargeable | Notes |
|---|---|---|---|
| CR2032 (Coin Cell) | 3.0V | No | Low current (~5mA). Great for RTC backup, BLE beacons. |
| AA Alkaline | 1.5V | No | Cheap, widely available. Use 2 in series for 3V. |
| Li-Ion / LiPo | 3.7V (nominal) | Yes | High energy density. Requires charge controller IC. Can catch fire if mishandled. |
| 18650 (Li-Ion) | 3.7V | Yes | Standard rechargeable cell used in laptops, EVs. |
| NiMH (AA) | 1.2V | Yes | Drop-in rechargeable replacement for AA. |

---

## Part XI: Reading Schematics

A schematic is a circuit diagram using standardized symbols. It is the "source code" of hardware. Learning to read schematics is like learning to read code.

### Common Symbols

```
    ─/\/\/─     Resistor
    ─||─        Capacitor
    ─)(─        Inductor
    ──▶|──      Diode (arrow points in direction of conventional current flow)
    ──▶|──      LED (same as diode, but with two arrows pointing out = light)

                 C
                 │
    NPN BJT:  B──┤      (Arrow on Emitter points OUT for NPN)
                 │
                 E
                 ↓

                 D
                 │
    N-MOSFET: G──┤      (Arrow on body diode points from S to D)
                 │
                 S

    ─┤├─        Crystal Oscillator
    ┌──┐
    │ + │       Battery (long line = positive terminal)
    └──┘

    ▽ or ⏚     Ground (GND, 0V reference)

    ── VCC      Power rail label (3.3V or 5V, depends on design)
```

### Net Labels and Power Symbols

In complex schematics, wires don't all physically connect on the page. Instead, engineers use **net labels** — if two wires have the same label, they are electrically connected even if they're on different pages.

```
    On page 1:  MCU Pin PA2 ──── UART_TX
    On page 5:  UART_TX ──── Connector Pin 3

    These are the same wire.
```

### How to Read a Schematic (Step by Step)

1. **Find the power supply section.** Trace where power enters and how it gets regulated.
2. **Find the main IC (MCU).** This is the brain. All other components connect to it.
3. **Follow the signal paths.** Trace from a sensor, through any conditioning circuits, to the MCU pin.
4. **Check decoupling caps.** Every IC should have 0.1µF caps near its power pins.
5. **Look for pull-up/pull-down resistors** on communication buses (I2C always needs 4.7kΩ pull-ups).

---

## Part XII: Lab Safety

### Voltage Danger Levels

| Voltage | Risk |
|---|---|
| 0 - 12V DC | Generally safe to touch (but high current can cause burns from short circuits melting wires) |
| 12 - 50V DC | Can be felt. Dangerous under certain wet conditions. |
| 50V+ DC / 30V+ AC | **Potentially lethal.** NEVER work on live mains (110V/220V AC) circuits. |

### ESD (Electrostatic Discharge)

Walking across carpet can charge your body to **10,000-25,000 Volts**. You don't feel it until ~3,000V. But sensitive CMOS chips (like your MCU) can be permanently damaged by as little as **100V**.

**Prevention:**
*   Touch a grounded metal object before handling chips.
*   Use an **anti-static wrist strap** connected to ground.
*   Store chips in anti-static bags.
*   Don't place boards on carpet, wool, or synthetic surfaces.

### Capacitor Safety

Large electrolytic capacitors can store dangerous amounts of energy even after the circuit is unplugged. Always discharge capacitors before working on a circuit:
*   Short the terminals through a 1kΩ resistor (not directly — a direct short can weld your screwdriver).

---

## Part XIII: Tools & Equipment

### Essential (Buy These First)

| Tool | Cost | Purpose |
|---|---|---|
| **Digital Multimeter** | \$10-20 | Measure voltage, current, resistance, continuity |
| **Breadboard** (830 point) | \$3-5 | Build circuits without soldering |
| **Jumper Wire Kit** | \$3-5 | Male-male, male-female, female-female |
| **Wire Strippers** | \$5-10 | Cleanly strip insulation from wire |
| **Flush Cutters** | \$5 | Cut component leads cleanly |

### Intermediate (Buy When Ready)

| Tool | Cost | Purpose |
|---|---|---|
| **Soldering Iron** (temp controlled) | \$20-40 | Permanent connections. Get a Pinecil or Hakko FX-888D clone. |
| **Solder** (60/40 or 63/37 leaded, 0.8mm) | \$5-10 | Leaded solder is MUCH easier to work with than lead-free for beginners. |
| **Solder Wick & Solder Sucker** | \$5 | Fixing mistakes |
| **Logic Analyzer** (Saleae clone) | \$10-15 | Capture and decode digital signals (UART, I2C, SPI) in real time. Invaluable for debugging communication. |

### Advanced (For Serious Work)

| Tool | Cost | Purpose |
|---|---|---|
| **Oscilloscope** (Rigol DS1054Z) | \$300-400 | Visualize electrical signals in real-time. See voltage waveforms, PWM duty cycles, signal integrity issues. The most powerful debugging tool in electronics. |
| **Bench Power Supply** | \$50-100 | Provide adjustable, current-limited power. Set a current limit so if you make a mistake, the supply shuts off instead of destroying your circuit. |
| **Hot Air Rework Station** | \$40-80 | Solder and desolder SMD components. |

### Using a Digital Multimeter

**Continuity Test (The Most Used Feature):**
1. Set dial to the continuity symbol (looks like a speaker/sound wave icon).
2. Touch both probes together → it should beep.
3. Touch probes to two points in your circuit.
4. **Beep = connected.** No beep = broken connection.
5. Use this to verify solder joints, find broken traces, check that your wiring is correct.

**Measuring Voltage:**
1. Set dial to **V DC** (the V with a straight line, not the wavy line).
2. **Black probe → GND.** **Red probe → point you want to measure.**
3. Read the value. You should see ~3.3V on your MCU's VDD pin.
4. **IMPORTANT: Measure voltage IN PARALLEL (probes touch the circuit while it's running, don't break the wire).**

**Measuring Current:**
1. Set dial to **A** (or mA).
2. Move the **red probe to the current jack** (often labeled "10A" or "mA").
3. **BREAK THE CIRCUIT** and put the meter IN SERIES — current must flow *through* the meter.
4. If you accidentally measure current in parallel (across a power supply), you create a short circuit through the meter's near-zero resistance. This will **blow the meter's internal fuse** (or worse).

**Measuring Resistance:**
1. **POWER OFF THE CIRCUIT COMPLETELY.**
2. Set dial to **Ω**.
3. Touch probes across the component.
4. Read the value. Verify it matches the color code on the resistor.

---

## Part XIV: Recommended Learning Resources

### Books (In Order of Difficulty)

1. **"Make: Electronics" by Charles Platt** — Hands-on, experiment-first. You learn by burning out LEDs and building circuits. Best for absolute beginners.
2. **"Practical Electronics for Inventors" by Scherz & Monk** — Comprehensive reference. Covers everything from DC theory to microcontrollers.
3. **"The Art of Electronics" by Horowitz & Hill** — The bible. Dense, thorough, and opinionated. This is the book that professional EEs keep on their desk for 30 years.

### YouTube Channels

| Channel | Focus | Why Watch |
|---|---|---|
| **Ben Eater** | Digital logic, CPU architecture | Builds an 8-bit CPU from NAND gates on breadboards. Mind-blowing. |
| **EEVblog (Dave Jones)** | Electronics fundamentals, teardowns | Australian engineer who explains everything deeply and enthusiastically. |
| **GreatScott!** | Practical projects | Project-based. Builds real things. Great for learning to solder. |
| **Afrotechmods** | Transistors, MOSFETs, basics | Short, clear, visual explanations of fundamental components. |
| **ElectroBOOM** | Physics of electricity (with comedy) | Learns by (intentionally) shocking himself. Entertaining AND educational. |

### Free Online Tools

| Tool | URL | Purpose |
|---|---|---|
| **Falstad Circuit Simulator** | falstad.com/circuit | Build and simulate circuits in your browser. See current flow animated. |
| **EveryCircuit** | everycircuit.com | Beautiful interactive circuit simulator (mobile app too). |
| **KiCad** | kicad.org | Free, professional-grade PCB design software. Open source. |
| **Wokwi** | wokwi.com | Simulate Arduino/ESP32/STM32 circuits in the browser. Run code without hardware! |
| **Digikey / Mouser** | digikey.com / mouser.com | Buy electronic components. Both have excellent parametric search filters. |

---

**You are now equipped with the theoretical foundation to understand every circuit you will ever encounter in embedded systems.** When you're ready, Phase 2 will teach you to put all of this knowledge to work with a real microcontroller.
