# ARM Seminar Board (Training Kit Rev 2.0)

> **Platform:** STM32F405VGT6 (Cortex‑M4)  
> **Goal:** 실무형 ARM(임베디드) 교육을 위한 “한 보드로 끝내는” 주변장치 통합 실습 키트

이 리포지토리는 **ARM Seminar Board**(Training Kit Rev 2.0)의 하드웨어 설계 자료와 교육/실습용 참고 문서를 정리한 저장소입니다.  
GPIO부터 SDIO, 외부 SDRAM(FMC), QSPI, CAN, USB, Wi‑Fi(IoT), RTC 등 **현업에서 자주 쓰는 주변장치를 한 번에 실습**할 수 있도록 구성했습니다.

---

## 1. What’s on the board

아래 구성은 **블록 다이어그램/보드 실물 라벨** 기준으로 정리했습니다.

### Core
- **Main MCU:** STM32F405VGT6
- **External SDRAM:** FMC 인터페이스
- **microSD:** SDIO (4‑bit)
- **QSPI Flash:** 외부 대용량 메모리 실습용
- **USB:**
  - USB Type‑C (전원/PC 연결)
  - USB OTG FS (USB 2.0)
  - UART‑to‑USB (FT232RL) 디버그/통신

### Display / UI
- **2004 CLCD**
- **Touch LCD** (LTDC, 24‑bit 병렬)

### Connectivity
- **Wi‑Fi Module:** ESP‑12F (UART 기반)
- **CAN:** CAN Tx/Rx + CAN 커넥터
- **GPS Module** (보드 옵션/확장)

### Sensors / Storage / Misc
- **RTC:** DS3231 + CR2032 배터리
- **EEPROM**
- **Temp Sensor:** AS6221
- **Analog inputs:** CDS Cell, 가변저항(ADC 실습)
- **User I/O:** LEDs, Buttons, GPIO/EXTI 스위치
- **Actuator:** SG90 Servo (PWM), Passive Buzzer

---

## 2. Images (add these to your repo)

이 README는 아래 폴더 구조를 기준으로 이미지를 참조합니다.

```
arm-seminar/
└─ docs/
   └─ images/
      ├─ block_diagram.png
      ├─ board_front.png
      └─ board_back.png
```

### 2.1 Block Diagram
![Block Diagram](docs/images/block_diagram.png)

### 2.2 Board Photos
![Board Front](docs/images/board_front.png)
![Board Back](docs/images/board_back.png)

> ✅ **이미지 추가 방법**
1) 위 파일명으로 이미지를 `docs/images/`에 복사  
2) Git에 추가  
```bash
git add docs/images/block_diagram.png docs/images/board_front.png docs/images/board_back.png
git commit -m "docs: add ARM Seminar Board images"
git push
```

---

## 3. Getting Started (Bring‑up)

### 3.1 Power
- **USB Type‑C**로 5V 전원 공급 (PC/어댑터 모두 가능)
- RTC는 **CR2032**를 장착하면 전원 분리 후에도 시간 유지

### 3.2 Flash / Debug
- **ST‑Link/V2 헤더**로 다운로드/디버깅
- 또는 **UART‑to‑USB(FT232RL)**를 사용한 시리얼 콘솔/로그

### 3.3 Recommended dev environment
- STM32CubeIDE (기본)
- (선택) STM32CubeProgrammer, Logic Analyzer / CAN‑USB / USB‑TTL

---

## 4. Peripheral Quick Map (실습용 체크리스트)

교육 진행 시 “오늘 실습 대상”을 빠르게 체크할 수 있도록 묶었습니다.

- **GPIO/EXTI:** 버튼/스위치, LED
- **ADC/DMA:** CDS Cell, 가변저항
- **Timer/PWM:** Servo(SG90), Buzzer
- **UART:** PC 디버그 콘솔, Wi‑Fi(ESP‑12F)
- **I2C:** EEPROM, AS6221, DS3231(RTC)
- **SPI:** VS1003 등 확장
- **SDIO/FATFS:** microSD 파일시스템
- **FMC SDRAM:** 외부 메모리 R/W
- **QSPI:** 외부 Flash R/W
- **CAN:** CAN B (보드 커넥터)
- **LTDC/Touch LCD:** 프레임버퍼/GUI(확장)

---

## 5. Seminar Curriculum (요약)

세미나는 총 **16 + n회차**로 기획되었고, 초급 → 고급으로 난이도가 점진적으로 상승하도록 설계했습니다.  
핵심 커리큘럼 예시는 아래와 같습니다.

- CubeIDE / GPIO / EXTI
- UART(폴링/인터럽트), printf 디버그
- CLCD 출력, ADC/DMA, Timer
- PWM/모터 구동, USB Mass Storage
- SDIO + FATFS, SPI, I2C, CAN
- (고급) FreeRTOS, 외부 SDRAM(FMC), QSPI, LTDC/GUI

---

## 6. Revision Notes

Rev 1.0 대비 주요 수정 요구사항(요약)
- RTC 배터리 홀더 위치 변경(뒷면)
- USB Type‑C 라인 다이오드 삭제
- UART 기반 USB 업로드 가능하도록 개선
- (부가) 실크스크린 영역/사이즈 조정, 날짜/버전 업데이트

---

## 7. Files

- `docs/` : 회로도(PDF), BOM, 세미나 계획서, 수정사항 문서 등
- (권장) `firmware/` : 예제 코드(회차별), 드라이버, 템플릿
- (권장) `hardware/` : KiCad 원본, Gerber, 제조 파일

---

## 8. License / Usage

**무단 도용 금지 (All Rights Reserved).**  
본 저장소의 회로/PCB/문서/이미지/콘텐츠는 저작권자의 사전 서면 허가 없이 **복제, 배포, 상업적 이용, 2차 저작물 제작**을 금지합니다.  
교육/연구 목적의 검토가 필요하다면, 반드시 저작권자에게 사용 허가를 요청해 주세요. (자세한 내용은 `LICENSE` 참조)

© 2025 MyungHoon Jung. All rights reserved.
