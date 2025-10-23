/*
 * app.c - ARM 개발보드 데모 앱 (홈/ADC/MP3/CTRL/SYSTEM)
 * 변경점 v1.3 (Sep 2025)
 *  - MP3 끊김 현상 개선:
 *    ① MP3 피더: DREQ 허용 시 32B 연속 전송 유지 + 2-버퍼 구조(2×4096B)
 *       └ 피더는 항상 "먼저 급식 → 그 다음에 작은 블록(512~1024B) 읽기" 순서로 동작
 *    ② CSV 로깅: 파일 상시 오픈 + 5s마다 f_sync() (open/close 스톨 제거)
 *    ③ 센서 주기: AS6221 반복 Init 제거(200ms마다 Read만)
 *  - 볼륨 맵핑 유지(0..31 → 0..-124dB, 0이 최대) — v1.2와 동일
 *  - LCD 드라이버의 문자당 1ms 지연 제거는 CLCD.c에서 이미 수정했다고 가정(delay_us≈50us)
 *
 * 사용 라이브러리:
 *  - CLCD.{c,h}   (lcd_Init, lcd_setCurStr 등)
 *  - AS6221.{c,h} (I2C1: 2개 센서)
 *  - VS1003.{c,h} (SPI1, SDI/SCI, DREQ=MP3_DREQ, VS1003_WriteData(32B) 가정)
 *  - DS3231.{c,h} (I2C2)
 */

#include "app.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "i2c.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "fatfs.h"
#include "gpio.h"

#include "CLCD.h"
#include "AS6221.h"
#include "VS1003.h"
#include "DS3231.h"

/* ===================== [CONFIG] 하드웨어 매핑 ===================== */
/* 버튼: Active Low 가정(필요시 APP_BTN_PRESSED 수정) */
#define APP_BTN_PRESSED(x)     ((x) == GPIO_PIN_RESET)
#define BTN1_READ()            HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin)
#define BTN2_READ()            HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin)
#define BTN3_READ()            HAL_GPIO_ReadPin(SW3_GPIO_Port, SW3_Pin)
#define BTN4_READ()            HAL_GPIO_ReadPin(SW4_GPIO_Port, SW4_Pin)

/* LED */
#define LED1_ON()              HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET)
#define LED1_OFF()             HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET)
#define LED2_ON()              HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET)
#define LED2_OFF()             HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET)

/* ===== BEEP: TIM1 CH1 사용 (2 kHz 권장) ===== */
extern TIM_HandleTypeDef htim1;
static uint32_t s_beep_deadline = 0;
static inline void BEEP_ON(void){
  uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (arr+1)/2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}
static inline void BEEP_OFF(void){
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
}
static inline void beep_play(uint16_t ms){
  BEEP_ON();
  s_beep_deadline = HAL_GetTick() + ms;
}

/* ===== 서보 PWM: 사용 타이머/채널에 맞게 수정 ===== */
extern TIM_HandleTypeDef htimX;         // ⚠️ 실제 사용 타이머로 교체 필요 시
#define SERVO_SET_US(us)       __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, (us))

/* ADC1 DMA 버퍼(3ch) - 프로젝트 심볼명에 맞게 extern */
extern volatile uint16_t adc1_dma_buf[3];
#define ADC1_CH1_RAW()         (adc1_dma_buf[0])
#define ADC1_CH2_RAW()         (adc1_dma_buf[1])
#define ADC1_CH3_RAW()         (adc1_dma_buf[2])

/* CDS: ADC2 단발 변환 */
extern ADC_HandleTypeDef hadc2;
static inline uint16_t ADC2_ReadOnce(void){
  HAL_ADC_Start(&hadc2);
  HAL_ADC_PollForConversion(&hadc2, 10);
  uint16_t v = HAL_ADC_GetValue(&hadc2);
  HAL_ADC_Stop(&hadc2);
  return v;
}

/* I2C 핸들: AS6221=I2C1, DS3231=I2C2 */
extern I2C_HandleTypeDef hi2c1;

/* FatFs */
extern FATFS SDFatFS;
extern char  SDPath[4];

/* ===================== 상수/전역 ===================== */
#define BTN_LONG_MS         1200
#define TICK_50_MS          50
#define TICK_200_MS         200
#define TICK_1S_MS          1000

#define AS6221_ADDR1        (0x48 << 1)
#define AS6221_ADDR2        (0x49 << 1)

#define MUSIC_DIR           "MUSIC"      // /MUSIC
#define LOG_DIR             "LOG"

/* ---- MP3 I/O 버퍼(더블버퍼) ---- */
#define MP3_BUF_SZ          4096
static uint8_t mp3_buf[2][MP3_BUF_SZ];
static uint16_t mp3_rd_i=0, mp3_wr_i=1;   // 읽기/쓰기 버퍼 인덱스
static uint32_t mp3_rd_off=0, mp3_rd_len=0; // 현재 급식 버퍼 소비 위치/길이
static uint32_t mp3_wr_len=0;               // 백그라운드 리필 결과 길이(0이면 비어있음)

typedef struct {
  AppState  st;
  AppConfig cfg;
  Sensors   s;

  /* 버튼 디바운스/롱프레스 */
  uint32_t  t_last_50, t_last_200, t_last_1s;
  uint32_t  t_down[4];
  uint8_t   prev[4];

  /* 화면/플래그 */
  uint8_t   adc_fmt;         // 0=Raw,1=Volt,2=Percent
  uint8_t   csv_on;          // CSV 로깅 플래그
  uint8_t   auto_servo;      // 컨트롤 화면 AUTO

  /* MP3/재생목록 */
  uint16_t  track_count;
  uint16_t  cur_track;
  FIL       fmp3;
  uint8_t   mp3_opened;
  uint8_t   mp3_playing;

  /* CSV 지속 오픈 핸들 */
  FIL       flog;
  uint8_t   log_open;
  uint8_t   log_day;         // 일(day) 캐시
  uint32_t  last_sync_ms;
} AppCtx;

static AppCtx app;

/* AS6221 객체 */
static AS6221_t as1 = {.address=AS6221_ADDR1, .CR=ConvPer1000ms, .CF=0, .SM=0, .IM=0, .POL=0, .SS=0};
static AS6221_t as2 = {.address=AS6221_ADDR2, .CR=ConvPer1000ms, .CF=0, .SM=0, .IM=0, .POL=0, .SS=0};

/* ===== 20x4 라인 캐시로 깜빡임 최소화 ===== */
static char s_line[4][21];  // 마지막으로 그린 4개 줄(널포함 21)
static void lcd_line_set(uint8_t row, const char *txt) {
  char buf[21]; memset(buf, ' ', 20); buf[20] = 0;
  size_t L = strlen(txt); if (L > 20) L = 20;
  memcpy(buf, txt, L);
  if (memcmp(s_line[row], buf, 21) != 0) {
    lcd_setCurStr(0, row, (char*)buf);   // 해당 줄만 갱신
    memcpy(s_line[row], buf, 21);
  }
}
static void lcd_all_clear_cache(void){ for (int r=0; r<4; ++r){ memset(s_line[r], 0xFF, 21); } }



/* ===================== RTC 편집기/도우미 ===================== */
typedef struct {
  int year; uint8_t mon, day, hour, min, sec;
  uint8_t field; // 0:Y 1:M 2:D 3:h 4:m 5:s
} RtcEdit;
static RtcEdit re;

static inline uint8_t clamp_u8(int v, int lo, int hi){ if(v<lo) v=lo; if(v>hi) v=hi; return (uint8_t)v; }
static inline int is_leap(int y){ return ((y%4==0) && (y%100!=0)) || (y%400==0); }
static inline uint8_t days_in_month(int y, int m){
  static const uint8_t d[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  return (m==2)? (d[1] + (is_leap(y)?1:0)) : d[m-1];
}
// 1=Sun..7=Sat (DS3231 convention)
static inline uint8_t day_of_week(int y,int m,int d){
  static int t[]={0,3,2,5,0,3,5,1,4,6,2,4};
  if(m<3) y-=1;
  int w=(y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
  return (w==0)?7:w;
}
static void rtc_load_from_ds3231(void){
  re.year = (int)DS3231_GetYear();
  re.mon  = DS3231_GetMonth();
  re.day  = DS3231_GetDate();
  re.hour = DS3231_GetHour();
  re.min  = DS3231_GetMinute();
  re.sec  = DS3231_GetSecond();
  re.field=0;
}
static void rtc_apply_to_ds3231(void){
  uint8_t dow = day_of_week(re.year, re.mon, re.day);
  DS3231_SetFullDate(re.day, re.mon, dow, (uint16_t)re.year);
  DS3231_SetFullTime(re.hour, re.min, re.sec);
  // Clear OSF on save
  uint8_t s = DS3231_GetRegByte(DS3231_REG_STATUS);
  DS3231_SetRegByte(DS3231_REG_STATUS, s & ~(1U<<DS3231_OSF));
}
static void rtc_incdec(int dir){ // dir=+1/-1
  switch(re.field){
    case 0: re.year = clamp_u8(re.year + dir, 2000, 2099); break;
    case 1: re.mon  = clamp_u8((int)re.mon  + dir, 1, 12); break;
    case 2:{
      uint8_t md = days_in_month(re.year, re.mon);
      int nd = (int)re.day + dir;
      if(nd<1) nd = md;
      if(nd>md) nd = 1;
      re.day = (uint8_t)nd; break;
    }
    case 3: re.hour = clamp_u8((int)re.hour + dir, 0, 23); break;
    case 4: re.min  = clamp_u8((int)re.min  + dir, 0, 59); break;
    case 5: re.sec  = clamp_u8((int)re.sec  + dir, 0, 59); break;
  }
}
static void rtc_next_field(void){ re.field = (re.field+1)%6; }

/* ===================== 부팅 셀프테스트 로거 ===================== */
static char boot_l1[21]={0}, boot_l2[21]={0}, boot_l3[21]={0};

static void boot_log_clear(void){
  lcd_all_clear_cache();
  memset(boot_l1,0,21); memset(boot_l2,0,21); memset(boot_l3,0,21);
  lcd_line_set(0, "Self-Test v1.5");
  lcd_line_set(1, "--------------------");
  lcd_line_set(2, "                    ");
  lcd_line_set(3, "                    ");
}
static void boot_log_line(const char* label, const char* status){
  // Compose "LABEL....STATUS" to fit 20 cols
  char ln[21]; memset(ln, 0, sizeof ln);
  size_t L1 = strlen(label), L2 = strlen(status);
  if(L1 > 20) L1 = 20;
  if(L2 > 20) L2 = 20;
  size_t dots = (20 >= L1 + L2) ? (20 - L1 - L2) : 1;
  if(dots > 16) dots = 16;
  char buf[24]; memset(buf, 0, sizeof buf);
  memcpy(buf, label, L1);
  memset(buf+L1, '.', dots);
  memcpy(buf+L1+dots, status, L2);
  // scroll: boot_l1<-boot_l2<-boot_l3<-new
  memcpy(boot_l1, boot_l2, 21);
  memcpy(boot_l2, boot_l3, 21);
  snprintf(boot_l3, sizeof boot_l3, "%-20.20s", buf);
  lcd_line_set(1, boot_l1[0]?boot_l1:"                    ");
  lcd_line_set(2, boot_l2[0]?boot_l2:"                    ");
  lcd_line_set(3, boot_l3);
  HAL_Delay(500);
}
/* ===================== 유틸 ===================== */
static inline uint32_t ms_now(void){ return HAL_GetTick(); }
static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len){
  crc = ~crc;
  for(size_t i=0;i<len;i++){
    crc ^= data[i];
    for(int b=0;b<8;b++){
      uint32_t mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
    }
  }
  return ~crc;
}

/* ===================== 설정(RAM 기본) ===================== */
static void cfg_default(AppConfig *c){
  memset(c, 0, sizeof(*c));
  c->magic         = 0xA55A;
  c->csv_period_s  = 1;
  c->servo_min_us  = 900;
  c->servo_max_us  = 2100;
  c->mp3_volume    = 25;
  c->can_enable    = 0;
  c->can_id        = 0x0100;
  c->last_track_idx= 0;
  c->option_bits   = 1;   // beep on 등
  c->crc32 = crc32_update(0, (uint8_t*)c, sizeof(*c)-4);
}
static void cfg_load(AppConfig *c){ cfg_default(c); }   // TODO: EEPROM 연동
static void cfg_save(const AppConfig *c){ (void)c; }    // TODO: EEPROM 연동

/* ===================== LCD 화면들 ===================== */
static void lcd_home_draw(void){
  char ln[24];
  uint32_t mhz = HAL_RCC_GetSysClockFreq()/1000000UL;

  snprintf(ln, sizeof ln, "Demo FW %s %3luMHz", APP_FW_VERSION, (unsigned long)mhz);
  lcd_line_set(0, ln);

  snprintf(ln, sizeof ln, "%04u-%02u-%02u %02u:%02u:%02u",
           (unsigned)DS3231_GetYear(), (unsigned)DS3231_GetMonth(), (unsigned)DS3231_GetDate(),
           (unsigned)DS3231_GetHour(), (unsigned)DS3231_GetMinute(), (unsigned)DS3231_GetSecond());
  lcd_line_set(1, ln);

  snprintf(ln, sizeof ln, "T1 %4.1fC  T2 %4.1fC", app.s.t1_c, app.s.t2_c);
  lcd_line_set(2, ln);

  lcd_line_set(3, "1ADC 2MP3 3CTL 4SYS");
}

static void lcd_adc_draw(void){
  char ln[24];
  snprintf(ln, sizeof ln, "ADC VIEW   CSV:%s", app.csv_on?"ON ":"OFF");
  lcd_line_set(0, ln);

  uint16_t r1=app.s.a1_raw, r2=app.s.a2_raw, r3=app.s.a3_raw;
  if(app.adc_fmt==0){
    snprintf(ln, sizeof ln, "A1 %4u", r1); lcd_line_set(1, ln);
    snprintf(ln, sizeof ln, "A2 %4u", r2); lcd_line_set(2, ln);
    snprintf(ln, sizeof ln, "A3 %4u", r3); lcd_line_set(3, ln);
  }else if(app.adc_fmt==1){
    float v1=3.3f*r1/4095.f, v2=3.3f*r2/4095.f, v3=3.3f*r3/4095.f;
    snprintf(ln, sizeof ln, "A1 %1.3fV", v1); lcd_line_set(1, ln);
    snprintf(ln, sizeof ln, "A2 %1.3fV", v2); lcd_line_set(2, ln);
    snprintf(ln, sizeof ln, "A3 %1.3fV", v3); lcd_line_set(3, ln);
  }else{
    uint8_t p1=(uint8_t)(100*r1/4095), p2=(uint8_t)(100*r2/4095), p3=(uint8_t)(100*r3/4095);
    snprintf(ln, sizeof ln, "A1 %3u%%", p1); lcd_line_set(1, ln);
    snprintf(ln, sizeof ln, "A2 %3u%%", p2); lcd_line_set(2, ln);
    snprintf(ln, sizeof ln, "A3 %3u%%", p3); lcd_line_set(3, ln);
  }
}

static void lcd_mp3_draw(const char *title){
  char ln[24];
  lcd_line_set(0, "MP3: /MUSIC");
  snprintf(ln, sizeof ln, "%s", title?title:"(No MP3)");
  lcd_line_set(1, ln);
  snprintf(ln, sizeof ln, "VOL:%02u %s", app.cfg.mp3_volume, app.mp3_playing?"PLAY":"PAUSE");
  lcd_line_set(2, ln);
  lcd_line_set(3, "1Prev 2Play 3Next 4Home");
}

static void lcd_ctrl_draw(void){
  char ln[24];
  snprintf(ln, sizeof ln, "SERVO %u-%u", app.cfg.servo_min_us, app.cfg.servo_max_us);
  lcd_line_set(0, ln);
  uint16_t a1 = app.s.a1_raw;
  uint16_t pwm = app.cfg.servo_min_us + (uint32_t)(app.cfg.servo_max_us - app.cfg.servo_min_us) * a1 / 4095u;
  snprintf(ln, sizeof ln, "ADC1:%4u -> %4uus", a1, pwm);
  lcd_line_set(1, ln);
  snprintf(ln, sizeof ln, "CDS:%4u  AUTO:%s", app.s.cds_raw, app.auto_servo?"ON ":"OFF");
  lcd_line_set(2, ln);
  lcd_line_set(3, "1-MIN 2AUTO 3+MAX 4BACK");
}

static void lcd_sysmenu_draw(uint8_t idx){
  lcd_line_set(0, "SYSTEM");
  lcd_line_set(1, (idx==0)?"> Date/Time":"  Date/Time");
  lcd_line_set(2, (idx==1)?"> Info/Diag":"  Info/Diag");
  lcd_line_set(3, "1^ 2OK 3v 4BACK");
}

static void lcd_sysinfo_draw(void){
  char ln[24];
  lcd_line_set(0, "INFO");
  snprintf(ln, sizeof ln, "T1 %.1f T2 %.1f", app.s.t1_c, app.s.t2_c); lcd_line_set(1, ln);
  snprintf(ln, sizeof ln, "ADC:%u,%u,%u", app.s.a1_raw, app.s.a2_raw, app.s.a3_raw); lcd_line_set(2, ln);
  lcd_line_set(3, "4BACK");
}

/* ===== 시간 설정 임시 화면 ===== */

static void lcd_datetime_draw(void){
  char ln[24];
  lcd_line_set(0, "SET DATE/TIME");
  snprintf(ln, sizeof ln, "Y:%4d M:%02u D:%02u", re.year, re.mon, re.day);
  if(re.field==0) ln[0]='>'; else if(re.field==1) ln[7]='>'; else if(re.field==2) ln[14]='>';
  lcd_line_set(1, ln);
  snprintf(ln, sizeof ln, "H:%02u  M:%02u  S:%02u", re.hour, re.min, re.sec);
  if(re.field==3) ln[0]='>'; else if(re.field==4) ln[7]='>'; else if(re.field==5) ln[14]='>';
  lcd_line_set(2, ln);
  lcd_line_set(3, "1- 2NEXT 3+ SAVE:2L");
}

/* ===================== 버튼 스캔 ===================== */
static void post_event(AppEvent ev);
static void scan_buttons_50ms(void){
  const GPIO_PinState rd[4] = { BTN1_READ(), BTN2_READ(), BTN3_READ(), BTN4_READ() };
  uint32_t now = ms_now();
  for(int i=0;i<4;i++){
    uint8_t cur = APP_BTN_PRESSED(rd[i]) ? 1 : 0;
    if(cur && !app.prev[i]){ app.t_down[i] = now; }
    if(!cur && app.prev[i]){
      uint32_t dur = now - app.t_down[i];
      if(dur >= BTN_LONG_MS) post_event((AppEvent)(EV_SW1L+i));
      else                   post_event((AppEvent)(EV_SW1+i));
    }
    app.prev[i] = cur;
  }
}

/* ===================== 센서 갱신 ===================== */
static AS6221_Error_t as_err1 = AS6221_ERROR_NONE, as_err2 = AS6221_ERROR_NONE;
static void sensors_update_200ms(void){
  /* 🔧 v1.3: 반복 Init 제거 → I2C 부하/지연 감소 */
  if(as_err1==AS6221_ERROR_NONE) AS6221_ReadTemperature(&as1);
  if(as_err2==AS6221_ERROR_NONE) AS6221_ReadTemperature(&as2);
  app.s.t1_c = as1.Temp; app.s.t2_c = as2.Temp;
  app.s.a1_raw = ADC1_CH1_RAW();
  app.s.a2_raw = ADC1_CH2_RAW();
  app.s.a3_raw = ADC1_CH3_RAW();
  app.s.cds_raw= ADC2_ReadOnce();
}

/* ===================== CSV 로깅 ===================== */
static void log_open_if_needed(void){
  uint8_t day = DS3231_GetDate();
  if(!app.log_open || day != app.log_day){
    if(app.log_open){ f_sync(&app.flog); f_close(&app.flog); app.log_open=0; }
    char path[64];
    snprintf(path, sizeof path, "%s/LOG_%04u%02u%02u.CSV", LOG_DIR,
             (unsigned)DS3231_GetYear(), (unsigned)DS3231_GetMonth(), (unsigned)day);
    f_mkdir(LOG_DIR);
    if(f_open(&app.flog, path, FA_OPEN_APPEND | FA_WRITE)==FR_OK){
      if(f_size(&app.flog)==0){ UINT w; const char *hdr="time,temp1_c,temp2_c,cds,adc1,adc2,adc3\r\n"; f_write(&app.flog, hdr, strlen(hdr), &w); }
      app.log_open=1; app.log_day=day; app.last_sync_ms = HAL_GetTick();
    }
  }
}
static void csv_log_1s(void){
  if(!app.csv_on) return;
  log_open_if_needed(); if(!app.log_open) return;
  char line[96]; UINT w;
  snprintf(line, sizeof line, "%02u:%02u:%02u,%.2f,%.2f,%u,%u,%u,%u\r\n",
           (unsigned)DS3231_GetHour(), (unsigned)DS3231_GetMinute(), (unsigned)DS3231_GetSecond(),
           app.s.t1_c, app.s.t2_c, app.s.cds_raw, app.s.a1_raw, app.s.a2_raw, app.s.a3_raw);
  f_write(&app.flog, line, strlen(line), &w);
  uint32_t now = HAL_GetTick();
  if(now - app.last_sync_ms >= 5000){ f_sync(&app.flog); app.last_sync_ms = now; }
}

/* ===================== MP3 / 재생목록 ===================== */
#define MAX_TRACKS   128
#define MAX_NAME     64
static char g_tracks[MAX_TRACKS][MAX_NAME];

static int is_mp3_name(const char *n){
  size_t L = strlen(n); if(L<4) return 0;
  const char *e = n + (L-4);
  char c1 = (e[0] | 0x20), c2 = (e[1] | 0x20), c3 = (e[2] | 0x20), c4 = (e[3] | 0x20);
  return (c1=='.' && c2=='m' && c3=='p' && c4=='3');
}

static void scan_music_dir(void){
  app.track_count = 0;
  DIR dir; FILINFO fno;
  if(f_opendir(&dir, MUSIC_DIR)==FR_OK){
    for(;;){ if(f_readdir(&dir, &fno)!=FR_OK || fno.fname[0]==0) break; if(fno.fattrib & AM_DIR) continue; if(is_mp3_name(fno.fname)){ strncpy(g_tracks[app.track_count], fno.fname, MAX_NAME-1); g_tracks[app.track_count][MAX_NAME-1]=0; app.track_count++; if(app.track_count>=MAX_TRACKS) break; } }
    f_closedir(&dir);
  }
  if(app.track_count==0){
    if(f_opendir(&dir, "/")==FR_OK){
      for(;;){ if(f_readdir(&dir, &fno)!=FR_OK || fno.fname[0]==0) break; if(fno.fattrib & AM_DIR) continue; if(is_mp3_name(fno.fname)){ strncpy(g_tracks[app.track_count], fno.fname, MAX_NAME-1); g_tracks[app.track_count][MAX_NAME-1]=0; app.track_count++; if(app.track_count>=MAX_TRACKS) break; } }
      f_closedir(&dir);
    }
  }
  if(app.cur_track >= app.track_count) app.cur_track = 0;
}

static const char* cur_title(void){ return (app.track_count==0)?"(No MP3)":g_tracks[app.cur_track]; }

/* 0..31 → (31-v)*8 LSB (0.5dB/LSB). v=31:0dB, v=0:큰 감쇄 */
static void vs1003_set_volume(uint8_t v){
  if(v>31) v=31; app.cfg.mp3_volume = v;
  uint16_t att = (uint16_t)((31 - v) * 8); if(att > 0xFE) att = 0xFE;
  VS1003_WriteReg(SPI_VOL, (att<<8) | att);
}

static void mp3_stop(void){
  if(app.mp3_opened){ f_close(&app.fmp3); app.mp3_opened = 0; }
  app.mp3_playing = 0;
  mp3_rd_i=0; mp3_wr_i=1; mp3_rd_off=0; mp3_rd_len=0; mp3_wr_len=0;
}

static uint32_t mp3_refill_buf(uint8_t idx, uint32_t want){
  UINT br=0; if(want>MP3_BUF_SZ) want=MP3_BUF_SZ;
  FRESULT fr=f_read(&app.fmp3, mp3_buf[idx], want, &br);
  return (fr==FR_OK)? (uint32_t)br : 0;
}

static void mp3_open_and_start(uint16_t idx){
  if(app.track_count==0) { mp3_stop(); return; }
  mp3_stop(); app.cur_track = idx;
  char path[96]; snprintf(path, sizeof path, "%s/%s", MUSIC_DIR, g_tracks[idx]);
  if(f_open(&app.fmp3, path, FA_READ)==FR_OK){
    app.mp3_opened = 1; app.mp3_playing = 1;
    VS1003_SoftReset();
    vs1003_set_volume(app.cfg.mp3_volume);
    // 더블버퍼 프리필
    mp3_rd_i=0; mp3_wr_i=1; mp3_rd_off=0;
    mp3_rd_len = mp3_refill_buf(mp3_rd_i, MP3_BUF_SZ);
    mp3_wr_len = mp3_refill_buf(mp3_wr_i, MP3_BUF_SZ);
  }else{ app.mp3_opened = 0; app.mp3_playing = 0; }
}

/* 매 주기(폴링)에서: 1) 급식 2) 작은 블록 읽기(최대 1024B) */
static void mp3_feed_task(void){
  if(!(app.mp3_opened && app.mp3_playing)) return;

  /* 1) DREQ=1 동안 32B씩 가능한 만큼 연속 투입 */
  while (MP3_DREQ == GPIO_PIN_SET && mp3_rd_off + 32 <= mp3_rd_len){
    VS1003_WriteData(&mp3_buf[mp3_rd_i][mp3_rd_off]);
    mp3_rd_off += 32;
  }

  /* 앞 버퍼 소진 시 스왑 */
  if(mp3_rd_off >= mp3_rd_len){
    mp3_rd_i ^= 1; mp3_wr_i ^= 1; mp3_rd_off=0; mp3_rd_len=mp3_wr_len; mp3_wr_len=0;
    if(mp3_rd_len==0){ // 파일 끝 → 다음 트랙
      uint16_t next = (app.cur_track+1) % (app.track_count?app.track_count:1);
      mp3_open_and_start(next); return;
    }
  }

  /* 2) 백버퍼가 비어 있으면 작은 블록(최대 1024B)만 리필 → 블로킹 최소화 */
  if(mp3_wr_len==0){
    uint32_t left = f_size(&app.fmp3) - f_tell(&app.fmp3);
    if(left){
      uint32_t want = (left >= 1024)? 1024 : left; // 작은 읽기로 지연 스파이크 완화
      mp3_wr_len = mp3_refill_buf(mp3_wr_i, want);
    }
  }
}

/* 탐색(±1% 단순 구현) */
static void mp3_seek_percent(int pct){
  if(!(app.mp3_opened)) return;
  FSIZE_t sz = f_size(&app.fmp3), cur = f_tell(&app.fmp3);
  FSIZE_t step = (FSIZE_t)((pct<0)?(-pct):pct) * (sz/100);
  FSIZE_t tgt  = (pct<0)? ((cur>step)? cur-step:0) : ((cur+step>sz)? sz:cur+step);
  f_lseek(&app.fmp3, tgt);
  // 리셋 버퍼
  mp3_rd_i=0; mp3_wr_i=1; mp3_rd_off=0;
  mp3_rd_len = mp3_refill_buf(mp3_rd_i, MP3_BUF_SZ);
  mp3_wr_len = mp3_refill_buf(mp3_wr_i, MP3_BUF_SZ);
}

/* ===================== 상태머신 ===================== */
static void app_enter(AppState s){
  app.st = s; lcd_all_clear_cache();
  switch(s){
    case ST_HOME:      lcd_home_draw(); break;
    case ST_ADC:       lcd_adc_draw();  break;
    case ST_MP3:       lcd_mp3_draw(cur_title()); break;
    case ST_CTRL:      lcd_ctrl_draw(); break;
    case ST_SYS_MENU:  lcd_sysmenu_draw(0); break;
    case ST_SYS_SET_TIME: rtc_load_from_ds3231(); lcd_datetime_draw(); break;
    case ST_SYS_INFO:  lcd_sysinfo_draw(); break;
    default: break;
  }
}

static uint8_t sys_idx = 0;

static void app_handle_event(AppEvent ev){
  switch(app.st){
    case ST_HOME:
      if(ev==EV_SW1 || ev==EV_SW2 || ev==EV_SW3 || ev==EV_SW4) beep_play(80);
      if(ev==EV_SW1) app_enter(ST_ADC);
      else if(ev==EV_SW2) app_enter(ST_MP3);
      else if(ev==EV_SW3) app_enter(ST_CTRL);
      else if(ev==EV_SW4) app_enter(ST_SYS_MENU);
      else if(ev==EV_TICK_1S) lcd_home_draw();
      break;

    case ST_ADC:
      if(ev==EV_SW4){ beep_play(80); app_enter(ST_HOME); }
      else if(ev==EV_SW1){ beep_play(80); app.adc_fmt = (app.adc_fmt+1)%3; lcd_adc_draw(); }
      else if(ev==EV_SW2){ beep_play(80); app.csv_on ^= 1; lcd_adc_draw(); }
      else if(ev==EV_SW3){ beep_play(80); app.cfg.csv_period_s = (app.cfg.csv_period_s==1)?5:1; lcd_adc_draw(); }
      else if(ev==EV_TICK_1S) lcd_adc_draw();
      break;

    case ST_MP3:
      if(ev==EV_SW4){ beep_play(80); mp3_stop(); app_enter(ST_HOME); }
      else if(ev==EV_SW2){ beep_play(80); app.mp3_playing ^= 1; lcd_mp3_draw(cur_title()); }
      else if(ev==EV_SW1){
        beep_play(80);
        if(app.track_count){ uint16_t prev=(app.cur_track + app.track_count - 1) % app.track_count; mp3_open_and_start(prev); lcd_mp3_draw(cur_title()); }
      }else if(ev==EV_SW3){
        beep_play(80);
        if(app.track_count){ uint16_t next=(app.cur_track + 1) % app.track_count; mp3_open_and_start(next); lcd_mp3_draw(cur_title()); }
      }else if(ev==EV_SW2L){
        vs1003_set_volume(app.cfg.mp3_volume); lcd_mp3_draw(cur_title());
      }else if(ev==EV_SW1L){ mp3_seek_percent(-1); }
      else if(ev==EV_SW3L){ mp3_seek_percent(+1); }
      else if(ev==EV_TICK_1S){ lcd_mp3_draw(cur_title()); }
      break;

    case ST_CTRL:
      if(ev==EV_SW4){ beep_play(80); app_enter(ST_HOME); }
      else if(ev==EV_SW2){ beep_play(80); app.auto_servo ^= 1; lcd_ctrl_draw(); }
      else if(ev==EV_SW1){ beep_play(80); if(app.cfg.servo_min_us>600) app.cfg.servo_min_us-=50; lcd_ctrl_draw(); }
      else if(ev==EV_SW3){ beep_play(80); if(app.cfg.servo_max_us<2600) app.cfg.servo_max_us+=50; lcd_ctrl_draw(); }
      else if(ev==EV_TICK_50MS || ev==EV_TICK_200MS){
        uint16_t src = app.auto_servo ? app.s.cds_raw : app.s.a1_raw;
        uint16_t pwm = app.cfg.servo_min_us + (uint32_t)(app.cfg.servo_max_us - app.cfg.servo_min_us) * src / 4095u;
        SERVO_SET_US(pwm);
        if(ev==EV_TICK_200MS) lcd_ctrl_draw();
      }
      break;

    case ST_SYS_MENU:
      if(ev==EV_SW4){ beep_play(80); app_enter(ST_HOME); }
      else if(ev==EV_SW1){ beep_play(80); if(sys_idx>0) sys_idx--; lcd_sysmenu_draw(sys_idx); }
      else if(ev==EV_SW3){ beep_play(80); if(sys_idx<1) sys_idx++; lcd_sysmenu_draw(sys_idx); }
      else if(ev==EV_SW2){
        beep_play(80);
        if(sys_idx==0){ app_enter(ST_SYS_SET_TIME); }
        else if(sys_idx==1){ app_enter(ST_SYS_INFO); }
      }
      break;

    case ST_SYS_SET_TIME:
      if(ev==EV_SW4){ beep_play(80); app_enter(ST_SYS_MENU); }
      else if(ev==EV_SW1){ beep_play(50); rtc_incdec(-1); lcd_datetime_draw(); }
      else if(ev==EV_SW3){ beep_play(50); rtc_incdec(+1); lcd_datetime_draw(); }
      else if(ev==EV_SW2){ beep_play(50); rtc_next_field(); lcd_datetime_draw(); }
      else if(ev==EV_SW2L){ beep_play(120); rtc_apply_to_ds3231(); app_enter(ST_SYS_MENU); }
      else if(ev==EV_TICK_1S){ lcd_datetime_draw(); }
      break;

    case ST_SYS_INFO:
      if(ev==EV_SW4){ beep_play(80); app_enter(ST_SYS_MENU); }
      else if(ev==EV_TICK_1S){ lcd_sysinfo_draw(); }
      break;

    default: break;
  }
}

/* 이벤트 큐 간단 버전 */
#define QMAX 16
static AppEvent q[QMAX]; static uint8_t qh, qt;
static void post_event(AppEvent ev){ uint8_t n=(qh+1)&(QMAX-1); if(n!=qt){ q[qh]=ev; qh=n; } }
static AppEvent get_event(void){ if(qt==qh) return EV_NONE; AppEvent e=q[qt]; qt=(qt+1)&(QMAX-1); return e; }

/* ===================== 공개 API ===================== */
void App_Init(void)
{
  memset(&app, 0, sizeof(app));
  cfg_load(&app.cfg);

  /* LCD */
  lcd_Init(APP_LCD_COLS, APP_LCD_ROWS);
  /* Boot Self-Test */
  boot_log_clear();
  boot_log_line("LCD(20x4)", "PASS!");
  /* I2C readiness checks */
  int ok_rtc = (HAL_OK == HAL_I2C_IsDeviceReady(&hi2c1, DS3231_I2C_ADDR<<1, 1, 100));
  boot_log_line("RTC(DS3231)", ok_rtc ? "PASS!" : "FAIL!");
  int ok_t1 = (HAL_OK == HAL_I2C_IsDeviceReady(&hi2c1, AS6221_ADDR1, 1, 100));
  boot_log_line("Temp1(AS6221@48)", ok_t1 ? "PASS!" : "FAIL!");
  int ok_t2 = (HAL_OK == HAL_I2C_IsDeviceReady(&hi2c1, AS6221_ADDR2, 1, 100));
  boot_log_line("Temp2(AS6221@49)", ok_t2 ? "PASS!" : "FAIL!");
  /* SD mount */
  FRESULT fr = f_mount(&SDFatFS, SDPath, 1);
  boot_log_line("SDCard(SDIO 4-bit)", (fr==FR_OK) ? "PASS!" : "FAIL!");
  /* VS1003 DREQ pin check */
  GPIO_PinState dreq = HAL_GPIO_ReadPin(VS1003_DREQ_GPIO_Port, VS1003_DREQ_Pin);
  boot_log_line("VS1003(DREQ)", (dreq==GPIO_PIN_SET) ? "PASS!" : "FAIL!");
  /* ADC quick read */
  uint16_t adc2 = ADC2_ReadOnce();
  boot_log_line("ADC2(one-shot)", (adc2<=4095) ? "PASS!" : "FAIL!");
  HAL_Delay(120);

  lcd_all_clear_cache();
  lcd_line_set(0, "Booting...");
  HAL_Delay(150);

  /* RTC(I2C2) */
  DS3231_Init(&hi2c1);
  /* RTC OSF guard: if oscillator stopped, go set time first */
  if(DS3231_IsOscillatorStopped()){
    app_enter(ST_SYS_SET_TIME);
  }


  /* AS6221(I2C1) */
  as_err1 = AS6221_Init(&as1); as_err2 = AS6221_Init(&as2);

  /* SD 마운트 */
  if(FR_OK == f_mount(&SDFatFS, SDPath, 1)){ LED2_ON(); scan_music_dir(); } else { LED2_OFF(); }

  /* VS1003 */
  VS1003_Init(); VS1003_SoftReset(); vs1003_set_volume(app.cfg.mp3_volume);

  /* 타이머 틱 초기화 */
  app.t_last_50  = ms_now(); app.t_last_200 = app.t_last_50; app.t_last_1s  = app.t_last_50;

  /* 첫 센서 리드 및 홈화면 */
  sensors_update_200ms();
  app_enter(ST_HOME);
}

void App_Task(void)
{
  uint32_t now = ms_now();

  /* 50ms: 버튼/부저 auto-off/작은 UI */
  if(now - app.t_last_50 >= TICK_50_MS){
    app.t_last_50 = now;
    scan_buttons_50ms();
    if (s_beep_deadline && (int32_t)(s_beep_deadline - now) <= 0){ s_beep_deadline = 0; BEEP_OFF(); }
    post_event(EV_TICK_50MS);
  }

  /* 200ms: 센서 */
  if(now - app.t_last_200 >= TICK_200_MS){ app.t_last_200 = now; sensors_update_200ms(); post_event(EV_TICK_200MS); }

  /* 1s: RTC/CSV */
  if(now - app.t_last_1s >= TICK_1S_MS){ app.t_last_1s = now; csv_log_1s(); post_event(EV_TICK_1S); }

  /* MP3 피드(비블로킹 우선, 작은 블록 읽기 보조) */
  mp3_feed_task();

  /* 이벤트 처리 */
  for(;;){ AppEvent ev = get_event(); if(ev == EV_NONE) break; app_handle_event(ev); }
}
