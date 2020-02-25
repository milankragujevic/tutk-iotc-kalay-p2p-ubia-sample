/*
 * control_channel.h
 *
 *  Created on: May 14, 2013
 *      Author: root
 */

#ifndef CONTROL_CHANNEL_H_
#define CONTROL_CHANNEL_H_

int tutk_send_open_channel_msg(int rdt_id, char *msg, int ch);
int tutk_send_close_channel_msg(int rdt_id, char *msg, int ch);
int tutk_send_channel_msg_rep(int rdt_id, char *msg, int ch);
int tutk_send_heart_msg(int rdt_id, char *msg);
int tutk_send_audio_dir_req(int rdt_id, char *msg, int dir);
int tutk_send_https_check_msg(void);
int send_nightmode_req_msg(int rdt_id, int state, int time);
int tutk_send_set_nightmode_command(int state, int time);
int send_wifi_signal_msg(int rdt_id);

#endif /* CONTROL_CHANNEL_H_ */
