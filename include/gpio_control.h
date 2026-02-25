#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

//初始化GPIO芯片和门禁/蜂鸣器引脚
bool gpioInit();

//设置指定BCM引脚的输出电平
bool gpioSetValue(int pin_bcm, int value);

//门禁开门逻辑：控制继电器通电2秒后断电
void openDoorDelay();

//报警逻辑：控制蜂鸣器响0.5秒后停止
void alarmBeep();

//清理GPIO资源
void gpioCleanup();

#endif // GPIO_CONTROL_H