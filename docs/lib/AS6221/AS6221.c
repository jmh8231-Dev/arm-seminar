// AS6221.c
#include "AS6221.h"
#include "i2c.h"

// 레지스터 주소 정의
#define AS6221_REG_TEMP     0x00
#define AS6221_REG_CFG      0x01
#define AS6221_REG_TLOW     0x02
#define AS6221_REG_THIGH    0x03

// 온도 변환 계수 정의
#define TEMP_CONVERSION_FACTOR 0.0078125

// 마지막 오류 상태를 저장하는 변수
static AS6221_Error_t last_error = AS6221_ERROR_NONE;

// I2C를 통해 레지스터에 값을 쓰는 함수
static inline bool AS6221_WriteRegister(uint16_t address, uint8_t reg, uint16_t value) {
    uint8_t data[3] = {reg, (value >> 8) & 0xFF, value & 0xFF};
    if (HAL_I2C_Master_Transmit(&hi2c1, address, data, 3, AS6221_I2C_TIMEOUT) != HAL_OK) {
        last_error = AS6221_ERROR_COMMUNICATION; // 통신 오류 발생 시 오류 상태 갱신
        return false;
    }
    return true;
}

// I2C를 통해 레지스터로부터 값을 읽는 함수
static inline bool AS6221_ReadRegister(uint16_t address, uint8_t reg, uint16_t* value) {
    uint8_t data[2];
    if (HAL_I2C_Mem_Read(&hi2c1, address, reg, I2C_MEMADD_SIZE_8BIT, data, 2, AS6221_I2C_TIMEOUT) != HAL_OK) {
        last_error = AS6221_ERROR_COMMUNICATION; // 통신 오류 발생 시 오류 상태 갱신
        return false;
    }
    *value = (data[0] << 8) | data[1]; // 읽은 값을 16비트로 결합
    return true;
}

// AS6221 센서를 초기화하는 함수
AS6221_Error_t AS6221_Init(AS6221_t *as6221) {
    uint16_t config = (as6221->CR << 6) | (as6221->SM << 8) | (as6221->IM << 9) |
                      (as6221->POL << 10) | (as6221->CF << 11) | (as6221->SS << 15);

    if (!AS6221_WriteRegister(as6221->address, AS6221_REG_CFG, config)) {
        return AS6221_ERROR_INIT_FAILED; // 초기화 실패 시 오류 반환
    }

    return AS6221_ERROR_NONE; // 초기화 성공 시 오류 없음 반환
}

// 센서로부터 온도를 읽어오는 함수
bool AS6221_ReadTemperature(AS6221_t *as6221) {
    uint16_t rawTemp;
    if (!AS6221_ReadRegister(as6221->address, AS6221_REG_TEMP, &rawTemp)) {
        as6221->Temp = 0.0; // 읽기 실패 시 온도를 0으로 설정
        return false;
    }
    as6221->Temp = (int16_t)rawTemp * TEMP_CONVERSION_FACTOR; // 읽은 값을 실제 온도로 변환
    return true;
}

// 높은 온도 임계값을 설정하는 함수
void AS6221_SetHighTemp(AS6221_t *as6221, double temperature) {
    uint16_t threshold = (uint16_t)(temperature / TEMP_CONVERSION_FACTOR);
    AS6221_WriteRegister(as6221->address, AS6221_REG_THIGH, threshold);
}

// 낮은 온도 임계값을 설정하는 함수
void AS6221_SetLowTemp(AS6221_t *as6221, double temperature) {
    uint16_t threshold = (uint16_t)(temperature / TEMP_CONVERSION_FACTOR);
    AS6221_WriteRegister(as6221->address, AS6221_REG_TLOW, threshold);
}

// 마지막 오류 상태를 반환하는 함수
AS6221_Error_t AS6221_GetLastError(void) {
    return last_error;
}
