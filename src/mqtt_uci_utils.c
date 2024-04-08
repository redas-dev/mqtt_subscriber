#define _DEFAULT_SOURCE
#include <uci.h>
#include <ucimap.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <cjson/cJSON.h>
#include <regex.h>

#include "mqtt_arg_parse.h"
#include "mqtt_uci_utils.h"
#include "mqtt_curl_utils.h"

// Change compare type from string to int
int comp_type_as_int(const char* comp_type)
{
    if (comp_type == NULL) return -1;

    int res = 0;
    for(int i = 0; i < strlen(comp_type); i++)
        res += (int)comp_type[i];
    return res;
}

void uci_read_topics(struct arguments* arguments)
{
    struct uci_context* ctx;
    struct uci_package* pkg;
    struct uci_element* elem;
    struct uci_ptr ptr;

    ctx = uci_alloc_context();

    int res = uci_load(ctx, "mqtt_sub", &pkg);

    if (res != 0){
        syslog(LOG_INFO, "Failed to load config");
        uci_free_context(ctx);
        return;
    }
    
    struct uci_element* i;
    uci_foreach_element(&pkg->sections, i)  // Loop through sections
    {
        struct uci_section* section = uci_to_section(i);

        if (strcmp(section->e.name, "topics") == 0){
            struct uci_element* j;
            uci_foreach_element(&section->options, j)   // Loop through options
            {
                struct uci_option* option = uci_to_option(j);
                
                if (option->type == UCI_TYPE_LIST)
                {
                    struct uci_element* k;
                    uci_foreach_element(&option->v.list, k) // Loop through list option values
                    {
                        if (topic_exists(k->name, arguments->topics, arguments->topic_count)) break;
                        arguments->topic_count++;
                        char** temp = (char**)realloc(arguments->topics, (arguments->topic_count + 1) * sizeof(char*));
                        if (temp == NULL) { arguments->topic_count--; break; }
                        arguments->topics = temp;
                        arguments->topics[arguments->topic_count - 1] = strdup(k->name);
                    }
                }
            }
        }
    }

    uci_free_context(ctx);
}

// Check if the given value is not empty
bool check_uci_value_valid(const char* value, char** event_arg)
{
    if (value == NULL || strcmp(value, "") == 0){
        syslog(LOG_INFO, "Invalid uci config value.");
        return false;
    }

    *event_arg = strdup(value);
    return true;
}

bool email_valid(const char* email)
{
    regex_t regex;
    int reti = regcomp(&regex, "[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,4}", REG_EXTENDED);

    if (reti){
        syslog(LOG_ERR, "Couldn't compile email regex");
        regfree(&regex);
        return false;
    }

    reti = regexec(&regex, email, 0, NULL, 0);
    if (!reti){
        syslog(LOG_ERR, "Email address isn't valid: %s", email);
    }
    
    regfree(&regex);

    return reti == 0;
}

// Find all the events that the specified topic has
struct mqtt_events* find_topic_events(char* topic)
{
    struct uci_context* ctx;
    struct uci_package* pkg;
    struct uci_element* elem;

    ctx = uci_alloc_context();

    int res = uci_load(ctx, "mqtt_sub", &pkg);

    if (res != 0){
        syslog(LOG_INFO, "Failed to load config");
        uci_free_context(ctx);
        return NULL;
    }

    struct mqtt_events* events = (struct mqtt_events*)malloc(sizeof(struct mqtt_events));
    events->events = NULL;
    events->count = 0;

    // Go through all events and get their information
    uci_foreach_element(&pkg->sections, elem)
    {
        struct uci_section* s = uci_to_section(elem);

        if (strcmp(s->type, "event") == 0) {
            const char* uci_topic = uci_lookup_option_string(ctx, s, "topic");
            if (uci_topic == NULL) continue;

            if (strcmp(topic, uci_topic) != 0) continue;
            
            struct mqtt_event* event = (struct mqtt_event*)malloc(sizeof(struct mqtt_event));

            int res = 0;

            const char* comp_type = uci_lookup_option_string(ctx, s, "compare_type");
            if (comp_type == NULL) { res = -1; goto end; }
            event->compare_type = comp_type_as_int(comp_type);

            const char* val_type = uci_lookup_option_string(ctx, s, "value_type");
            if (!check_uci_value_valid(val_type, &(event->value_type))) { res = -1; goto end; }

            const char* email = uci_lookup_option_string(ctx, s, "email");
            if (!email_valid(email) || !check_uci_value_valid(email, &(event->email))) { res = -1; goto end_email; }

            const char* recipient = uci_lookup_option_string(ctx, s, "recipient");
            if (!email_valid(recipient) || !check_uci_value_valid(recipient, &(event->recipient))) { res = -1; goto end_recip; }

            const char* param_name = uci_lookup_option_string(ctx, s, "param_name");
            if (!check_uci_value_valid(param_name, &(event->param_name))) { res = -1; goto end_param; }

            const char* value = uci_lookup_option_string(ctx, s, "value");
            if (!check_uci_value_valid(value, &(event->value))) { res = -1; goto end_val; }

            const char* username = uci_lookup_option_string(ctx, s, "username");
            if (!check_uci_value_valid(username, &(event->username))) { res = -1; goto end_username; }

            const char* password = uci_lookup_option_string(ctx, s, "password");
            if (!check_uci_value_valid(password, &(event->password))) { res = -1; goto end_pass; }

            const char* event_name = uci_lookup_option_string(ctx, s, "event_name");
            if (!check_uci_value_valid(event_name, &(event->event_name))){
                event->event_name = " ";
            }

            event->topic = strdup(uci_topic);

            events->count++;

            if (events->events == NULL){
                events->events = (struct mqtt_event**)malloc(events->count * sizeof(struct mqtt_event*));
            }
            else 
                events->events = (struct mqtt_event**)realloc(events->events, events->count * sizeof(struct mqtt_event*));
            events->events[events->count - 1] = event;

            if (res != 0){
                end_pass:
                    free(event->username);
                end_username:
                    free(event->value);
                end_val:
                    free(event->param_name);
                end_param:
                    free(event->recipient);
                end_recip:
                    free(event->email);
                end_email:
                    free(event->value_type);
                end:
                    free(event);
            }
        }
    }
    
    uci_free_context(ctx);

    return events;
}

// Check if specified data triggers an event
void check_events(struct mqtt_events* events, cJSON* data)
{
    for(int i = 0; i < events->count; i++)
    {
        struct mqtt_compare* compare = (struct mqtt_compare*)malloc(sizeof(struct mqtt_compare));
        compare->comp_type = 0;
        compare->double_ref_val = 0;
        compare->double_val = 0;
        compare->string_ref_val = "";
        compare->string_val = "";
        compare->val_type = "";
        
        compare->comp_type = events->events[i]->compare_type;
        compare->val_type = events->events[i]->value_type;

        int res = 1;

        if (strcmp(compare->val_type, "numeric") == 0){
            compare->double_ref_val = atof(events->events[i]->value);

            cJSON* dv = cJSON_GetObjectItem(data, events->events[i]->param_name);

            if (dv == NULL){ goto end; }

            if (dv->type != cJSON_Number) { syslog(LOG_ERR, "Value not of type 'Number'"); goto end; }

            syslog(LOG_INFO, "Double value: %f", dv->valuedouble);
            compare->double_val = dv->valuedouble;

            res = evaluate_numeric(compare);
        }
        else if (strcmp(compare->val_type, "alphanumeric") == 0)
        {
            compare->string_ref_val = events->events[i]->value;
            
            cJSON* sv = cJSON_GetObjectItem(data, events->events[i]->param_name);

            if (sv == NULL){ goto end; }

            if (sv->type != cJSON_String) { syslog(LOG_ERR, "Value not of type 'String'"); goto end; }
            
            syslog(LOG_INFO, "String value: %s", sv->valuestring);

            compare->string_val = sv->valuestring;

            res = evaluate_alphanumeric(compare);
        }
        else {
            syslog(LOG_ERR, "Invalid value type (should be numeric or alphanumeric).");
            goto end;
        }

        if (res < 0){
            syslog(LOG_ERR, "Unsupported compare type.");
            goto end;
        }

        if (res == 1){
            char* json_string = cJSON_Print(data);
            char* msg = build_message_body(events->events[i]->topic, events->events[i]->event_name, json_string);
            send_email(events->events[i]->email, events->events[i]->recipient, events->events[i]->username, events->events[i]->password, msg);

            free(msg);
            free(json_string);
        }

        end:
            free(compare);
    }
}

void free_events(struct mqtt_events* events)
{
    for(int i = 0; i < events->count; i++)
    {
        free(events->events[i]->email);
        free(events->events[i]->event_name);
        free(events->events[i]->param_name);
        free(events->events[i]->password);
        free(events->events[i]->recipient);
        free(events->events[i]->topic);
        free(events->events[i]->username);
        free(events->events[i]->value);
        free(events->events[i]->value_type);
        free(events->events[i]);
    }
    free(events->events);
    free(events);
}

int evaluate_numeric(struct mqtt_compare* compare)
{
    switch(compare->comp_type)
    {
        case EQUAL:
            return compare->double_val == compare->double_ref_val ? 1 : 0;
        case NOT_EQUAL:
            return compare->double_val != compare->double_ref_val ? 1 : 0;
        case LESS:
            return compare->double_val < compare->double_ref_val ? 1 : 0;
        case LESS_EQUAL:
            return compare->double_val <= compare->double_ref_val ? 1 : 0;
        case GREATER:
            return compare->double_val > compare->double_ref_val ? 1 : 0;
        case GREATER_EQUAL:
            return compare->double_val >= compare->double_ref_val ? 1 : 0;
        default:
            return -1;
    }
}

int evaluate_alphanumeric(struct mqtt_compare* compare)
{
    switch(compare->comp_type)
    {
        case EQUAL:
            return strcmp(compare->string_val, compare->string_ref_val) == 0 ? 1 : 0;
        case NOT_EQUAL:
            return strcmp(compare->string_val, compare->string_ref_val) != 0 ? 1 : 0;
        case LESS:
            return strcmp(compare->string_val, compare->string_ref_val) < 0 ? 1 : 0;
        case LESS_EQUAL:
            return strcmp(compare->string_val, compare->string_ref_val) <= 0 ? 1 : 0;
        case GREATER:
            return strcmp(compare->string_val, compare->string_ref_val) > 0 ? 1 : 0;
        case GREATER_EQUAL:
            return strcmp(compare->string_val, compare->string_ref_val) >= 0 ? 1 : 0;
        default:
            return -1;
    }
}