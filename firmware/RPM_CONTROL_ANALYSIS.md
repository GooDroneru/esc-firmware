# AM32 Firmware — Анализ возможностей RPM-контроля для соосного регулятора

## Содержание
1. [Текущая версия прошивки](#1-текущая-версия-прошивки)
2. [Архитектура прошивки](#2-архитектура-прошивки)
3. [Существующие возможности RPM-контроля](#3-существующие-возможности-rpm-контроля)
4. [DShot протокол — текущая реализация](#4-dshot-протокол--текущая-реализация)
5. [Телеметрия и измерение оборотов](#5-телеметрия-и-измерение-оборотов)
6. [PID-регуляторы](#6-pid-регуляторы)
7. [Параметры мотора и расчёт RPM](#7-параметры-мотора-и-расчёт-rpm)
8. [Что можно добавить для соосника](#8-что-можно-добавить-для-соосника)
9. [Новая версия AM32 (v2.20) — что появилось](#9-новая-версия-am32-v220--что-появилось)
10. [Протоколы для RPM-контроля](#10-протоколы-для-rpm-контроля)
11. [Рекомендации по реализации](#11-рекомендации-по-реализации)

---

## 1. Текущая версия прошивки

**Версия: ~2.08** (по changelog в `firmware/src/main.c`, последняя запись — v2.08)

**Поддерживаемые MCU:**
| MCU | Семейство | Примечание |
|-----|-----------|------------|
| STM32F051 | STMICRO | Основной целевой MCU |
| STM32G071 | STMICRO | |
| STM32G431 | STMICRO | FDCAN (CAN FD) — DroneCAN |
| STM32L431 | STMICRO | bxCAN — DroneCAN |
| GD32E230 | GIGADEVICES | |
| AT32F415 | ARTERY | bxCAN — DroneCAN |
| AT32F421 | ARTERY | |
| CH32V203 | WCH RISC-V | |
| K19XXVK035 | NIIET | Российский MCU |

---

## 2. Архитектура прошивки

```
firmware/
├── src/
│   ├── main.c            # Главный цикл, PID, управление мотором
│   ├── dshot.c           # Обработка DShot пакетов, телеметрия
│   ├── signal.c          # Обнаружение и обработка входного сигнала
│   ├── functions.c       # Вспомогательные функции (map, delay, CRC)
│   ├── sounds.c          # Звуковые сигналы
│   ├── kiss_telemetry.c  # KISS UART телеметрия
│   └── firmwareversion.c # Версия прошивки
├── inc/
│   ├── common.h          # Общие определения, структура fastPID, enum inputType
│   ├── eeprom.h          # Структура EEPROM (настройки)
│   ├── dshot.h           # Интерфейс DShot
│   ├── targets.h         # Конфигурации целевых плат (~5200 строк)
│   ├── functions.h       # Прототипы функций
│   ├── signal.h          # Прототипы обработки сигнала
│   └── ...
└── mcu/
    ├── k19xxvk035/       # Специфичный код для NIIET
    └── v203/             # Специфичный код для CH32V203
```

**Ключевые переменные управления:**

| Переменная | Файл | Назначение |
|-----------|------|-----------|
| `newinput` | main.c | Сырой входной сигнал (0-2047) |
| `adjusted_input` | main.c | Обработанный входной сигнал |
| `input` | main.c | Финальное значение для duty cycle |
| `duty_cycle_setpoint` | main.c | Уставка duty cycle |
| `target_e_com_time` | main.c | Целевое время электрической коммутации |
| `e_com_time` | main.c | Текущее время коммутации (мкс × 0.5) |
| `e_rpm` | main.c | Электрические обороты (×10) |
| `motor_kv` | main.c | KV мотора (вычисляется из EEPROM) |
| `drive_by_rpm` | main.c | Флаг режима управления по RPM |
| `use_speed_control_loop` | main.c | Флаг активного PID скорости |

---

## 3. Существующие возможности RPM-контроля

### 3.1 drive_by_rpm (v1.83+)

**Уже реализован!** В прошивке есть полноценный режим управления по оборотам.

```c
// firmware/src/main.c:272-274
uint8_t drive_by_rpm = 0;
uint32_t MAXIMUM_RPM_SPEED_CONTROL = 10000;
uint32_t MINIMUM_RPM_SPEED_CONTROL = 1000;
```

**Как работает:**
- Входной сигнал (47-2047) маппится в диапазон MINIMUM_RPM..MAXIMUM_RPM
- Вычисляется `target_e_com_time`:
  ```
  target_e_com_time = 60000000 / mapped_rpm / (motor_poles / 2)
  ```
- PID-регулятор `speedPid` сравнивает `e_com_time` с `target_e_com_time`
- Выход PID (`input_override`) подменяет входной сигнал

```c
// firmware/src/main.c:1131-1146
if (use_speed_control_loop) {
    if (drive_by_rpm) {
        target_e_com_time = 60000000 / 
            map(adjusted_input, 47, 2047, MINIMUM_RPM_SPEED_CONTROL, MAXIMUM_RPM_SPEED_CONTROL) 
            / (eepromBuffer.motor_poles / 2);
        if (adjusted_input < 47) { // dead band
            input = 0;
            speedPid.error = 0;
            input_override = 0;
        } else {
            input = (uint16_t)(input_override / 10000);
            // ... clamping ...
        }
    }
}
```

**Проблема:** `drive_by_rpm = 0` по умолчанию и **не настраивается через EEPROM** — нужно менять в коде или добавлять поле в EEPROM.

### 3.2 FIXED_SPEED_MODE

**Compile-time опция** для работы на фиксированных оборотах:

```c
// firmware/src/main.c:257-260
// #define FIXED_SPEED_MODE  // bypasses input signal and runs at a fixed rpm
// using the speed control loop PID
//#define FIXED_SPEED_MODE_RPM  1000  // intended final rpm
```

Активация (`main.c:1810-1814`):
```c
#ifdef FIXED_SPEED_MODE
    use_speed_control_loop = 1;
    target_e_com_time = 60000000 / FIXED_SPEED_MODE_RPM / (eepromBuffer.motor_poles / 2);
#endif
```

### 3.3 Low RPM Power Protection

Защита мощности на низких оборотах, основанная на KV:

```c
// firmware/src/main.c:784-785
low_rpm_level = motor_kv / 100 / (32 / eepromBuffer.motor_poles);
high_rpm_level = motor_kv / 12 / (32 / eepromBuffer.motor_poles);
```

Ограничивает duty cycle на низких оборотах через маппинг:
```c
// firmware/src/main.c:2122-2123
duty_cycle_maximum = map(k_erpm, low_rpm_level, high_rpm_level, 
    throttle_max_at_low_rpm, throttle_max_at_high_rpm);
```

---

## 4. DShot протокол — текущая реализация

### 4.1 Структура пакета DShot

```
| 11 бит: throttle (48-2047) | 1 бит: telem request | 4 бит: CRC |
```

**Файл:** `firmware/src/dshot.c`

### 4.2 Обработка входящих пакетов

```c
// firmware/src/dshot.c:74-267
void computeDshotDMA() {
    // 1. Декодирование 16 бит из DMA buffer
    // 2. Проверка CRC
    // 3. Определение: bidirectional telemetry (inverted signal)
    // 4. Разделение: throttle (48-2047) vs команды (1-47)
    // 5. Обработка команд (6 повторений для подтверждения)
}
```

**Поддерживаемые DShot-команды:**

| Команда | Действие |
|---------|----------|
| 1-5 | Маяковые звуки |
| 6 | Отправка info о ESC |
| 7 | Направление: нормальное |
| 8 | Направление: обратное |
| 9 | Bi-direction: выкл |
| 10 | Bi-direction: вкл |
| 12 | Сохранить настройки |
| 13 | Extended DShot Telemetry: вкл |
| 14 | Extended DShot Telemetry: выкл |
| 20 | Направление: нормальное (3D) |
| 21 | Направление: обратное (3D) |
| 36 | Программирование (eeprom write) |

### 4.3 Bidirectional DShot (eRPM телеметрия)

**Реализован!** При `dshot_telemetry = 1` (inverted signal) ESC отправляет eRPM обратно:

```c
// firmware/src/dshot.c:269-379
void make_dshot_package(uint16_t com_time) {
    // GCR-кодирование eRPM в ответный пакет
    // Формат: eee mmm mmm mmm (3 бита экспонента + 9 бит мантисса)
    dshot_full_number = ((shift_amount << 9) | (com_time >> shift_amount));
    // GCR encode + checksum
}
```

### 4.4 Extended DShot Telemetry (EDT)

**Реализован!** Команда 13 включает EDT. Чередование eRPM и расширенных данных:

```c
// firmware/src/dshot.c:282-305
// Планировщик телеметрии:
// - Temperature: каждые 200 вызовов (~4Hz при 800Hz)
// - Voltage: каждые 200 вызовов (~4Hz)
// - Current: каждые 40 вызовов (~20Hz)
// - eRPM: все остальные слоты

// Типы кадров EDT:
// 0b0010_xxxxxxxx — температура (°C)
// 0b0100_xxxxxxxx — напряжение (/ 25 = V)
// 0b0110_xxxxxxxx — ток (/ 50 = A, делитель зависит от hardware)
```

---

## 5. Телеметрия и измерение оборотов

### 5.1 Измерение eRPM (BEMF)

Обороты измеряются через время между коммутациями (back-EMF zero-crossing):

```c
// firmware/src/main.c:2115-2116
e_rpm = running * (600000 / e_com_time);  // в десятках RPM
k_erpm = e_rpm / 10;  // e_com_time — время одной эл. коммутации в мкс
```

**e_com_time** вычисляется как среднее последних 6 интервалов:
```c
// firmware/src/main.c:1873
e_com_time = ((commutation_intervals[0] + ... + commutation_intervals[5]) + 4) >> 1;
```

### 5.2 Формулы пересчёта

```
eRPM = 60,000,000 / e_com_time         (e_com_time в 0.5мкс единицах)
RPM  = eRPM / (motor_poles / 2)
e_com_time = 60,000,000 / target_eRPM   (для задания RPM)
```

### 5.3 KISS Telemetry (UART)

```c
// firmware/src/kiss_telemetry.c:8-25
void makeTelemPackage(uint8_t temp, uint16_t voltage, uint16_t current, 
                      uint16_t consumption, uint16_t e_rpm)
{
    // 9 байт: temp(1) + voltage(2) + current(2) + consumption(2) + e_rpm(2) + crc(1)
}
```

### 5.4 RPM Pulse Output

Некоторые targets имеют вывод импульсов на GPIO:
```c
// firmware/inc/targets.h:417-418
#define RPM_PULSE_PORT GPIOB
#define RPM_PULSE_PIN  LL_GPIO_PIN_10
```

---

## 6. PID-регуляторы

### 6.1 Структура fastPID

```c
// firmware/inc/common.h:74-85
typedef struct fastPID {
    int32_t error;
    uint32_t Kp, Ki, Kd;
    int32_t integral, derivative, last_error;
    int32_t pid_output;
    int32_t integral_limit;
    int32_t output_limit;
} fastPID;
```

### 6.2 PID скорости (speedPid)

```c
// firmware/src/main.c:277-283
fastPID speedPid = {
    .Kp = 10,
    .Ki = 0,
    .Kd = 100,
    .integral_limit = 10000,
    .output_limit = 50000
};
```

Вызывается в 1kHz цикле:
```c
// firmware/src/main.c:1426-1437
if (use_speed_control_loop && running) {
    input_override += doPidCalculations(&speedPid, e_com_time, target_e_com_time);
    // ... clamping ...
    if (zero_crosses < 100) {
        speedPid.integral = 0;  // сброс интегратора при старте
    }
}
```

### 6.3 PID тока (currentPid)

```c
// firmware/src/main.c:285-291
fastPID currentPid = {
    .Kp = 400,
    .Ki = 0,
    .Kd = 1000,
    .integral_limit = 20000,
    .output_limit = 100000
};
```

### 6.4 PID защиты отストール (stallPid)

```c
// firmware/src/main.c:293-299
fastPID stallPid = {
    .Kp = 1,
    .Ki = 0,
    .Kd = 50,
    .integral_limit = 10000,
    .output_limit = 50000
};
```

---

## 7. Параметры мотора и расчёт RPM

### 7.1 Хранение KV в EEPROM

```c
// firmware/inc/eeprom.h:37
uint8_t motor_kv;  // EEPROM position 26
```

Вычисление при загрузке:
```c
// firmware/src/main.c:642
motor_kv = (eepromBuffer.motor_kv * 40) + 20;
// Диапазон: 20..10220 KV (шаг 40)
```

### 7.2 Полюса мотора

```c
// firmware/inc/eeprom.h:38
uint8_t motor_poles;  // EEPROM position 27 (default: 14)
```

### 7.3 Формула максимальных оборотов

```
Max_RPM = KV × Voltage
Max_eRPM = Max_RPM × (motor_poles / 2)
```

**Пример:** Motor KV=2000, 3S LiPo (12.6V), 14 poles
```
Max_RPM = 2000 × 12.6 = 25,200 RPM
Max_eRPM = 25,200 × 7 = 176,400 eRPM
```

### 7.4 Текущий расчёт RPM-уровней

```c
// firmware/src/main.c:784-785
low_rpm_level  = motor_kv / 100 / (32 / eepromBuffer.motor_poles);  // thousand eRPM
high_rpm_level = motor_kv / 12  / (32 / eepromBuffer.motor_poles);  // thousand eRPM
```

Эти значения используются для low RPM throttle protection.

---

## 8. Что можно добавить для соосника

### 8.1 Приоритет: HIGH — Сделать drive_by_rpm настраиваемым

**Проблема:** `drive_by_rpm` — compile-time variable, не в EEPROM.

**Решение:**
1. Добавить поле в `eepromBuffer`:
```c
// В eeprom.h, зарезервированные байты 13-16:
// reserved_eeprom_3[0] → drive_by_rpm (0/1)
// reserved_eeprom_3[1] → min_rpm_percent (0-100)
// reserved_eeprom_3[2] → max_rpm_percent (0-100)
```

2. Загрузка из EEPROM в `loadEEpromSettings()`:
```c
drive_by_rpm = eepromBuffer.reserved_eeprom_3[0];
MINIMUM_RPM_SPEED_CONTROL = eepromBuffer.reserved_eeprom_3[1] * 100;
MAXIMUM_RPM_SPEED_CONTROL = eepromBuffer.reserved_eeprom_3[2] * 100;
```

### 8.2 Приоритет: HIGH — DShot RPM-relative control

**Идея:** Использовать DShot throttle (48-2047) как процент от Max RPM, вычисленного из KV.

**Текущее поведение в drive_by_rpm:**
```
input (47-2047) → map → MINIMUM_RPM..MAXIMUM_RPM → target_e_com_time
```

**Предлагаемое улучшение:**
```
input (47-2047) → процент (0-100%)
Max_RPM = motor_kv × battery_voltage
target_RPM = процент × Max_RPM / 100
target_e_com_time = 60000000 / target_RPM / (motor_poles / 2)
```

**Модификация в main.c:1132-1133:**
```c
// Было:
target_e_com_time = 60000000 / 
    map(adjusted_input, 47, 2047, MINIMUM_RPM_SPEED_CONTROL, MAXIMUM_RPM_SPEED_CONTROL) 
    / (eepromBuffer.motor_poles / 2);

// Стало:
uint32_t max_rpm_from_kv = motor_kv * (battery_voltage / 100);  // battery_voltage in 0.01V
uint32_t target_rpm = map(adjusted_input, 47, 2047, 0, max_rpm_from_kv);
if (target_rpm < 500) target_rpm = 0;  // dead zone
target_e_com_time = 60000000 / target_rpm / (eepromBuffer.motor_poles / 2);
```

### 8.3 Приоритет: HIGH — Voltage-compensated RPM control

Добавить компенсацию по напряжению батареи для стабильных оборотов при разряде:

```c
// В 1kHz PID loop, перед расчётом target_e_com_time:
uint32_t compensated_max_rpm = motor_kv * (battery_voltage / 100);
// Ограничить target_rpm compensated_max_rpm × 0.95 (запас)
```

### 8.4 Приоритет: MEDIUM — Feedforward для speedPid

Текущий `speedPid` имеет `Ki = 0`, что означает нет integral windup, но и нет устранения steady-state error.

**Улучшение:**
```c
// Добавить feedforward term:
int32_t ff_term = map(target_e_com_time, min_com_time, max_com_time, 2000, 48);
input_override = ff_term * 10000 + doPidCalculations(&speedPid, ...);
```

### 8.5 Приоритет: MEDIUM — Координация двух моторов (coaxial)

Для соосника нужно:
1. **Одинаковые target RPM** для обоих ESC — обеспечивается FC
2. **Противоположные направления** вращения — DShot команды 7/8 или 20/21
3. **Torque balancing** — если один мотор нагружен больше, компенсировать через второй

**Архитектура:**
```
FC (flight controller)
  ├── DShot/DroneCAN → ESC Upper Motor (CW) → RPM_pid → throttle
  └── DShot/DroneCAN → ESC Lower Motor (CCW) → RPM_pid → throttle
  
  RPM_upper_target = base_rpm + yaw_correction
  RPM_lower_target = base_rpm - yaw_correction
```

### 8.6 Приоритет: LOW — RPM slew rate limiting

Ограничение скорости изменения RPM для предотвращения десинхронизации:

```c
// Добавить в eepromBuffer:
uint8_t rpm_slew_rate;  // RPM change per second limit

// В 1kHz loop:
static uint32_t current_target_rpm = 0;
int32_t max_change = rpm_slew_rate / 1000;  // per 1ms tick
if (abs(target_rpm - current_target_rpm) > max_change) {
    current_target_rpm += (target_rpm > current_target_rpm) ? max_change : -max_change;
} else {
    current_target_rpm = target_rpm;
}
```

---

## 9. Новая версия AM32 (v2.20) — что появилось

**Новый репозиторий:** https://github.com/am32-firmware/AM32 (консолидированный, все MCU в одном проекте)

### 9.1 Ключевые новые фичи (v2.19 — v2.20)

| Фича | Описание | Полезность для соосника |
|------|----------|------------------------|
| **Active Brake** | Регенеративное торможение при сбросе газа | HIGH — быстрый отклик |
| **Low Voltage Cut-off** | Отключение по напряжению/ячейке | MEDIUM — безопасность |
| **Adjustable Ramp Speed** | 1s..1/50s ramp (throttle rate of change) | HIGH — плавность RPM |
| **8-144 kHz PWM** | Расширенный диапазон PWM | MEDIUM — эффективность |
| **0-30° timing advance** | Точная настройка опережения (1° шаг) | MEDIUM — эффективность на высоких RPM |
| **Current PID Loop** | PID-ограничение тока (P до 500, I до 200, D до 500) | HIGH — защита мотора |
| **Min duty cycle** | Минимальный duty 0-25% | MEDIUM — стабильность на малых оборотах |
| **EEPROM v3** | Новый формат настроек | Required |
| **Auto PWM frequency** | Автоматическая подстройка PWM | MEDIUM |
| **Fixed Speed Mode в setInput** | RPM-режим через входной сигнал | HIGH — уже сделано в upstream |
| **Improved zero cross filtering** | Улучшенная фильтрация BEMF | HIGH — стабильность на низких RPM |
| **Improved startup for low KV** | Улучшенный старт для низких KV моторов | HIGH |
| **Comparator interrupt blanking** | Бланкирование компаратора | MEDIUM |
| **RPM-based comp power level** | Мощность компаратора зависит от RPM | MEDIUM |
| **Ramp rate increases** | Увеличенная скорость ramp | MEDIUM |

### 9.2 DroneCAN поддержка (v2.18+)

**Доступна для:** STM32G431 (FDCAN), STM32L431 (bxCAN), AT32F415 (bxCAN)

**Что даёт:**
- Приём throttle команд через CAN bus
- Публикация телеметрии (RPM, температура, напряжение, ток)
- Динамическое назначение адресов (DNA)
- Управление параметрами через CAN
- Обновление прошивки через CAN

**Файлы:** `Src/DroneCAN/DroneCAN.c`, `Src/DroneCAN/sys_can_*.c`

**Для соосника DroneCAN — лучший протокол:**
- Оба ESC на одной шине
- FC может задавать RPM напрямую
- Телеметрия RPM в реальном времени
- Синхронизация между ESC

### 9.3 Конфигуратор

**Новый конфигуратор:** https://am32.ca + Config Tool v1.93+
- Веб-интерфейс
- Поддержка EEPROM v3
- Все новые настройки

---

## 10. Протоколы для RPM-контроля

### 10.1 Сравнительная таблица

| Протокол | RPM-контроль | Телеметрия | Сложность | Для соосника |
|----------|-------------|------------|-----------|--------------|
| **DShot 600 + drive_by_rpm** | Да (throttle→RPM) | eRPM (bidir) | Низкая | OK |
| **DShot + EDT** | Да | eRPM + temp + V + I | Средняя | Хорошо |
| **DroneCAN** | Прямой RPM-команда | Полная телеметрия | Высокая | **Лучший** |
| **KISS UART** | Нет (только telemetry) | RPM + V + I + temp | Низкая | Доп. канал |
| **PWM/Servo** | Через throttle | Нет | Низкая | Не подходит |
| **Serial (CRSF/ELRS)** | Через FC | Зависит | Средняя | Возможно |

### 10.2 Рекомендуемый подход: DShot + RPM-relative

**Протокол:** DShot600 с bidirectional telemetry

**Логика:**
1. FC вычисляет `Max_RPM = motor_KV × battery_voltage`
2. FC задаёт `target_RPM = input_percent × Max_RPM / 100`
3. FC маппит `target_RPM` в DShot throttle (48-2047)
4. ESC в режиме `drive_by_rpm` интерпретирует throttle как RPM
5. ESC отправляет реальный eRPM через bidirectional DShot
6. FC сравнивает target и actual RPM → коррекция

**Альтернатива (внутри ESC):**
- ESC сам знает KV и voltage
- ESC сам вычисляет Max_RPM
- DShot throttle = процент от Max_RPM
- Не требует вычислений на FC

### 10.3 DroneCAN — максимальный потенциал

**Преимущества для соосника:**
- Прямой RPM-команда (не нужно маппить throttle)
- Оба ESC видят друг друга на шине
- FC может синхронизировать RPM с микросекундной точностью
- Полная телеметрия без дополнительного провода
- Возможность future-proof: torque control, position control

**Недостатки:**
- Нужен MCU с CAN (G431, L431, F415)
- Более сложная прошивка
- Нужен CAN-трансивер на плате

---

## 11. Рекомендации по реализации

### 11.1 Быстрый путь (DShot + drive_by_rpm)

**Шаги:**
1. Добавить `drive_by_rpm` в EEPROM (зарезервированные байты)
2. Добавить `max_rpm_percent` и `min_rpm_percent` в EEPROM
3. Модифицировать `drive_by_rpm` логику для процентного RPM от Max (KV × V)
4. Добавить voltage compensation в target RPM
5. Настроить speedPid для быстрого отклика
6. Добавить Config Tool UI для новых параметров

**Файлы для модификации:**
- `firmware/inc/eeprom.h` — добавить поля
- `firmware/src/main.c:784-785, 1131-1146, 1426-1437` — логика RPM
- `firmware/src/main.c:277-283` — PID tuning

### 11.2 Оптимальный путь (DShot + EDT + voltage-compensated RPM)

**В дополнение к быстрому пути:**
1. Включить EDT по умолчанию (command 13 при старте)
2. Использовать battery_voltage для динамического Max_RPM
3. Добавить feedforward в speedPid
4. Добавить RPM slew rate limiting
5. Добавить anti-windup для integral

### 11.3 Лучший путь (DroneCAN)

1. Перенести изменения из upstream AM32 v2.20
2. Использовать MCU с CAN (STM32G431, L431)
3. Реализовать DroneCAN ESC node с RPM-командами
4. Добавить DroneCAN messages для RPM setpoint
5. Реализовать синхронизацию двух ESC через CAN

### 11.4 Конкретный план действий

```
Фаза 1: Минимальный RPM-контроль (1-2 дня)
├── Добавить drive_by_rpm в EEPROM
├── Добавить min/max RPM percent в EEPROM  
├── Тест: стабильность RPM при изменении напряжения
└── Тест: точность удержания RPM

Фаза 2: Voltage-compensated RPM (1-2 дня)
├── Вычислять Max_RPM = KV × V_bat
├── Маппить DShot throttle → % от Max_RPM
├── Тест: RPM при разряде батареи
└── Тест: переходные процессы

Фаза 3: Оптимизация PID (2-3 дня)
├── Настроить speedPid (P, I, D)
├── Добавить feedforward
├── Добавить slew rate limiting
└── Тест: step response, settling time

Фаза 4: Соосная координация (2-3 дня)
├── Два ESC с противоположным направлением
├── RPM balancing (torque compensation)
├── Тест на стенде
└── Тест в полёте
```

---

## Приложение A: Ключевые файлы и строки

| Файл | Строки | Описание |
|------|--------|----------|
| `src/main.c` | 272-274 | `drive_by_rpm`, RPM limits |
| `src/main.c` | 277-299 | PID структуры (speed, current, stall) |
| `src/main.c` | 565-588 | `doPidCalculations()` |
| `src/main.c` | 642 | `motor_kv` из EEPROM |
| `src/main.c` | 784-785 | `low_rpm_level`, `high_rpm_level` |
| `src/main.c` | 1131-1146 | drive_by_rpm логика |
| `src/main.c` | 1401-1437 | 1kHz PID loop |
| `src/main.c` | 1810-1814 | FIXED_SPEED_MODE активация |
| `src/main.c` | 2115-2116 | e_rpm вычисление |
| `src/dshot.c` | 74-267 | `computeDshotDMA()` — обработка DShot |
| `src/dshot.c` | 269-379 | `make_dshot_package()` — telemetry |
| `src/kiss_telemetry.c` | 8-25 | KISS UART telemetry |
| `inc/eeprom.h` | 10-77 | EEPROM структура |
| `inc/common.h` | 74-85 | `fastPID` структура |
| `inc/common.h` | 90-97 | `inputType` enum |

---

## Приложение B: Формулы

```
# Электрические обороты из commutation interval
eRPM = 60,000,000 / e_com_time          (e_com_time в 0.5мкс)

# Механические обороты
RPM = eRPM / (motor_poles / 2)

# Целевое время коммутации для заданного RPM
target_e_com_time = 60,000,000 / target_eRPM / (motor_poles / 2)

# Максимальные обороты из KV
Max_RPM = KV × Voltage
Max_eRPM = Max_RPM × (motor_poles / 2)

# KV из EEPROM
motor_kv = (eepromBuffer.motor_kv × 40) + 20

# Напряжение (зависит от target voltage divider)
voltage = ADC_raw / TARGET_VOLTAGE_DIVIDER

# DShot throttle → RPM (в режиме drive_by_rpm)
target_RPM = map(dshot_throttle, 47, 2047, MIN_RPM, MAX_RPM)

# Voltage-compensated RPM
Max_RPM_dynamic = KV × (battery_voltage / 100)
target_RPM = percent × Max_RPM_dynamic / 100
```

---

## Приложение C: Ссылки

- **Текущий репозиторий (старый):** https://github.com/AlkaMotors/AM32-MultiRotor-ESC-firmware
- **Новый репозиторий (актуальный):** https://github.com/am32-firmware/AM32
- **Релизы:** https://github.com/am32-firmware/AM32/releases
- **Конфигуратор:** https://am32.ca
- **DShot Handbook:** https://brushlesswhoop.com/dshot-and-bidirectional-dshot/
- **DroneCAN:** https://dronecan.github.io/
- **Rotorflight Governor:** https://rotorflight.org/docs/setup/governor.md
- **ArduPilot RSC Governor:** https://ardupilot.org/copter/docs/traditional-helicopter-internal-rsc-governor.html
