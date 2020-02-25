/*
 * adapter_httpserver.h
 *
 *  Created on: Apr 8, 2013
 *      Author: root
 */

#ifndef ADAPTER_UPGRADE_H_
#define ADAPTER_UPGRADE_H_

#include "common/utils.h"

extern void Upgrade_cmd_table(msg_container *self);
extern void UpgradeStart(msg_container self);
extern void UpgradeEnd(msg_container *self);
extern void UpgradeGetUpdateInfo(msg_container self);
extern void retUpgradeGetUpdateInfo(msg_container *self);
extern void UpgradeDownLoadBlock(msg_container self);
extern void retUpgradeSeverLoginCam(msg_container *self);
extern void UpgradeState(msg_container self);
extern void retUpgradeState(msg_container *self);
extern void retUpgradeEnd(msg_container self);
extern void retUpgradeStart(msg_container *self);

#endif /* ADAPTER_HTTPSERVER_H_ */
