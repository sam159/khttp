/* 
 * File:   config.h
 * Author: sam
 *
 * Created on 27 July 2014, 18:47
 */

#ifndef CONFIG_H
#define	CONFIG_H


#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include <stdbool.h>
    
#define CONFIG_SERVER_FOREACH_HOST(config, elem)                    \
    elem = config->hosts != NULL ? config->hosts[0] : NULL;         \
    for(int i=0; i < config->host_count; elem=config->hosts[i++])
    
    typedef struct config_host {
        char *hostname;
        bool default_host;
        bool enabled;
        char *serve_dir;
        char**index_files;
        size_t index_files_count;
        bool dir_listings;
    } config_host;
    
    typedef struct config_server {
        char *servername;
        char *administrator;
        uint16_t listen_port;
        config_host **hosts;
        size_t host_count;
    } config_server;
    
    config_server* config_server_new();
    void config_server_addhost(config_server *config, config_host *host);
    config_host* config_server_gethost(config_server *config, char* name);
    void config_server_delete(config_server* config);
    
    config_host* config_host_new();
    void config_host_delete(config_host *host);
    
    int config_read_ini(const char* filename, config_server *config);

#ifdef	__cplusplus
}
#endif

#endif	/* CONFIG_H */

