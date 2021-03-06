#ifndef XBOXRF_H_
#define XBOXRF_H_

#if defined(__cplusplus)
extern "C"
{
#endif

void rf360_init(int data_pin, int clock_pin);
void rf360_updateled(int controller, int8_t connected);

#if defined(__cplusplus)
}
#endif

#endif /* XBOXRF_H_ */