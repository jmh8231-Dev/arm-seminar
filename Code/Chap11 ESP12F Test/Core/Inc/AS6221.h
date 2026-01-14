#ifndef INC_AS6221_H_
#define INC_AS6221_H_

#include <stdint.h>
#include <stdbool.h>

// I2C 통신 타임아웃을 100ms로 설정
#define AS6221_I2C_TIMEOUT 100

// 라이브러리에서 발생할 수 있는 오류를 정의
typedef enum {
    AS6221_ERROR_NONE = 0,         // 오류 없음
    AS6221_ERROR_COMMUNICATION,    // 통신 오류
    AS6221_ERROR_INIT_FAILED       // 초기화 오류
} AS6221_Error_t;

// 변환률 설정을 위한 열거형 정의
typedef enum {
    ConvPer4000ms = 0,
    ConvPer1000ms = 1,
    ConvPer250ms = 2,
    ConvPer125ms = 3
} ConversionRate_t;

// 연속 오류 설정을 위한 열거형 정의
typedef enum {
    Single = 0,
    Double = 1,
    Triple = 2,
    Quadruple = 3
} ConsecutiveFaults_t;

// AS6221 센서의 설정 및 상태를 저장하는 구조체
typedef struct {
    uint16_t address;              // I2C 주소
    uint16_t TLOW;                 // 낮은 온도 임계값
    uint16_t THIGH;                // 높은 온도 임계값
    double Temp;                   // 현재 온도
    uint8_t CR : 2;                // 변환률 설정 (2비트)
    uint8_t CF : 2;                // 센서 구성 (2비트)
    bool SM : 1;                   // 동작 모드 (1비트)
    bool IM : 1;                   // 인터럽트 모드 (1비트)
    bool POL : 1;                  // 폴라리티 (1비트)
    bool SS : 1;                   // 셧다운 모드 (1비트)
} AS6221_t;

// 함수 프로토타입 선언
AS6221_Error_t AS6221_Init(AS6221_t *as6221);                   // 센서 초기화
bool AS6221_ReadTemperature(AS6221_t *as6221);                  // 온도 읽기
void AS6221_SetHighTemp(AS6221_t *as6221, double temperature);  // 높은 온도 임계값 설정
void AS6221_SetLowTemp(AS6221_t *as6221, double temperature);   // 낮은 온도 임계값 설정
AS6221_Error_t AS6221_GetLastError(void);                       // 마지막 오류 상태 반환

#endif /* INC_AS6221_H_ */
