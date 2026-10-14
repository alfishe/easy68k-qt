#pragma once
// Borland <dos.h> subset used by Sim68K (Trap #15 task 8). The oracle
// reports midnight so traces are deterministic.
struct time {
  unsigned char ti_min, ti_hour, ti_hund, ti_sec;
};
inline void gettime(struct time* t) {
  t->ti_min = t->ti_hour = t->ti_hund = t->ti_sec = 0;
}
