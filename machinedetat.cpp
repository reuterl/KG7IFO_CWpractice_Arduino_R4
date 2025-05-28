/*
 * machinedetat.cpp
 *
 *  Created on: Apr 18, 2023
 *      Author: luke
 */
#include <Serial.h>
#include <stdio.h>

#include "msggenerator.h"
#include "ProcessCommands.h"
#include "machinedetat.h"

machinedetat::machinedetat(Qcontainer * _Queues) {
	// TODO Auto-generated constructor stub
	state = st_sea;
	Queues = _Queues;
}

machinedetat::~machinedetat() {
	// TODO Auto-generated destructor stub

}

void machinedetat::initMsg(uint8_t length){
	msg[0] = 'C';
	msg[1] = 'Q';
	msg[2] = length;
}

void machinedetat::msgParser(uint8_t byte) {
	uint16_t calcChecksum, msgChecksum;
	uint16_t idx;
  MsgUtil msgUtil;
  //sprintf(sSprintf, "msgParser(0x%02X)\n", byte);
  //Serial.print(sSprintf);
	switch (state) {
	case st_sea:
		if (byte == 'C') {
			state = st_q;
		} else {
			state = st_sea;
		}
		break;
	case st_q:
		if (byte == 'Q') {
			state = st_len;
		} else {
			state = st_sea;
		}
		break;
	case st_len:
		length = byte;
		if (length > sizeof(msg)) {
			state = st_sea;
      sprintf(sSprintf,"\r\n!! Found message too long = %d\r\n",length);
      Serial.print(sSprintf);
			//Serial.print("\r\n!! Found message too long = %d\r\n");//, length);
		} else {
			initMsg(length);
			remaining = length - 3;
			fillPtr = 3;
			state = st_fill;
		}
		break;
	case st_fill:
		remaining--;
		msg[fillPtr++] = byte;
		if (remaining == 0) {
			calcChecksum = 0;
			for (idx = 0; idx < length - 2; idx++) {
				calcChecksum += (uint16_t) msg[idx];
			}
			msgChecksum = msg[length - 1];
			msgChecksum |= msg[length - 2] << 8;
			if ((msgChecksum == calcChecksum)) {
				//Serial.print("\r\nFound valid message!\r\n");
				//Serial.print("  [ ");
				//for (idx = 0; idx < length; idx++) {
        //  sprintf(sSprintf,"%02X ",msg[idx]);
        //  Serial.print(sSprintf);
					//Serial.print("%02X ");//, msg[idx]);
				//}
				//Serial.print("]\r\n");
        msgUtil.dsplMsg(msg);

				ProcessCQcmmd(msg);
			}
			state = st_sea;
		} else {
			state = st_fill;
		}
		break;
	default:
		Serial.print("\nDefault State!\n");
		break;
	}

}
