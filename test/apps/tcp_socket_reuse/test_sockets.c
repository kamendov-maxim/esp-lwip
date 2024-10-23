#include <pthread.h>
#include <tcp/tcp_helper.h>

#include "lwip_check.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/api.h"

Suite *sockets_suite(void);

static int
test_sockets_get_used_count(void)
{
    int used = 0;
    int i;

    for (i = 0; i < NUM_SOCKETS; i++) {
        struct lwip_sock* s = lwip_socket_dbg_get_socket(i);
        if (s != NULL) {
            if (s->fd_used) {
                used++;
            }
        }
    }
    return used;
}

#if 0
static int
wait_for_pcbs_to_cleanup(void)
{
    struct tcp_pcb *pcb = tcp_active_pcbs;
    while (pcb != NULL) {
        if (pcb->state == TIME_WAIT || pcb->state == LAST_ACK) {
            return -1;
        }
        pcb = pcb->next;
    }
    return 0;
}
#endif
/* Setups/teardown functions */
static void
sockets_setup(void)
{
    fail_unless(test_sockets_get_used_count() == 0);
}

static void
sockets_teardown(void)
{
    fail_unless(test_sockets_get_used_count() == 0);
    /* poll until all memory is released... */
    while (tcp_tw_pcbs) {
        tcp_abort(tcp_tw_pcbs);
    }
}

START_TEST(test_tcp_active_socket_limit)
{
    int i;
    int sock;
    LWIP_UNUSED_ARG(_i);
    for (i = 0; i < MEMP_NUM_TCP_PCB; i++) {
        sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
        fail_unless(sock != -1);
    }
    sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    fail_unless(sock == -1);
    for (i = 0; i < MEMP_NUM_TCP_PCB; i++) {
        lwip_close(i + LWIP_SOCKET_OFFSET);
    }
}
END_TEST

START_TEST(test_tcp_new_max_num)
{
    struct tcp_pcb* pcb[MEMP_NUM_TCP_PCB + 1];
    int i;
    mem_size_t prev_used, one_pcb_size;
    LWIP_UNUSED_ARG(_i);

    prev_used = lwip_stats.mem.used;
    pcb[0] = tcp_new();
    fail_unless(pcb[0] != NULL);
    one_pcb_size = lwip_stats.mem.used - prev_used;

    for(i = 1; i < MEMP_NUM_TCP_PCB; i++) {
        prev_used = lwip_stats.mem.used;
        pcb[i] = tcp_new();
        fail_unless(pcb[i] != NULL);
        fail_unless(one_pcb_size == lwip_stats.mem.used - prev_used);
    }
    /* Trying to remove the oldest pcb in TIME_WAIT,LAST_ACK,CLOSING state when pcb full */
    pcb[MEMP_NUM_TCP_PCB] = tcp_new();
    fail_unless(pcb[MEMP_NUM_TCP_PCB] == NULL);
    tcp_set_state(pcb[0], TIME_WAIT, &test_local_ip, &test_remote_ip, TEST_LOCAL_PORT, TEST_REMOTE_PORT);
    pcb[MEMP_NUM_TCP_PCB] = tcp_new();
    fail_unless(pcb[MEMP_NUM_TCP_PCB] != NULL);

    for (i = 1; i <= MEMP_NUM_TCP_PCB; i++)
    {
        tcp_abort(pcb[i]);
    }
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
sockets_suite(void)
{
    testfunc tests[] = {
            TESTFUNC(test_tcp_active_socket_limit),
            TESTFUNC(test_tcp_new_max_num),
    };
    return create_suite("TCP_ACTIVE_SOCKETS", tests, sizeof(tests)/sizeof(testfunc), sockets_setup, sockets_teardown);
}
