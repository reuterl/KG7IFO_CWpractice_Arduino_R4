/*
 * machinedetat.h
 *
 *  Created on: Apr 18, 2023
 *      Author: luke
 */

#ifndef MACHINEDETAT_H_
#define MACHINEDETAT_H_
#include <stdint.h>
#include <Qcontainer.h>

class machinedetat {
public:
	machinedetat(Qcontainer * _Queues);
	virtual ~machinedetat();
	void msgParser(uint8_t byte);

private:
	typedef enum {st_sea=0, st_q, st_len, st_fill} State_t;

	State_t state;
	uint16_t length;
	uint16_t remaining;
	uint8_t msg[256];
	uint16_t fillPtr;
  Qcontainer * Queues;
	void initMsg(uint8_t length);
  char sSprintf[64];
};

#endif /* MACHINEDETAT_H_ */
