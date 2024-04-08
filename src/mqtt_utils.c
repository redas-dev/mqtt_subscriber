#define _DEFAULT_SOURCE
#include <mosquitto.h>
#include "mqtt_arg_parse.h"
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#include "mqtt_uci_utils.h"

void on_message(struct mosquitto* mqtt, void* user_data, const struct mosquitto_message* msg)
{
    cJSON* json = cJSON_Parse(msg->payload);
    if (json == NULL)
    {
        syslog(LOG_ERR, "JSON is invalid");
        return;
    }

    if(cJSON_IsInvalid(json)){
        cJSON_Delete(json);
        syslog(LOG_ERR, "Bad input format");
        return;
    }

    cJSON* data = cJSON_GetObjectItem(json, "data");

    if (data == NULL) {
        syslog(LOG_ERR, "Missing data object");
        cJSON_Delete(json);
        return;
    }

    struct mqtt_events* events = find_topic_events(msg->topic);

    if (events == NULL){
        syslog(LOG_INFO, "No events");
        cJSON_Delete(json);
        return;
    }

    check_events(events, data);
    free_events(events);
    cJSON_Delete(json);
}

void on_connect(struct mosquitto *mqtt, void *obj, int reason_code)
{
    int rc;
    struct arguments* args = (struct arguments*)obj;

	syslog(LOG_INFO, "on_connect: %s\n", mosquitto_connack_string(reason_code));
	if(reason_code != 0){
		mosquitto_disconnect(mqtt);
	}

	rc = mosquitto_subscribe_multiple(mqtt, NULL, args->topic_count, args->topics, 1, 0, NULL);
	if(rc != MOSQ_ERR_SUCCESS){
		syslog(LOG_ERR, "Error subscribing: %s\n", mosquitto_strerror(rc));
		mosquitto_disconnect(mqtt);
	}
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	bool have_subscription = false;

	for(int i = 0; i < qos_count; i++){
		syslog(LOG_INFO, "on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
		if(granted_qos[i] <= 2){
			have_subscription = true;
		}
	}

	if(have_subscription == false){
		syslog(LOG_ERR, "Error: All subscriptions rejected.\n");
		mosquitto_disconnect(mosq);
	}
}

struct mosquitto* mqtt_init(struct arguments* args)
{
    struct mosquitto* mqtt = NULL;
    int res = 3;

    mqtt = mosquitto_new(NULL, true, args);

    res = mosquitto_tls_set(mqtt, args->cafile, NULL, args->certfile, args->keyfile, NULL);

    if (res == MOSQ_ERR_INVAL){
        syslog(LOG_INFO, "Not using TLS because it was not set or had invalid arguments.");
    }

    res = mosquitto_username_pw_set(mqtt, args->username, args->password);

    if (res == MOSQ_ERR_INVAL){
        syslog(LOG_INFO, "Not using username and password because either one or both were invalid or they were not set.");
    }

    mosquitto_connect_callback_set(mqtt, on_connect);
    mosquitto_message_callback_set(mqtt, on_message);
    mosquitto_subscribe_callback_set(mqtt, on_subscribe);

    res = mosquitto_connect(mqtt, args->host, args->port, 60);

    if (res != MOSQ_ERR_SUCCESS){
        char buffer[200] = {0};
        strerror_r(errno, buffer, sizeof(buffer));

        syslog(LOG_ERR, "Error: %s", buffer);
        syslog(LOG_ERR, "Failed to connect to broker %s:%d", args->host, args->port);
        mosquitto_disconnect(mqtt);
        mosquitto_destroy(mqtt);
        mqtt = NULL;
    } else {
        syslog(LOG_INFO, "Connected to broker %s:%d", args->host, args->port);
    }

    return mqtt;
}
