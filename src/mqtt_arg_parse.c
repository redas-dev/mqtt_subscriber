#define _DEFAULT_SOURCE

#include "mqtt_arg_parse.h"
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdbool.h>

static struct argp_option options[] = {
  { "host"         , 'h', "value", 0                   , "Host IP"},
  { "port"         , 'p', "value", 0                   , "Host Port"},
  { "topics"       , 't', "value", 0                   , "Topic to subscribe to"},
  { "username"     , 'u', "value", 0                   , "Username of subscriber"},
  { "password"     , 'P', "value", 0                   , "Password of subscriber"},
  { "cafile"       , 'c', "value", 0                   , "Path to certificate"},
  { "certfile"     , 'e', "value", 0                   , "Path to certificate"},
  { "keyfile"      , 'k', "value", 0                   , "Path to key"},
  { "daemon"       , 'D', 0      , OPTION_ARG_OPTIONAL , "Daemonize program"},
  { 0 }
};

bool topic_exists(char* topic, char** topics, int count)
{
  for(int i = 0; i < count; i++)
    if (strcmp(topic, topics[i]) == 0) return true;
  return false;
}

extern error_t parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* arguments = state->input;
  switch (key)
  {
    case 'h':
      arguments->host = arg;
      break;
    case 'p':
      arguments->port = atoi(arg);
      break;
    case 't':
      if (topic_exists(arg, arguments->topics, arguments->topic_count)) break;
      arguments->topic_count++;
      char** temp = (char**)realloc(arguments->topics, (arguments->topic_count + 1) * sizeof(char*));
      if (temp == NULL) { arguments->topic_count--; break; }
      arguments->topics = temp;
      arguments->topics[arguments->topic_count - 1] = strdup(arg);
      break;
    case 'u':
      strncpy(arguments->username, arg, sizeof(arguments->username) - 1);
      break;
    case 'P':
      strncpy(arguments->password, arg, sizeof(arguments->password) - 1);
      break;
    case 'c':
      strncpy(arguments->cafile, arg, sizeof(arguments->cafile) - 1);
      break;
    case 'e':
      strncpy(arguments->certfile, arg, sizeof(arguments->certfile) - 1);
      break;
    case 'k':
      strncpy(arguments->keyfile, arg, sizeof(arguments->keyfile) - 1);
      break;
    case 'D':
      arguments->is_daemon = 1;
      break;
    case ARGP_KEY_END:
      if (arguments->host == NULL)
      {
        if (arguments->topics) free_topics(arguments);
        argp_failure(state, 1, 0, "required -h not set. See --help for more info.");
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

struct argp argp = { options, parse_opt, NULL, _ARG_DESC_ };

struct arguments parse_subscriber_args(int argc, char** argv)
{
  struct arguments args;
  memset(args.cafile, 0, sizeof(args.cafile));
  memset(args.certfile, 0, sizeof(args.certfile));
  memset(args.keyfile, 0, sizeof(args.keyfile));
  memset(args.username, 0, sizeof(args.username));
  memset(args.password, 0, sizeof(args.password));
  args.host = NULL;
  args.is_daemon = 0;
  args.port = 1883; // Default port
  args.topic_count = 0;
  args.topics = NULL;
  
  error_t err = argp_parse(&argp, argc, argv, 0, 0, &args);

  if (err != 0){
    free_topics(&args);
    exit(1);
  }

  return args;
}

void free_topics(struct arguments* args)
{
  if (args != NULL){
    for(int i = 0; i < args->topic_count; i++)
      free(args->topics[i]);
    free(args->topics);
  }
}