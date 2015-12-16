#ifndef INCLUDE_MY_STUFF_H_
#define INCLUDE_MY_STUFF_H_

#define cli() __asm__("rsil a2, 15")
#define sei() __asm__("rsil a2, 0")

#endif /* INCLUDE_MY_STUFF_H_ */
