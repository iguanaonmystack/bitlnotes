/* moo.c - free cowbot for bitlbee! */

#include <bitlbee.h>

static void cmd_moo(irc_t *irc, char **cmd) {
    irc_usermsg(irc, "%s", cmd[0]);
}


void init_plugin(void) {
    root_command_add("moo", 0, cmd_moo, 0);
}

