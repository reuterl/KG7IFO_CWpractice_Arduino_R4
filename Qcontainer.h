#ifndef QCONTAINER_H_
#define QCONTAINER_H_

#include <queue>

typedef struct _Qcontainer{
	std::queue<uint8_t> ByteStream;
	std::queue<uint8_t> XmitStream;
	std::queue<void *> MsgQ;
} Qcontainer;
#endif