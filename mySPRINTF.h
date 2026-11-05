#ifndef MYSPRINTF_H_
#define MYSPRINTF_H_

#define SPRINTF(FMT, ...) \
  sprintf(sSprintf, FMT, ##__VA_ARGS__); \
  Serial.print(sSprintf);

extern char sSprintf[];

#endif