/*
 * Timer code for Watcom and similar
 */

#include <i86.h>

void timer_milisleep(int x) {
  delay(x);
}
