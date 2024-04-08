#define _DEFAULT_SOURCE

#include "mqtt_entry.h"
#include "mqtt_arg_parse.h"
#include "mqtt_uci_utils.h"
#include "mqtt_make_daemon.h"
#include "mqtt_utils.h"

#include <mosquitto.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>

volatile sig_atomic_t running = 1;
struct mosquitto* mqtt = NULL;

void sig_handler(sig_atomic_t signum)
{
	running = 0;
	mosquitto_disconnect(mqtt);
}

int entry(int argc, char** argv)
{
	openlog("mqtt_sub", LOG_PID | LOG_NDELAY, LOG_INFO);
	
	struct sigaction sa = { .sa_handler = sig_handler };
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGQUIT, &sa, 0);

	struct arguments args = parse_subscriber_args(argc, argv);
	uci_read_topics(&args);

	if (args.topics == NULL){
		syslog(LOG_ERR, "No topics to subscribe to.");
		return -1;
	}

    int res = 0;
    if (args.is_daemon)
        res = make_daemon(0);
    if (res != 0){
		free_topics(&args);
		return -1;
	}
    
	mosquitto_lib_init();
	
	mqtt = mqtt_init(&args);

	if (mqtt == NULL) goto end;

	mosquitto_loop_forever(mqtt, -1, 1);

	end:
		mosquitto_disconnect(mqtt);
		mosquitto_destroy(mqtt);
		mosquitto_lib_cleanup();
		free_topics(&args);
		return res;
}