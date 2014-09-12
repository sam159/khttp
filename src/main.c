/* 
 * File:   main.c
 * Author: sam
 *
 * Created on 16 July 2014, 20:05
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#include "util.h"
#include "log.h"
#include "main.h"
#include "server.h"
#include "server-state.h"

static server_state *current_state = NULL;

static void signal_handle(int sig) {
    if (current_state != NULL) {
        current_state->shutdown_requested = true;
    }
}

int main(int argc, char** argv) {
    
    //Load the config
    config_server *config = config_server_new();
    if (config_read_ini(DEFAULT_CONFIG_FILE, config) < 0) {
        fatal("Could not read config");
    }
    
    server_state *state = server_status_new(config);
    
    current_state = state;
    char sig_error_buf[128];
    if (signal(SIGINT, signal_handle) == SIG_ERR) {
        char *errstr = strerror_r(errno, sig_error_buf, 127);
        LOG(LERROR, "Failed to attach signal handler to SIGINT: %s", errstr);
    }
    if (signal(SIGTERM, signal_handle) == SIG_ERR) {
        char *errstr = strerror_r(errno, sig_error_buf, 127);
        LOG(LERROR, "Failed to attach signal handler to SIGTERM: %s", errstr);
    }
    
    //Run the server
    server_start(state);
    
    current_state = NULL;
    server_status_delete(state);
    
    return (EXIT_SUCCESS);
}