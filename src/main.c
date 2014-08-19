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
#include <bits/stdio2.h>

#include "util.h"
#include "main.h"
#include "server.h"
#include "server-state.h"

int main(int argc, char** argv) {
    
    //Load the config
    config_server *config = config_server_new();
    if (config_read_ini(DEFAULT_CONFIG_FILE, config) < 0) {
        fatal("Could not read config");
    }
    
    server_state *state = server_status_new(config);
    
    //Run the server
    server_start(state);
    
    return (EXIT_SUCCESS);
}