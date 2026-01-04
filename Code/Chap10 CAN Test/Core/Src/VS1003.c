/* Includes ------------------------------------------------------------------*/
#include "VS1003.h"

uint8_t vs1003ram[5] = { 0 , 0 , 0 , 0 , 250 };

extern SPI_HandleTypeDef hspi1;

static void VS1003_SPI_SetSpeed( uint8_t SpeedSet)
{
	hspi1.Instance = SPI1;
	  hspi1.Init.Mode = SPI_MODE_MASTER;
	  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
	  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
	  hspi1.Init.NSS = SPI_NSS_SOFT;
	  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	  hspi1.Init.CRCPolynomial = 10;

	  if(SpeedSet == SPI_SPEED_LOW)
	  {
		  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	  }
	  else
	  {
		  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	  }

	  if (HAL_SPI_Init(&hspi1) != HAL_OK)
	  {
	    Error_Handler();
	  }
}

/*******************************************************************************
*******************************************************************************/ 
static uint16_t VS1003_SPI_ReadWriteByte(uint16_t TxData)
{	
	uint8_t RxData;
	HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&TxData, &RxData, 1, 10);
	return RxData;
}

/*******************************************************************************
*******************************************************************************/ 
void VS1003_Init(void)
{		  
	MP3_RESET(0);
	HAL_Delay(1);
	MP3_RESET(1);

	MP3_DCS(1);
	MP3_CCS(1);
}

/*******************************************************************************
*******************************************************************************/ 
void VS1003_WriteReg(uint8_t reg, uint16_t value)
{  
	while(  MP3_DREQ ==0 );

	VS1003_SPI_SetSpeed( SPI_SPEED_LOW );	 
	MP3_DCS(1); 
	MP3_CCS(0); 
	VS1003_SPI_ReadWriteByte(VS1003_WRITE_COMMAND);
	VS1003_SPI_ReadWriteByte(reg);
	VS1003_SPI_ReadWriteByte(value >> 8);
	VS1003_SPI_ReadWriteByte(value);
	MP3_CCS(1); 
	VS1003_SPI_SetSpeed( SPI_SPEED_HIGH );
} 

/*******************************************************************************
*******************************************************************************/ 
uint16_t VS1003_ReadReg( uint8_t reg)
{ 
	uint16_t value;

	while(MP3_DREQ == 0);
	VS1003_SPI_SetSpeed(SPI_SPEED_LOW);
	MP3_DCS(1);     
	MP3_CCS(0);
	VS1003_SPI_ReadWriteByte(VS1003_READ_COMMAND);
	VS1003_SPI_ReadWriteByte(reg);
	value = VS1003_SPI_ReadWriteByte(0xff);
	value = value << 8;
	value |= VS1003_SPI_ReadWriteByte(0xff);
	MP3_CCS(1);
	VS1003_SPI_SetSpeed(SPI_SPEED_HIGH);
	return value; 
} 

/*******************************************************************************
*******************************************************************************/                       
void VS1003_ResetDecodeTime(void)
{
   VS1003_WriteReg(SPI_DECODE_TIME, 0x0000);
   VS1003_WriteReg(SPI_DECODE_TIME, 0x0000);
}

/*******************************************************************************
*******************************************************************************/    
uint16_t VS1003_GetDecodeTime(void)
{ 
   return VS1003_ReadReg(SPI_DECODE_TIME);   
} 

/*******************************************************************************
*******************************************************************************/
void VS1003_SoftReset(void)
{
	uint8_t retry; 	
				 
	while(  MP3_DREQ ==0 );
    VS1003_SPI_ReadWriteByte(0xff);
    retry = 0;
    while(VS1003_ReadReg(SPI_MODE) != 0x0804)
    {
    	VS1003_WriteReg(SPI_MODE, 0x0804);
    	HAL_Delay(2);
     if(retry++ > 100)
     {
    	 break;
     }
    }

    while(MP3_DREQ == 0);
    retry = 0;
    while(VS1003_ReadReg(SPI_CLOCKF) != 0x9800)
    {
    	VS1003_WriteReg(SPI_CLOCKF, 0X9800);
    	if(retry++ > 100)
    	{
    		break;
    	}
    }

    retry = 0;
    while(VS1003_ReadReg(SPI_AUDATA) != 0xBB81)
    {
    	VS1003_WriteReg(SPI_AUDATA, 0xBB81);
    			if(retry++ > 100)
    			{
    				break;
    			}
    }

    VS1003_WriteReg(SPI_VOL, 0x0000);
    VS1003_ResetDecodeTime();

    MP3_DCS(0);
    VS1003_SPI_ReadWriteByte(0xFF);
    VS1003_SPI_ReadWriteByte(0xFF);
    VS1003_SPI_ReadWriteByte(0xFF);
    VS1003_SPI_ReadWriteByte(0xFF);
    MP3_DCS(1);
    HAL_Delay(20);
} 

/*******************************************************************************
*******************************************************************************/
void VS1003_SineTest(void)
{	
	VS1003_WriteReg(SPI_VOL, 0X2020);
	VS1003_WriteReg(SPI_MODE, 0x0820);
	while(MP3_DREQ == 0);

	MP3_DCS(0);
	VS1003_SPI_WriteByte(0x53);
	VS1003_SPI_WriteByte(0xef);
	VS1003_SPI_WriteByte(0x6e);
	VS1003_SPI_WriteByte(0x24);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	HAL_Delay(100);
	MP3_DCS(1);
	
	MP3_DCS(0);
	VS1003_SPI_WriteByte(0x45);
	VS1003_SPI_WriteByte(0x78);
	VS1003_SPI_WriteByte(0x69);
	VS1003_SPI_WriteByte(0x74);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	HAL_Delay(100);
	MP3_DCS(1);

	MP3_DCS(0);     
	VS1003_SPI_WriteByte(0x53);
	VS1003_SPI_WriteByte(0xef);
	VS1003_SPI_WriteByte(0x6e);
	VS1003_SPI_WriteByte(0x44);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	HAL_Delay(100);
	MP3_DCS(1); 

	MP3_DCS(0);      
	VS1003_SPI_WriteByte(0x45);
	VS1003_SPI_WriteByte(0x78);
	VS1003_SPI_WriteByte(0x69);
	VS1003_SPI_WriteByte(0x74);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	VS1003_SPI_WriteByte(0x00);
	HAL_Delay(100);
	MP3_DCS(1); 
}

/*******************************************************************************
*******************************************************************************/
void VS1003_Reset(void)
{
	MP3_RESET(0);
	HAL_Delay(20);
	MP3_RESET(1);
	HAL_Delay(20);
	VS1003_SPI_ReadByte(0xFF);
	MP3_DCS(1);
	MP3_CCS(1);
	while(MP3_DREQ == 0);
	HAL_Delay(20);
}

/*******************************************************************************
*******************************************************************************/
void VS1003_RamTest(void)
{
   volatile uint16_t value;

   VS1003_Reset();
   VS1003_WriteReg(SPI_MODE,0x0820);
   while(MP3_DREQ ==0);
   MP3_DCS(0);
   VS1003_SPI_WriteByte(0x4d);
   VS1003_SPI_WriteByte(0xea);
   VS1003_SPI_WriteByte(0x6d);
   VS1003_SPI_WriteByte(0x54);
   VS1003_SPI_WriteByte(0x00);
   VS1003_SPI_WriteByte(0x00);
   VS1003_SPI_WriteByte(0x00);
   VS1003_SPI_WriteByte(0x00);
   HAL_Delay(50);  
   MP3_DCS(1);
   value = VS1003_ReadReg(SPI_HDAT0);
}     
		 				
/*******************************************************************************
*******************************************************************************/   
void VS1003_SetVol(void)
{
   uint8_t i;
   uint16_t bass=0;
   uint16_t volt=0;
   uint8_t  vset=0;
		 
   vset = 255 - vs1003ram[4];
   volt = vset;
   volt <<= 8;
   volt += vset;
   /* 0,henh.1,hfreq.2,lenh.3,lfreq */      
   for( i = 0; i < 4; i++ )
   {
       bass <<= 4;
       bass += vs1003ram[i]; 
   }     
   VS1003_WriteReg(SPI_BASS, bass);  
   VS1003_WriteReg(SPI_VOL, volt); 
}

void VS1003_WriteData(uint8_t* buf)
{
	uint8_t count = 32;
	
	MP3_DCS(0);
	while(count--)
	{
		VS1003_SPI_ReadWriteByte(*buf++);
	}
	MP3_DCS(1);
	MP3_CCS(1);
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
