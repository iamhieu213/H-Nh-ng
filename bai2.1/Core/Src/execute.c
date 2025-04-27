/*
 * execute.c
 *
 *  Created on: Apr 16, 2025
 *      Author: Admin
 */

 #include "execute.h"
 #include "SH1106.h"
 #include "fonts.h"
 #include "tm_stm32f4_mfrc522.h"
 #include <string.h>
 #include <stdio.h>
 
 // Biến toàn cục ngoại vi
 extern I2C_HandleTypeDef hi2c3;
 extern SPI_HandleTypeDef hspi4;
 extern UART_HandleTypeDef huart1;
 
 // Danh sách thẻ hợp lệ
 uint8_t validCards[10][5] = {
     {0x12, 0x34, 0x56, 0x78, 0x90},
     {0xA3, 0x69, 0x96, 0x34, 0x68}
 };
 uint8_t validCardCount = 2;
 uint8_t validCardMaxCount = 10;
 
 // Cấu trúc bản ghi
 typedef struct {
     struct Time time;
     uint8_t cardID[5];
 } LogEntry;
 
 LogEntry logData[100];
 uint8_t logCount = 0;
 
 void SetTime(uint8_t hour, uint8_t min, uint8_t sec, uint8_t weekday, uint8_t day, uint8_t month, uint8_t year) {
     struct Time time;
     time.sec = sec;
     time.min = min;
     time.hour = hour;
     time.weekday = weekday;
     time.day = day;
     time.month = month;
     time.year = year;
 
     HAL_I2C_Mem_Write(&hi2c3, 0xD0, 0, 1, (uint8_t *)&time, 7, 1000);
 }
 
//   struct Time GetTime() {
//       struct Time currentTime;
//       HAL_I2C_Mem_Read(&hi2c3, 0xD1, 0, 1, (uint8_t *)&currentTime, 7, 1000);
//       return currentTime;
//   }
 uint8_t BCD_To_Decimal(uint8_t bcd) {
     return ((bcd >> 4) * 10) + (bcd & 0x0F);
 }
struct Time GetTime() {
   struct Time rawTime;
   struct Time currentTime;

   // Đọc dữ liệu thô từ RTC
   HAL_I2C_Mem_Read(&hi2c3, 0xD1, 0, 1, (uint8_t *)&rawTime, 7, 1000);

   // Chuyển từng trường từ BCD sang Decimal
   currentTime.sec     = BCD_To_Decimal(rawTime.sec);
   currentTime.min     = BCD_To_Decimal(rawTime.min);
   currentTime.hour    = BCD_To_Decimal(rawTime.hour);
   currentTime.weekday = BCD_To_Decimal(rawTime.weekday);
   currentTime.day     = BCD_To_Decimal(rawTime.day);
   currentTime.month   = BCD_To_Decimal(rawTime.month);
   currentTime.year    = BCD_To_Decimal(rawTime.year);

   return currentTime;
}

 void SH1106_DisplayMessage(const char* message, uint8_t x, uint8_t y) 
 {
     char buf[100];
     sprintf(buf, "%s", message);
     SH1106_GotoXY(x, y);
     SH1106_Puts(buf, &Font_7x10, 1);
     SH1106_UpdateScreen();
 }
 
 
 void LogCard(struct Time time, uint8_t *cardID) {
     if (logCount >= 100) return;
     memcpy(logData[logCount].cardID, cardID, 5);
     logData[logCount].time = time;
     logCount++;
 }
 
 uint8_t IsCardValid(uint8_t *cardID) {
     for (uint8_t i = 0; i < validCardCount; i++) {
         if (memcmp(cardID, validCards[i], 5) == 0)
             return 1;
     }
     return 0;
 }
 
 void CheckCardAndLog() {
     uint8_t cardID[5];
     SH1106_Fill(0);
     if (TM_MFRC522_Check(cardID) == MI_OK) {
         // Bật LED3 báo phát hiện thẻ
         HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
         struct Time currentTime = GetTime();
         //SH1106_DisplayMessage("Card found", 10, 20);
         if (IsCardValid(cardID)) {
             HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);  // Bật LED4
             SH1106_DisplayMessage("Welcome", 10, 20);
             LogCard(currentTime, cardID);
         } else {
             HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);  // Tắt LED4
             SH1106_DisplayMessage("Rejected", 10, 20);
         }
 
     } else {
         // Không có thẻ: tắt cả hai LED
         HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
         HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
         SH1106_DisplayMessage("---------", 12, 10);
     }
 }

 void TestDS1307() {
    char buff[30];

    SetTime(0x8, 0x25, 0x45, 0x0, 0x19, 0x4, 0x25);

    while (1) {
        struct Time now = GetTime();

        sprintf(buff, "%02d:%02d:%02d-%02d-%02d/%02d/%02d \n",
                now.hour, now.min, now.sec,
                now.weekday, now.day, now.month, now.year);

        HAL_UART_Transmit(&huart1, (uint8_t *)buff, strlen(buff), 1000);
        HAL_Delay(300);
    }
}

void ShowTimeOnLCDLoop() {
    char buffer[50];
    struct Time now;

    SH1106_Init();
    HAL_Delay(500);
    SetTime(0x8, 0x25, 0x45, 0x0, 0x19, 0x4, 0x25);
    while (1) {
        now = GetTime();
        SH1106_Fill(SH1106_COLOR_BLACK);

        sprintf(buffer, "%02d:%02d:%02d", now.hour, now.min, now.sec);
        SH1106_DisplayMessage(buffer, 10, 0);

        sprintf(buffer, "%02d/%02d/%02d", now.day, now.month, now.year);
        SH1106_DisplayMessage(buffer, 20, 20);

        SH1106_UpdateScreen();
        HAL_Delay(300);
    }
}

void ProcessUARTCommands() {
    uint8_t rxBuf[20];

    // Nhận luôn 6 byte: 1 lệnh + 5 data
    if (HAL_UART_Receive(&huart1, rxBuf, 6, 300) == HAL_OK) {
        switch (rxBuf[0]) {
            case 'A': { // Thêm thẻ hợp lệ mới
                if (validCardCount < validCardMaxCount) {
                    memcpy(validCards[validCardCount], &rxBuf[1], 5);
                    validCardCount++;
                    const char *msg = "Card added\n";
                    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);
                }
                break;
            }
            case 'L': { // Liệt kê log
                char msg[100];
                for (int i = 0; i < logCount; i++) {
                    sprintf(msg, "[%02d:%02d:%02d] %02X %02X %02X %02X %02X\n",
                            logData[i].time.hour, logData[i].time.min, logData[i].time.sec,
                            logData[i].cardID[0], logData[i].cardID[1], logData[i].cardID[2],
                            logData[i].cardID[3], logData[i].cardID[4]);
                    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 300);
                }
                break;
            }
            default: {
                const char *msg = "Unknown cmd\n";
                HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);
                break;
            }
        }
    }
}


void TestSh1106(void)
{
    SH1106_Init();
    TM_MFRC522_Init();
    HAL_Delay(500);

    while (1) {
        uint8_t CardID[5];
        HAL_Delay(100);
        SH1106_Fill(0);

        if (TM_MFRC522_Check(CardID) == MI_OK) {
            char cardBuf[50];

            // Hiển thị dòng "Card found"
            SH1106_DisplayMessage("Card found", 12, 10);

            // Format ID thẻ thành chuỗi
            sprintf(cardBuf, "%02X %02X %02X %02X %02X",
                    CardID[0], CardID[1], CardID[2], CardID[3], CardID[4]);

            // Hiển thị dòng ID thẻ bên dưới
            SH1106_DisplayMessage(cardBuf, 12, 22);
        } else {
            SH1106_DisplayMessage("---------", 12, 10);
        }

    }
}


// Hàm chuyển 1 byte từ BCD về Decimal


