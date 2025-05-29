#ifndef TRIODEGIRLDAC_H_
#define TRIODEGIRLDAC_H_

#include <stdint.h>
#include <math.h>

class TriodeGirlDAC {
public:
  void writeDAC(uint16_t value);
  void setup_dac(void);
};
#endif  // TRIODEGIRLDAC_H_