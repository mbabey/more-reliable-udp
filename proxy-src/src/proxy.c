//
// Created by Maxwell Babey on 10/27/22.
//

#include "../../libs/include/error.h"
#include "../../libs/include/manager.h"
#include "../../libs/include/util.h"
#include "../include/proxy.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/**
 * Per cent - per one-hundred. Used with random number generation.
 */
#define HUNDRED_PERCENT 100

/**
 * @author D'Arcy Smith
 */
static volatile sig_atomic_t running;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * link_proxy
 * <p>
 * Create a socket, bind the IP specified in proxy_settings to the socket, then listen on the socket.
 * </p>
 * @param set - proxy_settings *: pointer to the settings for this proxy
 */
void link_proxy(struct proxy_settings *set);

/**
 * get_proxy_socket
 * <p>
 * Create a socket on which the proxy server will wait for messages.
 * </p>
 * @param set - the proxy settings
 */
void get_proxy_socket(struct proxy_settings *set);

/**
 * get_output_address
 * <p>
 * Create a socket to which the proxy will forward messages.
 * </p>
 * @param set
 */
void get_output_address(struct proxy_settings *set);

/**
 * await_connect
 * <p>
 * Await and accept client connections. If program is interrupted by the user while waiting, close the proxy.
 * </p>
 * @param set - proxy_settings *: pointer to the settings for this proxy
 */
void await_connect(struct proxy_settings *set);

/**
 * sv_recvfrom.
 * <p>
 * Await a message from the connected client. If no message is received and a timeout occurs,
 * resend the last-sent packet. If a timeout occurs too many times, drop the connection.
 * </p>
 * @param set - the proxy settings
 * @param send_packet - the packet struct to send
 * @param recv_packet - the packet struct to sv_recvfrom
 */
void await_message(struct proxy_settings *set);

/**
 * determine_action
 * <p>
 * Based on a random number, drop the packet, hold the packet then forward the packet, or forward the packet.
 * </p>
 * @param set - the proxy settings
 * @param buffer - the packet
 */
void determine_action(struct proxy_settings *set, uint8_t *buffer);

uint32_t get_packet_size(const uint8_t *buffer);

void forward_message(const struct proxy_settings *set, const uint8_t *buffer);

_Noreturn void close_proxy(struct proxy_settings *set, int exit_code);

/**
 * set_signal_handling
 * @param sa
 * @author D'Arcy Smith
 */
static void set_signal_handling(struct sigaction *sa);

/**
 * signal_handler
 * @param sig
 * @author D'Arcy Smith
 */
static void signal_handler(int sig);

void run(int argc, char *argv[], struct proxy_settings *set)
{
    init_def_state(argc, argv, set);
    link_proxy(set);
    await_connect(set);
}

void link_proxy(struct proxy_settings *set)
{
    get_proxy_socket(set);
    get_output_address(set);
}

void get_proxy_socket(struct proxy_settings *set)
{
    struct sockaddr_in proxy_addr;
    
    memset(&proxy_addr, 0, sizeof(struct sockaddr_in));
    
    if ((set->proxy_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // NOLINT(android-cloexec-socket) : SOCK_CLOEXEC dne
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_proxy(set, EXIT_FAILURE);
    }
    
    proxy_addr.sin_family           = AF_INET;
    proxy_addr.sin_port             = htons(set->proxy_port);
    if ((proxy_addr.sin_addr.s_addr = inet_addr(set->proxy_ip)) == (in_addr_t) -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_proxy(set, EXIT_FAILURE);
    }
    
    if (bind(set->proxy_fd, (struct sockaddr *) &proxy_addr, sizeof(struct sockaddr_in)) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_proxy(set, EXIT_FAILURE);
    }
}

void get_output_address(struct proxy_settings *set)
{
    set->output_addr = (struct sockaddr_in *) s_calloc(1, sizeof(struct sockaddr_in), __FILE__, __func__, __LINE__);
    if (errno == ENOTRECOVERABLE)
    {
        close_proxy(set, EXIT_FAILURE);
    }
    set->mem_manager->mm_add(set->mem_manager, set->output_addr);
    
    set->output_addr->sin_family           = AF_INET;
    set->output_addr->sin_port             = htons(set->output_port);
    if ((set->output_addr->sin_addr.s_addr = inet_addr(set->output_ip)) == (in_addr_t) -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_proxy(set, EXIT_FAILURE);
    }
}

void await_connect(struct proxy_settings *set)
{
    struct sigaction sa;
    
    set_signal_handling(&sa);
    running = 1;
    
    while (running)
    {
        await_message(set);
    }
}

void await_message(struct proxy_settings *set)
{
    uint8_t   buffer[BUF_LEN];
    ssize_t   bytes_recv;
    socklen_t sockaddr_in_size;
    
    srandom(time(NULL));
    sockaddr_in_size = sizeof(struct sockaddr_in);
    set->from_addr = (struct sockaddr_in *) s_calloc(1, sizeof(struct sockaddr_in), __FILE__, __func__, __LINE__);
    if (errno == ENOTRECOVERABLE)
    {
        close_proxy(set, EXIT_FAILURE);
    }
    set->mem_manager->mm_add(set->mem_manager, set->from_addr);
    
    set->input_addr = (struct sockaddr_in *) s_calloc(1, sizeof(struct sockaddr_in), __FILE__, __func__, __LINE__);
    if (errno == ENOTRECOVERABLE)
    {
        close_proxy(set, EXIT_FAILURE);
    }
    set->mem_manager->mm_add(set->mem_manager, set->input_addr);
    do
    {
        printf("Awaiting message\n");
        
        memset(buffer, 0, BUF_LEN);
        if ((bytes_recv = recvfrom(set->proxy_fd, buffer, BUF_LEN, 0,
                                   (struct sockaddr *) set->from_addr, &sockaddr_in_size)) == -1)
        {
            switch(errno)
            {
                case EINTR:
                {
                    close_proxy(set, EXIT_SUCCESS);
                }
                default:
                {
                    fatal_errno(__FILE__, __func__, __LINE__, errno);
                    close_proxy(set, EXIT_FAILURE);
                }
            }
        }
        
        // Will get the first address as the client address.
        if (set->input_addr->sin_addr.s_addr == 0)
        {
            memcpy(set->input_addr, set->from_addr, sizeof(struct sockaddr_in));
        }
        
        determine_action(set, buffer);
        
    } while (bytes_recv > 0);
}

void determine_action(struct proxy_settings *set, uint8_t *buffer)
{
    uint64_t directive;
    char *src = inet_ntoa(set->from_addr->sin_addr); // NOLINT(concurrency-mt-unsafe) : no threads here
    
    directive = random() % HUNDRED_PERCENT; // NOLINT(clang-analyzer-security.insecureAPI.rand) : insecure ok
    if (0 < directive && directive <= set->drop_bound)
    {
        // Do not send the packet.
        printf("Packet from %s with flags %s dropped.\n", src, check_flags(*buffer));
        return;
    }
    if (set->drop_bound < directive && directive <= set->hold_bound)
    {
        // Pause a new thread.
    }
    forward_message(set, buffer);
}

void forward_message(const struct proxy_settings *set, const uint8_t *buffer)
{
    struct sockaddr_in to_addr;
    char *src = inet_ntoa(set->from_addr->sin_addr); // NOLINT(concurrency-mt-unsafe) : no threads here
    char *dest;
    
    if (set->from_addr->sin_addr.s_addr == set->input_addr->sin_addr.s_addr)
    {
        to_addr = *set->output_addr;
        dest = set->output_ip;
    } else
    {
        to_addr = *set->input_addr;
        dest = inet_ntoa(set->input_addr->sin_addr); // NOLINT(concurrency-mt-unsafe) : no threads here
    }
    
    printf("Packet coming from %s and going to %s with flags %s\n", src, dest, check_flags(*buffer));
    
    uint8_t packet_size = get_packet_size(buffer);
    
    if (sendto(set->proxy_fd, buffer, packet_size, 0,
               (struct sockaddr *) &to_addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("Proxy failed to forward message");
    }
}

uint32_t get_packet_size(const uint8_t *buffer)
{
    const uint8_t flag_size = 4;
    uint16_t payload_len;
    uint32_t ret_val;
    
    memcpy(&payload_len, buffer + sizeof(uint16_t), sizeof(uint16_t));
    
    payload_len = ntohs(payload_len);
    
    ret_val = payload_len + flag_size;
    
    return ret_val;
}

_Noreturn void close_proxy(struct proxy_settings *set, int exit_code)
{
    if (set->proxy_fd != 0)
    {
        close(set->proxy_fd);
    }
    free_memory_manager(set->mem_manager);
    exit(exit_code); // NOLINT(concurrency-mt-unsafe) : no threads here
}

static void set_signal_handling(struct sigaction *sa)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = signal_handler;
    if (sigaction(SIGINT, sa, NULL) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void signal_handler(int sig)
{
    running = 0;
}

#pragma GCC diagnostic pop

