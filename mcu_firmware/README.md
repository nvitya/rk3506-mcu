# Test Firmware for the Cortex-M0 Core

I include some test firmwares made by my VIHAL library here.
(Currently only for Luckfox Lyra Plus)

blinkled.elf:
  just blinks the onboard led. At the start it blinks faster, than with 0.5 s.

uart.elf:
  Does what the blinkled, plus writes to the uart at 1.C2 + 1.C3
  VIHAL pin initialization code for the UART:
```
  // using the same pins as the MCU Example from Rockchip
  hwpinctrl.PinSetupRmio(1, PINNUM_C2, RMIO_UART4_TX, 0);
  hwpinctrl.PinSetupRmio(1, PINNUM_C3, RMIO_UART4_RX, PINCFG_PULLUP);
  conuart.baudrate = 1500000; // the highest baudrate from the 24 MHz Oscillator
  conuart.Init(4);
```

