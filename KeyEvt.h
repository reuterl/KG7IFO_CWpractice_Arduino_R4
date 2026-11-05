#ifndef KEYEVT_H_
#define KEYEVT_H_

typedef enum keyEdgePos { EdgeUp = 11,
                          EdgeDn,
                          EdgeNull } e_keyEdgePos;
typedef struct keyEvt {
  e_keyEdgePos evt;
  uint32_t evtTime;
} st_keyEvt;

#endif