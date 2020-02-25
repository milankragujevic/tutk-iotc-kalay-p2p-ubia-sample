/*
 * BellService.h
 *
 *  Created on: Nov 27, 2013
 *      Author: root
 */

#ifndef BELLSERVICE_H_
#define BELLSERVICE_H_


enum {
	BELL_SERVICE_BUTTON_DOWN,
	BELL_SERVICE_INFRARED_ACT,
};

extern int BellServiceThreadStart();
#endif /* BELLSERVICE_H_ */
