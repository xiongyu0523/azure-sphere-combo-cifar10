#ifndef TESTIO_H
#define TESTIO_H

#include <stdint.h>

#define TESTIO_0_MASK	(1 << 0)
#define TESTIO_1_MASK	(1 << 1)
#define TESTIO_2_MASK	(1 << 2)
#define TESTIO_3_MASK	(1 << 3)

void testio_init(uint8_t bit_mask);
void testio_set_high(uint8_t index);
void testio_set_low(uint8_t index);

#endif
