drk7: Pin Control Attack implementation with LKM.
-------------------------------------------------------

*Simplified version used for performance tests.*

Method: GPIO base remap, Pin Multiplexing.
Effect: PWM disabling.

Target system: RaspberryPi with Adafruit PWM controller connected via I2C interface.
The I2C data goes through GPIO pin 2 multiplexed as alternate function 0 (ALT0),
which corresponds to I2C master 1 data line (SDA1).
A servo motor is attached to PWM, and the PLC logic uses a sinus wave to make it
rotating from -45° to +45°.

The attack changes the pin 2 mode into GPIO, so that the PLC logic cannot write to it
anymore through the I2C controller, and the servo motor stops moving.
The Codesys Runtime is not able to detect it.