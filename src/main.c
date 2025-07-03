#include <restful/event_loop.h>
#include <sys/syslog.h>

int main(void)
{
    openlog("restful", LOG_PID, LOG_USER);

    restful_loop_t loop;
    restful_init(&loop, "0.0.0.0", 8000);

    // TODO: implement signals
    for (;;) {
        restful_dispatch(&loop);
    }

    closelog();
    return 0;
}
