/*
 * Timer code for POSIX and friends
 */

#include <unistd.h>

void timer_milisleep(int x) {
  usleep(x * 1000);
}
