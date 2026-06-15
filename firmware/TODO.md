# TODO: esc-firmware

## 🔴 Critical

### Баги

- [ ] **DShot programming mode — buffer overflow** — `src/dshot.c:146` — `position` (11-bit, 0..2047) пишет в `eepromBuffer.buffer[192]` без проверки границ — arbitrary write в RAM через DShot
- [ ] **Division by zero: `e_com_time`** — `src/main.c:2115` — `600000 / e_com_time` — на старте или после desync `e_com_time = 0` → hard fault
- [ ] **Division by zero: `map()`** — `src/main.c:1133` — делитель `(eepromBuffer.motor_poles / 2)` может быть 0
- [ ] **Division by zero: `MILLIVOLT_PER_AMP`** — `src/main.c:2062` — не определён на таргетах `AIRBEE_F421`, `TBS_MINI_F421`, `NEUTRON_1_2S_AIO_F421`

### Race conditions

- [ ] **Missing `volatile` на ~25 shared globals (ISR ↔ main)** — список:
  - `src/main.c` — `newinput`, `input`, `adjusted_input`, `armed`, `running`, `duty_cycle`, `duty_cycle_setpoint`, `commutation_interval`, `step`, `e_com_time`, `zero_crosses`, `signaltimeout`, `lastzctime`, `thiszctime`, `waitTime`, `bemfcounter`, `bad_count`, `bemf_timeout_happened`, `desync_happened`, `old_routine`, `desync_check`, `dshot`, `servoPwm`, `dshotcommand`, `prop_brake_active`, `inputSet`, `send_esc_info_flag`, `PROCESS_ADC_FLAG`
  - `src/dshot.c:52` — `send_telemetry`
- [ ] **Race в duty cycle pipeline** — `src/main.c:1317` — `duty_cycle = duty_cycle_setpoint` — оба без volatile, ISR читает `duty_cycle` на 20kHz
- [ ] **Критический race: `commutation_interval` пишется из 3 контекстов** — `src/main.c:891` (TIM ISR), `src/main.c:1578` (main loop), читается `src/main.c:931-934` (EXTI ISR)
- [ ] **`dma_buffer[64]` не volatile — DMA + ISR data race** — `mcu/v203/Src/IO.c:17`, `mcu/k19xxvk035/Src/IO.c:19`

### Interrupt safety

- [ ] **`__RAMFUNC` — no-op на CH32V203** — `inc/common.h:8-10` — `interruptRoutine`, `tenKhzRoutine`, `PeriodElapsedCallback` и ещё 5 ISR остаются во flash, не в RAM. При flash write — 100% HardFault
- [ ] **Interrupts не отключаются при flash write** — все `save_flash_nolib` (4 варианта) не дисаблят прерывания. На CH32V203 + баг `__RAMFUNC` = гарантированный HardFault при DShot save

### EEPROM / настройки

- [ ] **Нет EEPROM magic/checksum** — `src/main.c:591-793` — `loadEEpromSettings()` не валидирует данные. Частичная запись/коррупция → ESC применяет мусор
- [ ] **Перегрев защиты нет по дефолту** — `src/main.c:718-719` — uninitialized EEPROM → `temperature = 255` → защита никогда не сработает
- [ ] **`current_limit` выключен по дефолту** — `src/main.c:722-724` — без установки через конфигуратор — нет SW overcurrent protection

## 🟠 High

### DShot

- [ ] **DShot CRC — 4 бита** — `src/dshot.c:86-87, 127` — 1/16 шанс ложного принятия шума как команды
- [ ] **Telemetry CRC инверсия сломана** — `src/dshot.c:121-123` — `~checkCRC + 16` — несовместимо с `calcCRC`
- [ ] **`programming_mode` без timeout** — `src/dshot.c:133-150` — после входа (command 36) при потере frame — ESC перестаёт принимать throttle
- [ ] **Programming mode не дисаблит мотор** — `src/dshot.c:255` — `armed = 0` закомментирован — мотор молотит во время EEPROM write
- [ ] **DShot decode без per-bit тайминг валидации** — `src/dshot.c:76-84` — `pdiff > halfpulsetime` — нет min/max pulse width

### Flash операции

- [ ] **Missing erase при записи** — `mcu/v203/Src/eeprom.c:35-38` — erase только если `add % 256 == 0`, иначе пишется поверх старых данных
- [ ] **Нет re-entrance guard** — `src/dshot.c:228`, `src/signal.c:89`, `src/main.c:1723` — одновременный вызов `saveEEpromSettings()` = interleaved flash write
- [ ] **Flash busy-wait без timeout** — `mcu/v203/Src/eeprom.c:20-23` — `while(FLASH->STATR_BSY)` с комментарием `/*add time-out*/` — нет таймаута
- [ ] **Flash возвраты не проверяются** — все `FLASH_ProgramPage_Fast()`, `MFLASH_WriteData()` — return value игнорируется

### Signal capture

- [ ] **`detectInput()` хардкодит 31 итерацию** — `src/signal.c:275-284` — при `buffersize < 31` читает мусор из DMA буфера
- [ ] **`computeMSInput()` проверяет 2 записи** — `src/signal.c:36-47` — только `dma_buffer[1] - dma_buffer[0]`
- [ ] **Нет glitch filter в edge detection** — `src/signal.c:275-283` — один шумовой спайк валидно декодируется

### Power-safe

- [ ] **Нет power-loss protection в EEPROM write** — при пропадании питания mid-erase — полная потеря настроек, `loadEEpromSettings()` не обнаружит коррупцию
- [ ] **Servo calibration пишет partial threshold** — `src/signal.c:68-72, 83-88` — `eepromBuffer.servo.*` без volatile, mid-update → мусор

### Motor control

- [ ] **Shoot-through в `proportionalBrake()`** — `mcu/v203/Src/phaseouts.c:42-58` — GPIO CFGHR/CFGLR переключаются неатомарно, потенциальное cross-conduction
- [ ] **Sound блокирует IRQ на 600ms** — `src/sounds.c:98, 191, 217, 279` — `__disable_irq()` на время startup tune — все ISR пропущены
- [ ] **`__disable_irq()` в commutation path** — `src/main.c:870-874` — `comStep()` без IRQ — может пропустить zero-cross

## 🟡 Medium

- [ ] **`eepromBuffer` shared ISR/main без volatile** — `inc/eeprom.h` — читается из ISR и пишется main
- [ ] **`dead_time_override` может сменить encoding BDTR** — `src/main.c:689-716` — `|= 200` (0xC8) устанавливает бит 7, меняя режим кодирования dead-time
- [ ] **`bad_count` (uint8_t) overflow** — `src/main.c:833-845` — после 255 → 0, несколько команд без фильтрации
- [ ] **`map()` рекурсивный — 12 уровней (~576 байт стека)** — `src/functions.c:22-40` — риск stack overflow на МК с 8KB RAM
- [ ] **HardFault/K19: NMI_Handler пустой — fall-through** — `mcu/k19xxvk035/Src/K19xxVK035_it.c:40-42` — `{}`, не `while(1){}`
- [ ] **Несбалансированное `disable_irq/enable_irq`** — `src/main.c:870-874` — если `comStep` выбросит, IRQ не включатся
- [ ] **PWM частота — integer truncation** — `src/main.c:623-624` — `* 100 / 6` теряет точность, частота может не совпадать с заданной
- [ ] **`DUTY_LIMIT_99` `#define` внутри функции** — `src/main.c:1475` — не scope-ограничен
- [ ] **`presc == 60` дубликат в K19XXVK035** — `mcu/k19xxvk035/Src/peripherals.c:115-130` — dead code
- [ ] **K19XX: EEPROM save с unsigned wrap на малых остатках** — `mcu/k19xxvk035/Src/eeprom.c:26-36` — `bytes_left - 4u` при `bytes_left < 4` → огромное число
- [ ] **K19XX: EEPROM read 8 байт несмотря на остаток** — `mcu/k19xxvk035/Src/eeprom.c:46-57` — может читать за границу flash
- [ ] **DShot GCR padding не обновляется при runtime смене** — `mcu/k19xxvk035/Src/peripherals.c:413` — `buffer_padding` меняется с 7 до 14, DMA SRC_END_PTR не обновлён
- [ ] **Нет overvoltage protection** — `src/main.c:2066-2092` — только low-voltage cutoff
- [ ] **Zero-cross filter (12 проверок) может пропустить легитимный ZC** — `src/main.c:920-928`

## 🟢 Low

- [ ] **`pwmSin[340]` (680 байт) в RAM вместо .rodata** — `src/main.c:487-512`
- [ ] **Magic numbers по всему коду** — `47, 1047, 48, 2047, 1000, 2000, 3300, 4095, 45000` и т.д.
- [ ] **Phase naming: `phaseAPWM()` конфигуряет Phase C** — `mcu/v203/Src/phaseouts.c:268-310`
- [ ] **Telemetry DMA стартует без проверки завершения предыдущей** — `mcu/v203/Src/serial_telemetry.c:14-27`
- [ ] **Averaging filter 50 семплов — первые 2.5 сек заниженные токи** — `src/main.c:800-811`
- [ ] **NOP-10 delay в ADC ISR — хрупкий timing** — `mcu/k19xxvk035/Src/K19xxVK035_it.c:76-78`
- [ ] **HardFault_Handler пустой — нет диагностики** — `mcu/k19xxvk035/Src/K19xxVK035_it.c:51-56`
- [ ] **K19XX: WDT INTEN+RESEN оба включены — может маскировать зависания** — `mcu/k19xxvk035/Src/peripherals.c:223-224`
- [ ] **`forward` reversal code (`1 - dir_reversed`) — запутанно** — `src/dshot.c:212-218`
- [ ] **EXTI3_IRQHandler не проверяет источник прерывания** — `mcu/v203/Src/ch32v20x_it.c:195-217`
