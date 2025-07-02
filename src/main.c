#include <restful/event_loop.h>
#include <sys/syslog.h>

int main(void)
{
    openlog("restful", LOG_PID, LOG_USER);

    restful_loop_t loop;
    restful_init(&loop, "0.0.0.0", 8000);
    restful_dispatch(&loop);
    // TODO: exit function (adds overhead for fd tracking)

    closelog();
    return 0;
}
