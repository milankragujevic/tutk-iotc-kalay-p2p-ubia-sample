#include "common/utils.h"
#include "Tooling/ToolingAdapter.h"
#include "common/common.h"
#include "common/msg_queue.h"

void Tooling_start(msg_container *self);
void Tooling_stop(msg_container *self);

cmd_item tooling_cmd_table[] = {
		{1, Tooling_start},
		{2, Tooling_stop},
};

void tooling_cmd_table_fun(msg_container *self) {
	MsgData *msgrcv = (MsgData *)self->msgdata_rcv;
	cmd_item icmd;
	icmd.cmd = msgrcv->cmd;
	msg_fun fun = utils_cmd_bsearch(&icmd, tooling_cmd_table, sizeof(tooling_cmd_table)/sizeof(cmd_item));
	if(fun != NULL) {
		fun(self);
	}
}

void Tooling_start(msg_container *self) {
	printf("start \n");
}

void Tooling_stop(msg_container *self) {
	printf("stop \n");
}
