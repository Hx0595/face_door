/**
 * @file gpio_control.cpp
 * @brief 基于libgpiod的GPIO控制实现，用于门禁系统的继电器（开门）和蜂鸣器（报警）控制
 */
#include "gpio_control.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <gpiod.h>// libgpiod库头文件

// BCM 引脚编号
#define DOOR_PIN_BCM    18   // 继电器
#define BUZZER_PIN_BCM  17   // 蜂鸣器

// 全局变量
static struct gpiod_chip *chip = nullptr;               // GPIO芯片对象
static struct gpiod_line_request *door_req = nullptr;   // 继电器引脚请求对象
static struct gpiod_line_request *buzzer_req = nullptr; // 蜂鸣器引脚请求对象

// 初始化 GPIO
bool gpioInit() {
    // -----------------打开 GPIO 芯片------------------------------
    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        std::cerr << "[GPIO] 错误：无法打开 gpiochip0\n";
        return false;
    }
    
    // ---------------- 配置继电器引脚（DOOR_PIN_BCM = 18） ----------------
    // 1.1 创建引脚配置对象
    gpiod_line_settings* door_settings = gpiod_line_settings_new();
    if (!door_settings) {
        std::cerr << "[GPIO] 错误：创建继电器引脚设置失败\n";
        gpiod_chip_close(chip);// 释放已打开的芯片资源
        return false;
    }
    // 设置引脚为输出模式
    gpiod_line_settings_set_direction(door_settings, GPIOD_LINE_DIRECTION_OUTPUT);
    // 设置初始电平为低电平（INACTIVE），默认关闭继电器（门关闭）
    gpiod_line_settings_set_output_value(door_settings, GPIOD_LINE_VALUE_INACTIVE);

    // 1.2 创建线路配置对象（关联引脚和配置）
    gpiod_line_config* door_line_cfg = gpiod_line_config_new();
    if (!door_line_cfg) {
        std::cerr << "[GPIO] 错误：创建继电器引脚配置失败\n";
        gpiod_line_settings_free(door_settings);// 释放配置对象
        gpiod_chip_close(chip);                  // 释放芯片
        return false;
    }
    // 定义所需引脚数组
    unsigned int door_pin[] = {DOOR_PIN_BCM};
    // 将引脚和配置关联（1表示数组长度）
    gpiod_line_config_add_line_settings(door_line_cfg, door_pin, 1, door_settings);
    gpiod_line_settings_free(door_settings);// 配置完成，释放临时对象

     // 1.3 创建请求配置对象（设置引脚占用者名称）
    gpiod_request_config* door_req_cfg = gpiod_request_config_new();
    if (!door_req_cfg) {
        std::cerr << "[GPIO] 错误：创建继电器请求配置失败\n";
        gpiod_line_config_free(door_line_cfg);// 释放线路配置
        gpiod_chip_close(chip);               // 释放芯片
        return false;
    }
    // 设置引脚占用者标识（consumer）
    gpiod_request_config_set_consumer(door_req_cfg, "door_relay");

    // 1.4 申请引脚资源（占用引脚并应用配置）
    door_req = gpiod_chip_request_lines(chip, door_req_cfg, door_line_cfg);
    // 释放临时配置对象
    gpiod_line_config_free(door_line_cfg);
    gpiod_request_config_free(door_req_cfg);
    // 验证引脚申请结果
    if (!door_req) {
        std::cerr << "[GPIO] 错误：请求继电器引脚 " << DOOR_PIN_BCM << " 失败\n";
        gpiod_chip_close(chip);// 释放芯片
        return false;
    }

    // ---------------- 配置蜂鸣器引脚（BUZZER_PIN_BCM = 17） ----------------
    // 2.1 创建引脚配置对象
    gpiod_line_settings* buzzer_settings = gpiod_line_settings_new();
    if (!buzzer_settings) {
        std::cerr << "[GPIO] 错误：创建蜂鸣器引脚设置失败\n";
        gpiod_line_request_release(door_req);// 释放已申请的继电器引脚
        gpiod_chip_close(chip);              // 释放芯片
        return false;
    }
    // 设置为输出模式
    gpiod_line_settings_set_direction(buzzer_settings, GPIOD_LINE_DIRECTION_OUTPUT);
    // 初始电平为高电平（INACTIVE），默认关闭蜂鸣器
    gpiod_line_settings_set_output_value(buzzer_settings, GPIOD_LINE_VALUE_INACTIVE);

    // 2.2 创建线路配置对象
    gpiod_line_config* buzzer_line_cfg = gpiod_line_config_new();
    if (!buzzer_line_cfg) {
        std::cerr << "[GPIO] 错误：创建蜂鸣器引脚配置失败\n";
        gpiod_line_settings_free(buzzer_settings);// 释放配置对象
        gpiod_line_request_release(door_req);     // 释放继电器引脚
        gpiod_chip_close(chip);                   // 释放芯片
        return false;
    }
    // 定义蜂鸣器引脚数组
    unsigned int buzzer_pin[] = {BUZZER_PIN_BCM};
    // 关联引脚和配置
    gpiod_line_config_add_line_settings(buzzer_line_cfg, buzzer_pin, 1, buzzer_settings);
    gpiod_line_settings_free(buzzer_settings);// 释放临时配置

    // 2.3 创建请求配置对象
    gpiod_request_config* buzzer_req_cfg = gpiod_request_config_new();
    if (!buzzer_req_cfg) {
        std::cerr << "[GPIO] 错误：创建蜂鸣器请求配置失败\n";
        gpiod_line_config_free(buzzer_line_cfg);// 释放线路配置
        gpiod_line_request_release(door_req);   // 释放继电器引脚
        gpiod_chip_close(chip);                 // 释放芯片
        return false;
    }
    // 设置蜂鸣器引脚占用者标识
    gpiod_request_config_set_consumer(buzzer_req_cfg, "buzzer");

    // 2.4 申请蜂鸣器引脚资源
    buzzer_req = gpiod_chip_request_lines(chip, buzzer_req_cfg, buzzer_line_cfg);
    // 释放临时配置对象
    gpiod_line_config_free(buzzer_line_cfg);
    gpiod_request_config_free(buzzer_req_cfg);
    // 验证申请结果
    if (!buzzer_req) {
        std::cerr << "[GPIO] 错误：请求蜂鸣器引脚 " << BUZZER_PIN_BCM << " 失败\n";
        gpiod_line_request_release(door_req);// 释放继电器引脚
        gpiod_chip_close(chip);              // 释放芯片
        return false;
    }

    // 初始化成功日志
    std::cout << "[GPIO] 初始化成功 → 继电器(BCM18)、蜂鸣器(BCM17)\n";
    return true;
}

// 设置引脚电平（复用测试逻辑）
bool gpioSetValue(int pin_bcm, int value) {
    // 前置检查：GPIO芯片未初始化直接返回失败
    if (!chip) {
        std::cerr << "[GPIO] 错误：未初始化\n";
        return false;
    }

    // 将用户传入的0/非0转换为libgpiod标准电平枚举
    gpiod_line_value val = (value != 0) ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
    // 根据引脚编号选择对应的请求对象设置电平
    if (pin_bcm == DOOR_PIN_BCM && door_req) {
        // 设置继电器引脚电平，返回0表示成功
        return gpiod_line_request_set_value(door_req, pin_bcm, val) == 0;
    } else if (pin_bcm == BUZZER_PIN_BCM && buzzer_req) {
        // 设置蜂鸣器引脚电平，返回0表示成功
        return gpiod_line_request_set_value(buzzer_req, pin_bcm, val) == 0;
    } else {
        // 无效引脚或引脚未申请成功
        std::cerr << "[GPIO] 错误：无效引脚 " << pin_bcm << "\n";
        return false;
    }
}

/**
 * @brief 门禁开门逻辑：控制继电器通电2秒后断电（开门→关门）
 * @note 业务逻辑：识别成功后触发，高电平打开继电器（开门），2秒后低电平关闭
 */
void openDoorDelay() {
    std::cout << "\n=====================================\n";
    std::cout << "[门禁] 识别成功 → 开门2秒\n";
    // 输出高电平（ACTIVE）：继电器吸合，打开门禁
    gpioSetValue(DOOR_PIN_BCM, 1); 
    // 休眠2秒（保持开门状态）
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // 输出低电平（INACTIVE）：继电器断开，关闭门禁
    gpioSetValue(DOOR_PIN_BCM, 0); 
    std::cout << "[门禁] 门已关闭\n";
    std::cout << "=====================================\n\n";
}

/**
 * @brief 报警逻辑：控制蜂鸣器响0.5秒后停止
 * @note 业务逻辑：未知人脸触发，低电平（0）响铃，高电平（1）停止
 */
void alarmBeep() {
    std::cout << "[报警] 未知人脸 → 蜂鸣器响\n";
     // 输出低电平：触发蜂鸣器响铃
    gpioSetValue(BUZZER_PIN_BCM, 0); // 响
    // 持续响铃0.5秒
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // 输出高电平：停止蜂鸣器
    gpioSetValue(BUZZER_PIN_BCM, 1); // 停
}

/**
 * @brief 清理GPIO资源（避免资源泄漏）
 * @note 清理顺序：先释放引脚请求→再关闭芯片，与初始化顺序相反
 */
void gpioCleanup() {
    // 释放继电器引脚请求
    if (door_req)  gpiod_line_request_release(door_req);
    // 释放蜂鸣器引脚请求
    if (buzzer_req) gpiod_line_request_release(buzzer_req);
    // 关闭GPIO芯片
    if (chip)       gpiod_chip_close(chip);
    std::cout << "[GPIO] 资源已清理\n";
}