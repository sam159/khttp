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

#include "http_parser.h"

#include "ut/utlist.h"
#include "ut/utarray.h"
#include "main.h"
#include "socket.h"
#include "http.h"
#include "http-reader.h"
#include "config.h"
#include "http-server.h"
#include "mime.h"
#include "queue.h"
#include "thread-pool.h"
#include "log.h"
#include "main-loop.h"

int serverfd = 0;
volatile static bool stop = false;

static void signal_int(int signum) {
    fprintf(stderr, "Terminating...\n");
    stop = true;
}

int main(int argc, char** argv) {
    
    hmain_status *status = calloc(1, sizeof(hmain_status));
    hmain_setup(status);
    
    hmain_loop(status);
    
    hmain_teardown(status);
    
    return (EXIT_SUCCESS);
}