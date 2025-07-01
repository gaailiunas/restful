#include <restful/eventloop.h>

int main(void)
{
    restful_loop_t loop;
    restful_init(&loop, "0.0.0.0", 8000);
    restful_dispatch(&loop);
    // TODO: exit function (adds overhead for fd tracking)
    return 0;
}
