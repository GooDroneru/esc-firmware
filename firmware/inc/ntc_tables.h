#ifdef USE_NTC

  #if defined(K19xxVK035) || defined(WCH)
  // NTC 10k B=3380, R_series=10k to VCC, ADC on NTC-GND junction
  // Values in degrees Celsius. Index = ADC >> 6, interpolated by lower 6 bits.
  int NTC_table[65] = {
    846, 197, 155, 133, 119, 108, 100,
    93,  87,  82,  77,  73,  69,  66,
    63,  60,  57,  54,  52,  50,  47,
    45,  43,  41,  39,  37,  35,  34,
    32,  30,  28,  27,  25,  23,  22,
    20,  19,  17,  15,  14,  12,  11,
      9,   7,   6,   4,   2,   0,  -1,
    -3,  -5,  -7,  -9, -11, -14, -16,
    -19, -22, -25, -28, -33, -38, -44,
    -55, -101
  };
  #endif
#endif

