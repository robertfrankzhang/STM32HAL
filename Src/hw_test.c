
#include "definitions.h"

#ifdef HW_TEST // no need all if not defined

#include "hw_test.h"
#include "hardware_abstraction_layer.h"

extern uint8_t EventIRSample;

#include "serial.h"
#include "db.h"

// forward declaration
void hw_test_motor_on_off();

void hw_test_motor_overload();

void hw_test_ir_read();

void hw_test_battery_read();

void hw_test_tim1();

void hw_test_charger();

void hw_test_usb_serial();

// for test only
uint16_t test_val[100];


void hw_test() {
    // only select one to test
    int test_case = 6;

    switch (test_case) {
        case 0:
            hw_test_motor_on_off();
            break;
        case 1:
            hw_test_motor_overload();
            break;
        case 2:
            hw_test_ir_read();
            break;
        case 3:
            hw_test_battery_read();
            break;
        case 4:
            hw_test_tim1();
            break;
        case 5:
            hw_test_charger();
            break;
        case 6:
            hw_test_usb_serial();
            break;
    }
}

// test motor
void hw_test_motor_on_off() {
    while (1) {
        // spin dispense Motor
        spinDispenseMotor(1); // reverse
        HAL_Delay(1000);
        spinDispenseMotor(0); // forward
        HAL_Delay(1000);
        stopDispenseMotor(); // stop
        HAL_Delay(1000);
    }
}

void hw_test_motor_overload() {
    // hold motor spinning to check valtage
    uint32_t i;
    spinDispenseMotor(0);
    while (1) {
        for (i = 0; i < 99; ++i) {
            //test_val[i++] = getADC(getADC(batteryVoltageADC));
            test_val[i] = dispenseMotorCurrentAverage(128);
        }
        test_val[i] = dispenseMotorCurrentAverage(128);
        //stopDispenseMotor();

    }
}

void hw_test_ir_read() {
    while (1) {
        HAL_GPIO_WritePin(irReceiverPower, SET);
        for (int i = 0; i < 4; ++i) {
            HAL_GPIO_WritePin(IRLED, SET);
            HAL_Delay(1);
            test_val[2 * i] = getADC(irReceiverADC);
            HAL_GPIO_WritePin(IRLED, RESET);
            HAL_Delay(1);
            test_val[2 * i + 1] = getADC(irReceiverADC);
        }
        HAL_GPIO_WritePin(irReceiverPower, RESET);
        HAL_Delay(500);
    }
}

void hw_test_battery_read() {
    while (1) {
        for (int i = 0; i < 8; ++i) {
            test_val[i] = getADC(batteryVoltageADC);
            HAL_Delay(250);
        }
        HAL_Delay(100); // set break here
    }
}

void hw_test_tim1() {
    while (1) {
        startIRSamplingTimer();
        for (int i = 0; i < 8; ++i) {
            test_val[i] = htim1.Instance->CNT;
            HAL_Delay(10);
        }
        if (EventIRSample) {
            EventIRSample = 0;
            HAL_Delay(10);
        }
        HAL_Delay(10);
    }
}

void hw_test_charger() {
//  while(1){
//    if(batteryIsCharging())
//      HAL_GPIO_WritePin(chargeLED,SET);
//    else
//      HAL_GPIO_WritePin(chargeLED,RESET);
//    test_val[0] = batteryLevel();
//    HAL_Delay(100);
//  }
}

uint64_t packed_dataetime(RTC_TimeTypeDef time, RTC_DateTypeDef date) {
    uint64_t output = 0;
    output |= ((uint64_t) time.Seconds) << 0;
    output |= ((uint64_t) time.Minutes) << 8;
    output |= ((uint64_t) time.Hours) << 16;
    output |= ((uint64_t) date.Date) << 24;
    output |= ((uint64_t) date.Month) << 32;
    output |= ((uint64_t) date.Year) << 40;

    return output;
}

void hw_test_usb_serial() {
    usbHostConnect();
    uint8_t rxBuf[64] = {0};
    uint16_t rxLen = 0;
    populateDBWithFake();

    while (1) {
        rxLen = getSerialData(rxBuf, 64);

        if (rxBuf[0] == 'r') {
            for (int i = 0; i < _db.index; ++i) {
                serial_printf("i:%is:%ie:%idt:%u", i, _db.item[i].dispenseStatus, _db.item[i].event,
                              packed_dataetime(_db.item[i].time, _db.item[i].date));
            }
        }

        rxBuf[0] = '\0';

//    HAL_Delay(1000);
//    HAL_GPIO_WritePin(chargeLED,SET);
//    HAL_Delay(1000);
//    HAL_GPIO_WritePin(chargeLED,RESET);
    }
}

#endif // HW_TEST
