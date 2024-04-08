#include <argp.h>
#include <stdbool.h>

#ifndef _ARG_DESC_
#define _ARG_DESC_ "MQTT subscriber"
#endif

#ifndef _ARG_PARSER_
#define _ARG_PARSER_
  struct arguments
  {
    char* host;
    int port;
    char** topics;
    int topic_count;
    char username[40];
    char password[40];
    char cafile[512];
    char certfile[512];
    char keyfile[512];
    int is_daemon;
  };

  bool topic_exists(char* topic, char** topics, int count);
  struct arguments parse_subscriber_args(int argc, char** argv);
  void free_topics(struct arguments* args);
#endif