#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include "main.h"
#include "config.h"
#include "ini.h"

const char *default_servername = "localhost";
const char *default_administrator = "root@localhost";

config_server* config_server_new() {
    config_server* config = calloc(1, sizeof(config_server));
    config->host_count = 0;
    config->listen_port = 80;
    
    config->servername = calloc(128, sizeof(char));
    if (gethostname(config->servername, 128) < 0) {
        warning(true, "failed to get server hostname");
        free(config->servername);
        config->servername = strdup(default_servername);
    }
    config->servername = realloc(config->servername, (strlen(config->servername)+1)*sizeof(char));
    
    config->administrator = strdup(default_servername);
    
    return config;
}
void config_server_addhost(config_server *config, config_host *host) {
    if (config->host_count == 0) {
        config->hosts = calloc(1, sizeof(config_server*));
        config->host_count++;
    } else {
        config->hosts = realloc(config->hosts, ++config->host_count * sizeof(config_server*));
    }
    config->hosts[config->host_count-1] = host;
}
config_host* config_server_gethost(config_server *config, char *name) {
    config_host *defaulthost=NULL;
    config_host *host=NULL;
    CONFIG_SERVER_FOREACH_HOST(config, host) {
        if (name != NULL && strcasecmp(name, host->hostname) == 0) {
            return host;
        }
        if (host->default_host == true) {
            defaulthost = host;
        }
    }
    return defaulthost;
}
void config_server_delete(config_server* config) {
    if (config->administrator != NULL) free(config->administrator);
    if (config->servername != NULL)    free(config->servername);
    config_host *host;
    CONFIG_SERVER_FOREACH_HOST(config, host) {
        config_host_delete(host);
    }
    if (config->hosts != NULL) free(config->hosts);
    free(config);
}

config_host* config_host_new() {
    config_host *host = calloc(1, sizeof(config_host));
    host->default_host = false;
    host->enabled = true;
    host->hostname = NULL;
    host->serve_dir = NULL;
    host->dir_listings = true;
    host->index_files = calloc(1, sizeof(char*));
    host->index_files[0] = strdup("index.html");
    host->index_files_count = 1;
    
    return host;
}
void config_host_delete(config_host *host) {
    if (host->hostname != NULL)  free(host->hostname);
    if (host->serve_dir != NULL) free(host->serve_dir);
    for(int i=0; i<host->index_files_count; i++) {
        free(host->index_files[i]);
    }
    free(host->index_files);
    free(host);
}

static int config_read_ini_cb(void* _config, const char* section, const char* name, 
        const char* value) {
    config_server *config = (config_server*)_config;
    static config_host *host = NULL;
    
#define MATCH(s, n) strcasecmp(s, section) == 0 && strcasecmp(n, name) == 0
    
    if (name == NULL && strcasecmp(section, "Host") == 0) {
        host = config_host_new();
        config_server_addhost(config, host);
    } else if (strcasecmp("Host", section) == 0 && host == NULL) {
        return -1;
    } else if (name != NULL) {
        if (MATCH("Server", "name")) {
            config->servername = realloc(config->servername, strlen(value)+1);
            strcpy(config->servername, value);
        } else if (MATCH("Server", "admin")) {
            config->administrator = realloc(config->administrator, strlen(value)+1);
            strcpy(config->administrator, value);
        } else if (MATCH("Server", "listen")) {
            errno = 0;
            config->listen_port = (uint16_t)strtol(value, NULL, 10);
            if (errno != 0) {
                warning(true, "Config: Invalid port number for [Server]listen");
            }
            return -1;
        } else if (MATCH("Host", "name")) {
            host->hostname = realloc(host->hostname, strlen(value)+1);
            strcpy(host->hostname, value);
        } else if (MATCH("Host", "enabled")) {
            if (strcasecmp(value, "yes") == 0) {
                host->enabled = true;
            } else if (strcasecmp(value, "no") == 0) {
                host->enabled = false;
            }
        } else if (MATCH("Host", "default")) {
            
            if (strcasecmp(value, "yes") == 0) {
                //Ensure there is only one default host
                config_host *tmp;
                CONFIG_SERVER_FOREACH_HOST(config, tmp) {
                    tmp->default_host = false;
                }
                host->default_host = true;
            } else if (strcasecmp(value, "no") == 0) {
                host->default_host = false;
            }
        } else if (MATCH("Host", "serve")) {
            char* serve_dir = realpath(value, NULL);
            if (serve_dir == NULL) {
                warning(true, "Config: host serve directory is invalid");
                return -1;
            }
            DIR* dir = opendir(serve_dir);
            if (dir == NULL) {
                free(serve_dir);
                warning(true, "Config: host serve directory is invalid");
                return -1;
            }
            closedir(dir);
            if (host->serve_dir != NULL) free(host->serve_dir);
            host->serve_dir = serve_dir;
        } else if (MATCH("Host", "index_files")) {
            for(int i=0;i<host->index_files_count;i++) {
                free(host->index_files[i]);
            }
            free(host->index_files);
            host->index_files = NULL;
            host->index_files_count = 0;
            
            char* savepos=NULL;
            char* value_cpy = strdup(value);
            char* file = strtok_r(value_cpy, ",", &savepos);
            while(file != NULL) {
                host->index_files = realloc(host->index_files, sizeof(char*)*(host->index_files_count+1));
                host->index_files[host->index_files_count++] = strdup(str_trimwhitespace(file));
                file = strtok_r(NULL, ",", &savepos);
            }
            free(value_cpy);
        } else if (MATCH("Host", "dir_listings")) {
            if (strcasecmp(value, "yes") == 0) {
                host->dir_listings = true;
            } else if (strcasecmp(value, "no") == 0) {
                host->dir_listings = false;
            } else {
                warning(false, "Unexpected value for [Host]dir_listings");
            }
        }
    }
#undef MATCH
    
    return 1;
}

int config_read_ini(const char* filename, config_server *config) {
    return ini_parse(filename, config_read_ini_cb, config);
}