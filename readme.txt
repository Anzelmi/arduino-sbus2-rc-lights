Arduino S.BUS2 RC Lighting Controller

An Arduino Nano-based lighting controller for 1:10 scale RC cars. It decodes live Futaba S.BUS2 receiver data via hardware inversion to control addressable NeoPixel (RGBW) LED effects, backfire flames, and dynamic CH2-driven brake lights.

Features
- Futaba S.BUS2 Integration: Direct decoding of receiver channel data without extra PWM wiring.
- Auto-Calibrating Brake Lights: Continuously monitors CH2 (Throttle/Brake) with automatic neutral-position calibration at startup.
- Multi-Mode Lighting FX (CH6 / CH7 Toggle):
	Mode 0 (Off): All secondary effects disabled; brake lights remain functional.
	Mode 1 (Street/Underglow): Smooth animated underglow pulse effect + randomized exhaust flame backfire.
	Mode 2 (Police): Alternating red/blue strobe sequence + randomized exhaust flame backfire.
- Failsafe System: Automatically douses all LEDs if the S.BUS2 signal drops for more than 1000 ms.

Hardware Requirements
	Microcontroller: Arduino Nano V3.0 (ATmega328P)
	Receiver: Futaba S.BUS2 compatible receiver (e.g., R334SBS, R304SB)
	LEDs: 30x Addressable RGBW NeoPixel / SK6812 LED strip
	Signal Inverter: NPN transistor (e.g., KSP2222A or BC547) + 10kΩ resistor + 4.3V/4.7V Zener diode circuit

Pin Mapping
Arduino Pin 	Connection
D0 (RX)			Inverted S.BUS2 Signal Input
D2 (VCC Out)	5V Power Rail for Hardware Inverter Circuit
D6 				NeoPixel Data Input
A0				Unconnected (Analog Random Seed Generator)
GND / 5V 		Common Ground & Power Lines

Installation & Usage
1. Upload Code: Disconnect the D0 (RX) wire during USB flashing to avoid serial upload conflicts. Reconnect D0 after uploading.
2. Boot Calibration: Keep the throttle trigger in the neutral position when powering on the vehicle to allow automatic CH2 brake calibration.
3. Channel Inversion: If brake lights trigger during forward throttle, change BRAKE_DIRECTION from 1 to -1 in the code.
