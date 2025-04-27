/*
 * execute.h
 *
 *  Created on: Apr 16, 2025
 *      Author: Admin
 */

#ifndef INC_EXECUTE_H_
#define INC_EXECUTE_H_

#include "main.h"

// Cấu trúc thời gian
struct Time {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint8_t year;
};

// Các hàm xử lý thời gian và hiển thị

// Hàm thiết lập và lấy thời gian từ DS1307
void SetTime(uint8_t hour, uint8_t min, uint8_t sec, uint8_t weekday, uint8_t day, uint8_t month, uint8_t year);
struct Time GetTime(void);

// Hiển thị thời gian qua UART
void TestDS1307(void);

// Hiển thị thông tin trên LCD
void SH1106_DisplayMessage(const char* message, uint8_t x, uint8_t y);
void ShowTimeOnLCDLoop(void);

// Kiểm tra và ghi log thẻ
void CheckCardAndLog(void);
void LogCard(struct Time time, uint8_t *cardID);
uint8_t IsCardValid(uint8_t *cardID);
void ProcessUARTCommands();
void TestSh1106(void);


#endif /* INC_EXECUTE_H_ */
