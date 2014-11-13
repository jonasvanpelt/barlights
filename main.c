#include <stdio.h>

#include "pico_stack.h"
#include "pico_config.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_device.h"
#include "pico_dev_tun.h"

static struct pico_device *pico_dev;

int main()
{
        struct pico_ip4 my_ip;
        struct pico_ip4 netmask;

        pico_string_to_ipv4("192.168.5.10", &my_ip.addr);
        pico_string_to_ipv4("255.255.255.0", &netmask.addr);

        printf("picoTest start!\n");
        pico_stack_init();

        pico_dev = (struct pico_device *) pico_tun_create("tun0");
        if(!pico_dev) {
                printf("Error creating pico device, exiting...\n");
                exit(1);
        }

        pico_ipv4_link_add(pico_dev, my_ip, netmask);

        printf("Starting picoTCP loop\n");
        while(1) {
                pico_stack_tick();
                PICO_IDLE();
        }
}
