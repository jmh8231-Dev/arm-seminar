# ARM Seminar Board (STM32F405VGT6) — ARM Training Kit v2.0

Practical ARM Cortex-M training board + **16(+n) session curriculum** designed for **hands-on embedded systems training**  
(GPIO → UART → ADC/DMA → SDIO/FATFS → CAN → FreeRTOS → (optional) SDRAM/QSPI/LTDC/GUI)

> Repository: https://github.com/jmh8231-Dev/arm-seminar

---

## 1) Overview

This project contains:
- **ARM Seminar Board (Hardware)**: STM32F405VGT6 기반 실습 보드  
  (USB Type-C 전원, UART-USB, SDIO microSD, I2C RTC/EEPROM/Temp, CAN, USB OTG FS 등)
- **Firmware Examples**: 세미나 회차별 예제(권장 구조 제공)
- **Seminar Curriculum**: 총 16 + n 회차로 구성된 실무 중심 커리큘럼 문서

---

## 2) Board Highlights (What you can practice)

### Core MCU
- **Main MCU**: STM32F405VGT6 (LQFP-100, Cortex-M4F)

### PC Connectivity / Debug
- **ST-LINK/V2 다운로드** (권장)
- **UART-to-USB (FT232RL)**: 디버그 출력/CLI/로그 수집용

### Storage
- **microSD (SDIO 4-bit)** + FATFS 실습
- (Optional Track) **External memory**: SDRAM(FMC), QSPI Flash *(보드 리비전/구성에 따라)*

### Communications
- **CAN 2.0B** *(Transceiver: SN65HVD231 계열)*
- **Wi-Fi Module**: ESP-12F(ESP8266) *(UART 기반 연동 / IoT 확장)*

### I2C Peripherals
- **RTC**: DS3231M + **CR2032 백업**
- **EEPROM**: AT24C256
- **Temperature Sensor**: AS6221 *(복수 채널 구성)*

### Analog / GPIO / UI
- Buttons / LEDs / Buzzer
- ADC 실습용 입력(예: CDS Cell / 가변저항 등)
- (옵션) CLCD/디스플레이/LTDC/Touch 등은 커리큘럼 확장 트랙에서 다룸

### Power
- **USB Type-C 5V 입력**
- **3.3V Regulator**: LM1117-3.3 계열
- 테스트 포인트 제공(5V / 3.3V / GND)

---

## 3) Repository Structure (Recommended)

> 레포에 이미 폴더가 있으면 그대로 쓰고, 없으면 아래처럼 정리 추천.

```text
arm-seminar/
├─ hardware/
│  ├─ schematic/                 # PDF / KiCad project
│  ├─ bom/                       # BOM files
│  └─ photos/                    # board images
├─ firmware/
│  ├─ common/                    # 공용 드라이버/유틸(UART printf, ringbuffer, etc.)
│  ├─ session-01_gpio/
│  ├─ session-02_uart/
│  ├─ session-04_adc_dma/
│  ├─ session-08_sdio_fatfs/
│  ├─ session-10_can/
│  └─ session-15_freertos/
└─ docs/
   ├─ curriculum/                # 세미나 계획서/슬라이드
   └─ bringup/                   # 보드 bring-up 체크리스트/FAQ
```

---

## 4) Quick Start

### Requirements
- STM32CubeIDE
- ST-LINK driver (ST-LINK/V2 사용 시)
- (Optional) Serial terminal (TeraTerm / PuTTY)

### Steps
1. **USB Type-C**로 보드 전원 인가
2. **ST-LINK/V2** 연결 후 `firmware/session-xx_*` 예제 프로젝트 열기
3. Build & Download
4. UART-USB(FT232RL) COM 포트로 로그 확인

---

## 5) Seminar Curriculum (16 + n sessions)

> “초급 → 중급 → 고급”으로 난이도 상승, 필요 시 하루 2~3회차 묶어서 진행.

### STM32 Basic Track
- 1-1 개발환경 구축 + H/W 소개
- 1-2 CubeIDE 기본 + GPIO
- 1-3 EXTI 외부 인터럽트
- 2-1 UART Polling (MCU ↔ PC 디버그 채널)
- 2-2 printf 리타겟
- 2-3 UART RX Interrupt
- 3 CLCD 출력
- 4-1 ADC (Polling + DMA)
- 4-2 DAC
- 5 Timer + Interrupt (Delay 구현 포함)
- 6-1 PWM 생성
- 6-2 PWM 기반 모터 구동
- 6-3 실시간 PWM 주기 변경
- 7 USB Mass Storage (외부 대용량 메모리)
- 8 SDIO + FATFS (파일 관리)
- 9-1 SPI (VS1003 기반 실습)
- 9-2 I2C
- 10 CAN 통신 (CAN B)

### RTOS / Advanced Track
- 15-2 FreeRTOS 기초 (Task/Queue/Thread 통신)
- (n) External SDRAM (FMC)
- (n) External QSPI
- (n) LTDC + 외부 프레임버퍼
- (n) GUI + Touch Interface

### IoT Track (Draft / Optional)
- Wi-Fi 모듈 사용법 / GPIO 제어 / Sensor value 전송
- OTA / DFU / 저전력 모드 / 최종 작품 통합

---

## 6) Practice Assignments (Examples)

- UART
  - PC에서 특정 문자를 받으면 LED ON/OFF (Polling / Interrupt 버전)
- Media
  - **VS1003 + SD Card**로 MP3 플레이어 만들기
- RTOS
  - FreeRTOS 기반 병렬 처리로 “보드에 있는 부품들”로 작은 작품 제작
- IoT
  - 펌웨어 업데이트 실패 시 이전 펌웨어로 복구(안전한 업데이트 플로우)

---

## 7) Hardware Docs

- Schematic (PDF): `hardware/schematic/ARM_Training_Kit_v2.0_schematic.pdf`
- Curriculum Plan: `docs/curriculum/ARM_Seminar_Plan_KR.pdf`
- BOM (XLSX): `hardware/bom/ARM_Seminar_Board_BOM_20250925.xlsx`

---

## 8) Photos / Block Diagram

> `hardware/photos/`에 아래 파일명을 권장합니다. (README 이미지 링크/문서 통일용)

- `hardware/photos/board_top_labeled.png`
- `hardware/photos/board_front_labeled.png`
- `hardware/photos/block_diagram.png`

---

## 9) License (Proprietary)

**Copyright (c) 2025–present, MyungHoon Jung. All rights reserved.**

This repository (including hardware design files, schematics, BOM, documents, and firmware) is **proprietary**.  
You may **not** copy, modify, publish, distribute, sublicense, or use this work (in whole or in part) without **explicit written permission** from the author.

If you need permission for internal training / collaboration, please contact the maintainer.

---

## 10) Contact
- Maintainer: MyungHoon Jung (정명훈)
- GitHub: https://github.com/jmh8231-Dev

_Last updated: 2026-01-09 (KST)_
