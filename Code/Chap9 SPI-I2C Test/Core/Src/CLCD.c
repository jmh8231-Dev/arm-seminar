#include <stdio.h>
#include "CLCD.h"
#include "tim.h"
// 디스플레이가 전원을 인가받을 때 다음과 같이 구성됩니다:
//
// 1. 디스플레이 지움
// 2. 기능 설정:
//    DL = 1; 8비트 인터페이스 데이터
//    N = 0; 1줄 디스플레이
//    F = 0; 5x8 도트 글꼴
// 3. 디스플레이 켜기/끄기 제어:
//    D = 0; 디스플레이 끄기
//    C = 0; 커서 끄기
//    B = 0; 깜박임 끄기
// 4. 입력 모드 설정:
//    I/D = 1; 1씩 증가
//    S = 0; 이동 없음
//
// 그러나 아두이노를 재설정하더라도 LCD가 재설정되지 않으므로
// 스케치를 시작할 때 (그리고 LiquidCrystal 생성자가 호출될 때) 해당 상태에 있다고 가정할 수 없습니다.

/**
 *  main.h에서 Lcd_PortType data_ports[], Lcd_PinType data_pins[]를 검사하여 Lcd_PortType 및 Lcd_PinType 구조체를 초기화합니다.
 */

#ifdef _LCD_8BITMODE_
   Lcd_PortType data_ports[] = { D0_GPIO_Port, D1_GPIO_Port, D2_GPIO_Port, D3_GPIO_Port,
		D4_GPIO_Port, D5_GPIO_Port, D6_GPIO_Port, D7_GPIO_Port};
   Lcd_PinType  data_pins[] = { D0_Pin, D1_Pin, D2_Pin, D3_Pin,
		D4_Pin, D5_Pin, D6_Pin, D7_Pin};
#endif
#ifdef _LCD_4BITMODE_
   Lcd_PortType data_ports[] = {
		D4_GPIO_Port, D5_GPIO_Port, D6_GPIO_Port, D7_GPIO_Port};
   Lcd_PinType  data_pins[] = {
		D4_Pin, D5_Pin, D6_Pin, D7_Pin};
#endif

// 새 Lcd_HandleTypeDef를 생성하고 hlcd를 초기화합니다.
Lcd_HandleTypeDef hlcd;


void command(uint8_t value);
size_t write(uint8_t value);
void send(uint8_t value, GPIO_PinState mode);
void pulseEnable(void);
void write4bits(uint8_t value);
void write8bits(uint8_t value);

void lcd_Init(uint8_t cols, uint8_t lines)
{
	HAL_TIM_Base_Start(&htim4);

  hlcd.data_ports = data_ports;  // LCD 데이터 핀을 data_ports 배열에 복사
  hlcd.data_pins = data_pins;    // LCD 데이터 핀을 data_pins 배열에 복사

#ifdef _LCD_4BITMODE_
    hlcd.displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
#endif
#ifdef _LCD_8BITMODE_
    hlcd.displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
#endif


  if (lines > 1) {
    hlcd.displayfunction |= LCD_2LINE;
  }
  hlcd.numlines = lines;

  // 1줄 디스플레이의 경우 최소 10픽셀 높은 글꼴을 선택할 수 있습니다.
  if ((LCD_DOTSIZE != LCD_5x8DOTS) && (lines == 1)) {
    hlcd.displayfunction |= LCD_5x10DOTS;
  }

  // 데이터시트에 따르면 전원이 2.7V 이상이 되면 최소 40ms가 필요합니다.
  // 그래서 명령을 보내기 전에 최소 50ms를 기다립니다.
  HAL_Delay(50);
  // 이제 RS 및 R/W 핀을 모두 낮은 상태로 끌어내려 명령을 시작합니다.
  HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);

  // 4비트 또는 8비트 모드로 LCD를 설정합니다.
#ifdef _LCD_4BITMODE_
    write4bits(0x03);
    HAL_Delay(4);

    write4bits(0x03);
    delay_us(100);

    write4bits(0x03);
    delay_us(150);

    write4bits(0x02);
    delay_us(100);
#endif
#ifdef _LCD_8BITMODE_
    command(LCD_FUNCTIONSET | hlcd.displayfunction);
    delay_us(4500);

    command(LCD_FUNCTIONSET | hlcd.displayfunction);
    delay_us(150);
#endif

  // 최종적으로 라인 수, 글꼴 크기 등을 설정합니다.
  command(LCD_FUNCTIONSET | hlcd.displayfunction);

  // 디스플레이를 커서 또는 깜박임 없이 켭니다.
  hlcd.displaycontrol = LCD_DISPLAYOFF | LCD_CURSOROFF | LCD_BLINKOFF;
  command(LCD_DISPLAYCONTROL | hlcd.displaycontrol);

  // 디스플레이를 지웁니다.
  lcd_clear();
  HAL_Delay(3);

  // 기본 텍스트 방향을 설정합니다.
  hlcd.displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // 입력 모드를 설정합니다.
  command(LCD_ENTRYMODESET | hlcd.displaymode);

  lcd_display();
}


/********** 사용자를 위한 고수준 명령 **********/
void lcd_clear()
{
  command(LCD_CLEARDISPLAY);
  HAL_Delay(2);
}

void lcd_home()
{
  command(LCD_RETURNHOME);
  HAL_Delay(2);
}

void lcd_setCursor(uint8_t col, uint8_t row)
{
  uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if ( row > hlcd.numlines ) {
    row = hlcd.numlines-1;
  }

  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void lcd_noDisplay() {
  hlcd.displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | hlcd.displaycontrol);
}

void lcd_display() {
  hlcd.displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | hlcd.displaycontrol);
}

void lcd_noCursor() {
  hlcd.displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | hlcd.displaycontrol);
}
void lcd_cursor() {
  hlcd.displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | hlcd.displaycontrol);
}

void lcd_noBlink() {
  hlcd.displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | hlcd.displaycontrol);
}
void lcd_blink() {
  hlcd.displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | hlcd.displaycontrol);
}

void lcd_scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void lcd_scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void lcd_leftToRight(void) {
  hlcd.displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | hlcd.displaymode);
}

void lcd_rightToLeft(void) {
  hlcd.displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | hlcd.displaymode);
}

void lcd_autoscroll(void) {
  hlcd.displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | hlcd.displaymode);
}

void lcd_noAutoscroll(void) {
  hlcd.displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | hlcd.displaymode);
}

void lcd_createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7;
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    write(charmap[i]);
  }
}

/*********** 텍스트 출력을 위한 중간 수준의 명령 ***********/
void lcd_putchar(char c) {
	write(c);
}

void lcd_string(char * str_data) {
   while(*str_data) {
	  write(*str_data++);
   }
}

void lcd_setCurStr(uint8_t col, uint8_t row,  char * str) {
	lcd_setCursor(col, row);
	lcd_string(str);
}

/*********** 데이터 및 명령 전송을 위한 저수준 명령 ***********/
void command(uint8_t value) {
  send(value, GPIO_PIN_RESET);
}

size_t write(uint8_t value) {
  send(value, GPIO_PIN_SET);
  return 1;
}

void delay_us(uint16_t time) {
	__HAL_TIM_SET_COUNTER(&htim4, 0);
	while((__HAL_TIM_GET_COUNTER(&htim4))<time);
}

void send(uint8_t value, GPIO_PinState mode) {
  HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, mode);

  #ifdef _LCD_8BITMODE_
    write8bits(value);
  #endif
  #ifdef _LCD_4BITMODE_
    write4bits(value>>4);
    write4bits(value);
  #endif
}

void pulseEnable(void) {
  HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);
  delay_us(1);
  HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);
  delay_us(1);
  HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);
  delay_us(50);
}

#ifdef _LCD_4BITMODE_
void write4bits(uint8_t value) {
  for (uint8_t i = 0; i < 4; i++) {
    HAL_GPIO_WritePin(hlcd.data_ports[i], hlcd.data_pins[i],  (value >> i) & 0x01 );
  }

  pulseEnable();
}
#endif

#ifdef _LCD_8BITMODE_
void write8bits(uint8_t value) {
  for (uint8_t i = 0; i < 8; i++) {
	  HAL_GPIO_WritePin(hlcd.data_ports[i], hlcd.data_pins[i],  (value >> i) & 0x01 );
  }

  pulseEnable();
}
#endif
