#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <hiredis/hiredis.h>

#define PORT 5000

// Funktion för att hämta UID från JSON
void get_uid(const char *data, char *uid) {
    sscanf(data, "{\"uid\":\"%63[^\"]\"}", uid);
}

// Funktion för att kolla access i Redis
int check_access(char *uid) {
    redisContext *conn = redisConnect("127.0.0.1", 6379);

    if (conn == NULL || conn->err) {
        printf("Redis error\n");
        return 0;
    }

    redisReply *reply = redisCommand(conn, "GET access:%s", uid);

    int result = 0;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        if (strcmp(reply->str, "1") == 0) {
            result = 1;
        }
    }

    if (reply) freeReplyObject(reply);
    redisFree(conn);

    return result;
}

// Logga passage
void log_event(char *uid) {
    redisContext *conn = redisConnect("127.0.0.1", 6379);

    if (conn != NULL && !conn->err) {
        redisCommand(conn, "RPUSH parking_logs %s", uid);
        redisFree(conn);
    }
}

// HTTP handler
int request_handler(void *cls,
                    struct MHD_Connection *connection,
                    const char *url,
                    const char *method,
                    const char *version,
                    const char *upload_data,
                    size_t *upload_data_size,
                    void **con_cls)
{
    if (strcmp(method, "POST") != 0) {
        return MHD_NO;
    }

    if (*upload_data_size != 0) {
        char uid[64] = {0};
        get_uid(upload_data, uid);

        printf("UID received: %s\n", uid);

        int access = check_access(uid);

        if (access) {
            log_event(uid);
        }

        const char *response_text;

        if (access) {
            response_text = "{\"access\": true}";
        } else {
            response_text = "{\"access\": false}";
        }

        struct MHD_Response *response =
            MHD_create_response_from_buffer(strlen(response_text),
                                            (void *)response_text,
                                            MHD_RESPMEM_PERSISTENT);

        *upload_data_size = 0;
        return MHD_queue_response(connection, 200, response);
    }

    return MHD_YES;
}

int main() {
    struct MHD_Daemon *server;

    server = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                              PORT,
                              NULL,
                              NULL,
                              &request_handler,
                              NULL,
                              MHD_OPTION_END);

    if (!server) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on port %d...\n", PORT);
    getchar();

    MHD_stop_daemon(server);
    return 0;
}
