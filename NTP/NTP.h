/*
 * NTP.h
 *
 *  Created on: Mar 13, 2013
 *      Author: root
 */

#ifndef NTP_H_
#define NTP_H_

extern int Create_NTP_Thread();
/* when present, debug is a true global */
#ifdef ENABLE_DEBUG
extern int debug;
#else
#define debug 1
#endif

#endif /* NTP_H_ */
