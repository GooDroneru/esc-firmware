# Отчёт: таргет K1921VG5T (NIIET RISC-V) для esc-firmware

Дата: 2026-09-02. Ветка с `827b661`/`c7febf6` доведена до собираемого состояния,
порт переработан под нативную периферию vg5t (по UM_K1921VG5T, 534 стр.),
проверены сборки vg5t + vk035 + бутлоадера vg5t.

## 1. Плата (пинмап, табл. 3.2 UM)

| Вывод | Функция | AF |
|---|---|---|
| PA8 / PA9 | PWM0_A / PWM0_B — фаза A (TIM1_CH1/CH1N) | 1 |
| PA10 / PA11 | PWM1_A / PWM1_B — фаза B (TIM1_CH2/CH2N) | 1 |
| PA12 / PA13 | PWM2_A / PWM2_B — фаза C (TIM1_CH3/CH3N) | 1 |
| PA14 | **DShot/servo вход = TMR0_IO** | 1 |
| PB0 / PB1 / PB2 | ADC_CH0 напряжение / CH1 ток / CH2 температура | — |
| PB4 / PB5 / PB6 | BEMF компараторы A / B / C (GPIO edge IRQ) | — |
| PB9 | Телеметрия = UART0_TX | 1 |

Важно: у **PA5 нет CAP1/TMR** (только TDI/QEP_I), поэтому схема «ECAP на
сигнальном пине» с vk035 на vg5t невозможна в принципе. Ключ к «правильному»
DShot — что сигнальный пин = PA14 = TMR0_IO: захват и генерация GCR делаются
нативным TMR0 без всяких GPIO-DMA-хаков.

## 2. Распределение таймеров

| Блок | Роль |
|---|---|
| TMR0 | DShot: захват фронтов PA14 (CAPCOM[0], оба фронта) + генерация GCR (compare, OUTMODE=7) |
| TMR1 | COM-тайлер коммутации (IRQ по COUNT=PERIOD) |
| TMR2 | Zero-cross interval timer, свободный счёт (MODE=Multiple) |
| TMR3 | Блокирующая задержка delayMicros() |
| mtimer | 20 кГц тик управления (MTI, 4800 отсчётов @ 96 МГц) |

Клок TMR = SYSCLK (UM 14.1); делители времени `/50` vk035 (100 МГц) заменены
на `/(CPU_FREQUENCY_MHZ/2)` = /48.

## 3. Улучшения периферии по даташиту (вместо vk035-извращений)

1. **DShot RX на TMR0 CAPCOM + DMA** (`IO.c`, `peripherals.c`, `it.c`):
   каждый фронт PA14 → аппаратный захват COUNT в CAPCOM[0].VAL0 → DMA-запрос
   (DMA_RXIM.CAPCOM0_0, requestor 5) → канал DMA0 кладёт таймстампы в
   `dma_buffer[]`. По завершении буфера — IRQ → `transfercomplete()` +
   `processDshot()`. Никакого vk035-хака «GPIO-DMA читает TMR3» и никакого
   `reverseBuffer()` (TMR считает вверх — значения уже прямые).
2. **DShot TX (GCR-телеметрия) на TMR0** (`IO.c`): MODE=Up, PERIOD=periodTime,
   OUTMODE=7 (сброс по VAL0=0, установка по VAL1), DMA-канал 1 по запросу
   периода (DMA_TXIM.TMR) грузит следующее слово `gcr[]` в CAPCOM[0].VAL1 —
   полный аналог ECAP-APWM vk035, но нативно.
3. **Реальный контроллер DMA vg5t** (UM гл. 12): каналы 0/1 настраиваются через
   `STATIC0/1/4` (RD/WR_BURST_MAX=4, инкременты, RD_PER_NUM/WR_PER_NUM),
   дескрипторы в регистрах канала, IRQ по окончании буфера (CONFIG.CMD_SET_INT
   + INT_ENABLE.CH_END). vk035-структура `DMA->BASEPTR`/`DMA_CONFIGDATA`
   удалена полностью.
4. **ADC без NOP-цикла** (`K19xxVG5T_it.c`): секвенсорное прерывание приходит
   после ICNT+1 = 3 записей в FIFO (UM 16.2) — все три результата уже в FIFO,
   читаем ровно 3 значения. Хрупкий `for(nop)` из vk035 удалён.
   Триггер секвенсора: `EM0 = ADC_EMUX_EM0_PWM0A` (SOCA-импульс PWM0 при CTR=0),
   каналы RQ0/1/2 = CH0/CH1/CH2 = напряжение/ток/температура.
5. **20 кГц тик на машинном таймере** (`peripherals.c`, `it.c`): после сброса
   mtimer тикает от клока ядра (CLKSRC=0, DIVIDER=0, UM 10.1) = 96 МГц →
   период 4800. Диспетчеризация: `riscv_irq_init()` + обработчики
   `RISCV_IRQ_MTI` (тик) и `RISCV_IRQ_MEI` → `PLIC_MachHandler()`.
6. **PLIC вместо NVIC** (`K19xxVG5T_it.c`): для каждого источника обязательная
   настройка `SRC_MODE` (PLIC_SetMode, HILEVEL) + приоритет 1..7 (0 = запрещён,
   UM 11.5). Приоритеты: TMR1=7, GPIOB=6, ADC_SEQ0=4, DMA0/1=3, UART0_TD=1.
7. **SIU->CNTEN** (UM A.2, стр. 306): TMR/PWM не считают, пока не выставлены
   биты TMR0-3EN/PWM0-2EN. PWM0-2 стартуют одной записью CNTEN — аппаратная
   синхронизация трёх фаз без PWMSYNC.
8. **WDT**: клок выбирается явно через `WDTCFG.CLKSEL` (UM стр. 297), не
   полагаемся на дефолт.
9. **eeprom.c на plib5t FLASH** (`FLASH_ErasePage/WriteData/ReadData`,
   гранула 8 байт = MEM_FLASH_BUS_WIDTH_WORDS=2, страница 1 КБ, 512 КБ flash) —
   как в бутлоадере; адресация 0x0800xxxx работает (ADDR биты 31-19
   игнорируются, sdk-patches #6).

## 4. Чинил в SDK (dependencies) — по духу sdk-patches.md

| Файл | Что | Почему |
|---|---|---|
| `ldscripts/k1921vg5t_app.ld` | **новый**: ROM ORIGIN=0x1000 (512K-4K), RAM 16K, TCM 32K + `k1921vg5t_common.lds` | у vk035 есть K1921VK035_app.ld, у vg5t приложения не было — раньше firmware-скрипт ссылался на ARM-скрипт vk035 |
| `plib5t/src/plib5t_dma.c` | DMA_ChannelInit/DeInit переписаны под фактический `DMA_ChannelInit_TypeDef` (+ PER_NUM, инкременты) | .c был рассинхронизирован с .h (поля Mode/ArbitrationRate/TransfersTotal не существуют) — не компилировался бы |
| `plib5t/inc/plib5t_dma.h` | прототип DMA_ChannelDeInit поправлен под структуру конфига | то же |
| `plib5t/inc/plib5t_conf.h` | включён `plib5t_dma.h` | без него API DMA не виден (так же, как ранее включали plib5t_flash.h) |
| `plib5t/inc/plib5t.h` | `__STATIC_INLINE` = `static inline` (было `inline`) | C99 `inline` без static давал undefined reference на RCU_AHBRstCmd при -flto |
| `Device/.../source/plic.c` | удалён `supervisor_plic_handler[128]` | не используется (mach-only), −512 Б RAM |
| `cmake/toolchain/arm-none-eabi.cmake` | в ветке find_program выставляется `_GCC_EXE` | иначе gcc.cmake собирал пустой путь → локальная Windows-сборка vk035 падала |

## 5. Изменения в esc-firmware

- `inc/targets.h`: MCU-секции `MCU_K19XXVG5T` (и vk035) перенесены в начало
  файла — targets.h без include-guard, hardware-группа активируется только
  если ключ определён РАНЬШЕ (иначе inline-хелперы в IO.h не видят
  IC_TIMER_REGISTER). Добавлены `HARDWARE_GROUP_K19XXVG5T` (пинмап выше) и
  MCU-секция (CPU_FREQUENCY_MHZ 96, TMR1/2/3, APPLICATION_ADDRESS 0x1000,
  COM_TIMER_IRQ IsrVect_IRQ_TMR1 и т.д.).
- `mcu/k19xxvg5t/*`: переписаны peripherals.c / K19xxVG5T_it.c / IO.c /
  eeprom.c / serial_telemetry.c / phaseouts.c / comparator.c / ADC.c — все
  vk035-имена (PCLKCFG, HRSTCFG, ECAP1, INTERVAL_TIMER_EN, DENSET, INTENCLR_bit,
  NVIC_*, SysTick, MFLASH_*) заменены на vg5t (CGCFGAHB/CGCFGAPB/RSTDIS*,
  CAP1EN→не нужен, TMR CTRL/MODE/COUNT/PERIOD/IM, PLIC, mtimer, FLASH_*).
  GPIO: SET-регистры имеют битовые поля PINx, CLR-регистры — единое поле VAL
  (пишется маской), DENSET/DENCLR на vg5t отсутствуют.
- Ветви `K19XXVK035` в src/*.c (functions/signal/dshot/main, ntc_tables)
  расширены до `|| defined(K19XXVG5T)`; dshot-порог `halfpulsetime > 0x72`
  оставлен как у vk035 (96 vs 100 МГц — ~4%, уточнить на железе);
  `signal.c` деление `/100` заменено на `/CPU_FREQUENCY_MHZ` (для vk035
  эквивалентно).
- `inc/common.h`: шимы `__disable_irq/__enable_irq` (riscv-irq) и
  `NVIC_SystemReset()` (RCU->RSTSYS = KEY A55A | RSTEN).
- `CMakeLists.txt`: включены `HSECLK_VAL=16000000`, `SYSCLK_PLL`, `CKO_NONE`,
  добавлен `MTIME_FREQ_HZ=96000000ULL`. SystemInit приложение не зовёт — клоки
  настроены бутлоадером (как vk035 с `__NO_SYSTEM_INIT`), в main() остаётся
  `SystemCoreClockUpdate()`.
- `mcu/k1921vg5t/` удалён (устаревший дубликат k19xxvg5t).
- `.github/workflows/build.yml`: matrix-запись K1921VG5T (build-riscv) +
  xpack `riscv-none-elf-gcc` 14.2.0 (нативный Linux, без Wine; GCC 8.2 embed
  не имеет rv32imafdc/ilp32d) + выгрузка артефактов в release.
- esc-bootloader: `firmware/vg5t/init.h` — сигнальный пин PA5 → **PA14**.

## 6. Результаты сборки

```
esc-firmware-vg5t: text 18008, data 204, bss 3690  (ROM 18.5К / 508К, RAM ~3.9К + стек 2К / 16К)
  .text.startup (_start) = 0x1000, .eeprom = 0xF000, .device_type = 0xEFE0, .calibration = 0xF800
esc-firmware-vk035: MFLASH 20028/64K, RAM 8708/16K — регрессии нет
esc-bootloader vg5t: собирается после смены пина на PA14
```

Тулчейн: xpack riscv-none-elf-gcc 14.2.0 (C:\riscvtool14) и GCC12 из
dependencies — оба с `rv32imafdc/ilp32d`; локально достаточно `RISCV_TOOLCHAIN_DIR`.
vk035 локально требует xpack arm 14.2.1 в PATH (системный 10.3.1 не понимает
`--no-warn-rwx-segments`).

## 7. Что проверить на железе (bring-up)

1. **mtimer по адресу 0x2040000/0x20BFF8** (CLINT-раскладка из mtimer.h) — если
   mtime не тикает, тик не пойдёт; проверить нарастание `mtimer_get_raw_time()`.
2. **NDTL DMA — «количество передач»** (TOC UM) vs «байты» (комментарий
   plib5t): в коде передано число слов (buffersize / 22). Если DMA переносит
   в 4 раза меньше данных — интерпретировать как байты и умножить на 4.
3. **Захват DShot**: CAPMODE=3 (оба фронта), DMA request на CAPCOM0_0; на
   DShot300 фронт каждые ~833 нс — latency DMA (~100-200 нс) с запасом.
4. Полярность GCR-выхода (OUTMODE=7: high = PERIOD-VAL1, эквивалент APWMPOL=1
   vk035) и поведение при VAL1=0 (так же неоднозначно, как CMP=0 у ECAP).
5. `halfpulsetime > 0x72` порог и делитель `/CPU_FREQUENCY_MHZ` в
   computeServoInput — подстроить по факту.
6. WDT: сейчас не включается (как на vk035); MX_IWDG_Init настроен под HSI 4 МГц
   (LOAD=200000 → 50 мс) — при включении пересчитать под задачу.
7. Потенциал на будущее:_hw trip-zone PWM_TZ (PA1) от компаратора тока,
   фильтры входов QUALSET/QUALSAMPLE на BEMF, high-resolution PWM (HRCTL),
   CAP0/CAP2 свободны (PA7/B7).
