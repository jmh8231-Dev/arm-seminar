
#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

/* ===== 앱 버전/표시 ===== */
#define APP_FW_VERSION     "v1.6"
#define APP_LCD_COLS       20
#define APP_LCD_ROWS       4

/* ===== 상태 정의 ===== */
typedef enum {
  ST_HOME = 0,
  ST_ADC,
  ST_MP3,
  ST_CTRL,
  ST_SYS_MENU,
  ST_SYS_SET_TIME,
  ST_SYS_INFO
} AppState;

/* ===== 이벤트(짧게/길게) ===== */
typedef enum {
  EV_NONE = 0,
  EV_SW1, EV_SW2, EV_SW3, EV_SW4,
  EV_SW1L, EV_SW2L, EV_SW3L, EV_SW4L,
  EV_TICK_50MS, EV_TICK_200MS, EV_TICK_1S
} AppEvent;

/* ===== 센서 캐시 ===== */
typedef struct {
  float  t1_c, t2_c;     // AS6221
  uint16_t cds_raw;      // ADC2
  uint16_t a1_raw, a2_raw, a3_raw; // ADC1 DMA 3ch
} Sensors;

/* ===== 설정(EEPROM에 저장 예정: 현재 RAM 디폴트) ===== */
typedef struct {
  uint16_t magic;          // 0xA55A
  uint8_t  csv_period_s;   // 1/5 등
  uint16_t servo_min_us;   // 기본 900
  uint16_t servo_max_us;   // 기본 2100
  uint8_t  mp3_volume;     // 0..31
  uint8_t  can_enable;     // 0/1 (옵션)
  uint16_t can_id;         // 0x0100 (옵션)
  uint16_t last_track_idx; // 마지막 재생 곡
  uint8_t  option_bits;    // 비프/LED 등
  uint32_t crc32;
} AppConfig;

/* ===== 공개 API ===== */
void App_Init(void);
void App_Task(void);

#endif /* APP_H */
