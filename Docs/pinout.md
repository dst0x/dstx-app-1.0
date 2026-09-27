# DST0X Board Pinout — STM32F412RETx

## Clock
| Signal | Pin  | Notes            |
|--------|------|------------------|
| HSE IN | PH0  | 8 MHz crystal    |
| HSE OUT| PH1  |                  |

## USART2 — Debug / VCP (115200 baud)
| Signal | Pin | AF  |
|--------|-----|-----|
| TX     | PA2 | AF7 |
| RX     | PA3 | AF7 |

## USART1 — RS-485 (9600 baud)
| Signal | Pin  | AF  |
|--------|------|-----|
| TX     | PA9  | AF7 |
| RX     | PA10 | AF7 |

## SDIO — Micro SD (4-bit)
| Signal | Pin  | Pull-up |
|--------|------|---------|
| CK     | PC12 | No      |
| CMD    | PD2  | Yes     |
| D0     | PC8  | Yes     |
| D1     | PC9  | Yes     |
| D2     | PC10 | Yes     |
| D3     | PC11 | Yes     |

## GPIO
| Function | Pin | Mode          |
|----------|-----|---------------|
| LED      | PB2 | Output PP     |
