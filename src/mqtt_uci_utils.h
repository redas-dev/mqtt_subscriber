#include <cjson/cJSON.h>

#ifndef _MQTT_UCI_UTILS_H
#define _MQTT_UCI_UTILS_H
    struct mqtt_event {
        char* topic;
        char* event_name;
        char* param_name;
        char* value_type;
        int compare_type;
        char* value;
        char* email;
        char* password;
        char* username;
        char* recipient;
    };

    struct mqtt_events {
        struct mqtt_event** events;
        unsigned int count;
    };

    struct mqtt_compare {
        char* string_val;
        char* string_ref_val;
        double double_val;
        double double_ref_val;
        int comp_type;
        char* val_type;
    };

    void uci_read_topics(struct arguments* arguments);
    struct mqtt_events* find_topic_events(char* topic);
    void check_events(struct mqtt_events* events, cJSON* data);
    void free_events(struct mqtt_events* events);
    int evaluate_numeric(struct mqtt_compare* compare);
    int evaluate_alphanumeric(struct mqtt_compare* compare);

    // Numbers are the int representation of the char sum of each sign. E.g. == is 61 + 61, and >= is 61 + 62 etc.
    enum { EQUAL = 122, NOT_EQUAL = 94, GREATER = 62, LESS = 60, GREATER_EQUAL = 123, LESS_EQUAL = 121 };
#endif