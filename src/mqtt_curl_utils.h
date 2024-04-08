#ifndef _MQTT_CURL_UTILS_H
#define _MQTT_CURL_UTILS_H
    int send_email(const char* from, const char* to, const char* username, const char* password, const char* message);
    char* build_email_body(const char* from, const char* to, const char* subject, const char* message);
    char* build_message_body(const char* topic, const char* event_name, const char* data);
#endif