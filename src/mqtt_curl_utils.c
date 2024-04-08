#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>

#include "mqtt_curl_utils.h"

struct email_msg {
  size_t bytes_read;
  char* DATA;
};

static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct email_msg *upload_ctx = (struct email_msg *)userp;
	const char *data;
	size_t room = size * nmemb;

	if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
		return 0;
	}

	data = &upload_ctx->DATA[upload_ctx->bytes_read];

	if (data) {
		size_t len = strlen(data);
		if (room < len)
			len = room;
		memcpy(ptr, data, len);
		upload_ctx->bytes_read += len;

		return len;
	}

	return 0;
}

int send_email(const char* from, const char* to, const char* username, const char* password, const char* message)
{
  if (from == NULL || to == NULL || username == NULL || password == NULL) { 
    syslog(LOG_INFO, "Tried to send email with invalid data. Canceling."); 
    return -1;
  }
  
  CURL *curl;
  int res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct email_msg upload_ctx = { 0 };
  
  curl = curl_easy_init();
  if(curl) {
    syslog(LOG_INFO, "Curl initiated");

    upload_ctx.DATA = build_email_body(from, to, "MQTT event trigger", message);

    curl_easy_setopt(curl, CURLOPT_USERNAME, username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);
    recipients = curl_slist_append(recipients, to);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
      syslog(LOG_ERR, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    free(upload_ctx.DATA);
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
  }
  else syslog(LOG_ERR, "Failed to init curl");
  
  return res;
}

// Builds the body for the email to send by curl
char* build_email_body(const char* from, const char* to, const char* subject, const char* message)
{
  size_t ffromSize = strlen(from) + 10;
  char* ffrom = malloc(ffromSize);
  snprintf(ffrom, ffromSize, "%s%s%s", "From:", from, "\r\n");

  size_t ttoSize = strlen(to) + 7;
  char* tto = malloc(ttoSize);
  snprintf(tto, ttoSize, "%s%s%s", "To:", to, "\r\n");

  size_t ssubjectSize = strlen(subject) + 12;
  char* ssubject = malloc(ssubjectSize);
  snprintf(ssubject, ssubjectSize, "%s%s%s", "Subject:", subject, "\r\n");

  char linebreak[] = "\r\n";
  size_t linebreakSize = strlen(linebreak) + 1;

  size_t mmessageSize = strlen(message) + 4;
  char* mmessage = malloc(mmessageSize);
  snprintf(mmessage, mmessageSize, "%s%s", message, "\r\n");

  size_t payloadSize = ffromSize + ttoSize + linebreakSize + ssubjectSize + mmessageSize + 1;

  char* body = malloc(payloadSize);

  snprintf(body, payloadSize, "%s%s%s%s%s", ffrom, tto, ssubject, linebreak, mmessage);

  free(ffrom);
  free(tto);
  free(ssubject);
  free(mmessage);

  return body;
}

// Builds the message to be sent by curl
char* build_message_body(const char* topic, const char* event_name, const char* data)
{
  if (topic == NULL || event_name == NULL || data == NULL) return NULL;

  size_t msgSize = strlen(topic) + strlen(event_name) + strlen(data) + 40;
  char* msg = malloc(msgSize);
  snprintf(msg, msgSize, "Event '%s' accured on topic '%s' with data %s", event_name, topic, data);

  return msg;
}