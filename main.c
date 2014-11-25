#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pico_stack.h"
#include "pico_config.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_device.h"
#include "pico_dev_tun.h"
#include "pico_http_server.h"

/*------------------------------------------------------------------------------
 global variable declarations
 ------------------------------------------------------------------------------*/
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#define SIZE 1 * 1024

/*------------------------------------------------------------------------------
 function prototypes
 ------------------------------------------------------------------------------*/
static void serverWakeup(uint16_t ev, uint16_t conn);
static void picoTickTask(void);
void getRgbFromResource(char *resource, uint8_t *r, uint8_t *g, uint8_t *b);

/*------------------------------------------------------------------------------
 global variable declarations
 ------------------------------------------------------------------------------*/

static struct pico_device *pico_dev;
char http_buffer[SIZE];

/*------------------------------------------------------------------------------
 implementation code
 --------------------------------------------------------------------------------*/

void getRgbFromResource(char *resource, uint8_t *r, uint8_t *g, uint8_t *b) {

	char* token = strtok(&resource[11], "-");
	int i = 0;

	while (token) {
		switch (i) {
		case 0:
			*r = atoi(token);
			break;
		case 1:
			*g = atoi(token);
			break;
		case 2:
			*b = atoi(token);
			break;
		default:
			break;
		}
		token = strtok(NULL, "-");
		i++;
	}
}

void serverWakeup(uint16_t ev, uint16_t conn) {
	static FILE * f;
	char buffer[SIZE];

	if (ev & EV_HTTP_CON) {
		printf("New connection received....\n");
		pico_http_server_accept();
	}
	if (ev & EV_HTTP_REQ) // new header received
	{
		//int read;
		char * resource;
		printf("Header request was received...\n");
		printf("> Resource : %s\n", pico_http_getResource(conn));
		resource = pico_http_getResource(conn);

		// Accepting request
		/*if (strcmp(resource, "/") == 0 || strcmp(resource, "index.html") == 0 || strcmp(resource, "/index.html") == 0) {
		 printf("Accepted connection...\n");
		 pico_http_respond(conn, HTTP_RESOURCE_FOUND);
		 f = fopen("html/index.html", "r");
		 if (!f) {
		 fprintf(stderr, "Unable to open the file /test/examples/index.html\n");
		 exit(1);
		 }
		 read = fread(buffer, 1, SIZE, f);
		 pico_http_submitData(conn, buffer, read);
		 } else*/
		if (strstr(resource, "rgb")) {
			uint8_t r, g, b;
			printf("Accepted connection...\n");

			pico_http_respond(conn, HTTP_RESOURCE_FOUND);
			getRgbFromResource(resource, &r, &g, &b);
			printf("Received rgb values: %d %d %d\n", r, g, b);

		} else {
			// reject
			printf("Rejected connection...\n");
			pico_http_respond(conn, HTTP_RESOURCE_NOT_FOUND);
		}
	}
	if (ev & EV_HTTP_PROGRESS) // submitted data was sent
	{
		uint16_t sent, total;
		pico_http_getProgress(conn, &sent, &total);
		printf("Chunk statistics : %d/%d sent\n", sent, total);
	}
	if (ev & EV_HTTP_SENT) // submitted data was fully sent
	{
		int read;
		read = fread(buffer, 1, SIZE, f);
		printf("Chunk was sent...\n");
		if (read > 0) {
			printf("Sending another chunk...\n");
			pico_http_submitData(conn, buffer, read);
		} else {
			printf("Last chunk !\n");
			pico_http_submitData(conn, NULL, 0); // send the final chunk
			fclose(f);
		}
	}
	if (ev & EV_HTTP_CLOSE) {
		printf("Close request...\n");
		pico_http_close(conn);
	}
	if (ev & EV_HTTP_ERROR) {
		printf("Error on server...\n");
		pico_http_close(conn);
	}
}

static void picoTickTask(void) {

	struct pico_ip4 my_ip;
	struct pico_ip4 netmask;

	pico_string_to_ipv4("192.168.2.150", &my_ip.addr);
	pico_string_to_ipv4("255.255.255.0", &netmask.addr);

	DEBUG_PRINT("picoTest start!\n");

	pico_stack_init();

	pico_dev = (struct pico_device *) pico_tun_create("tun0");
	if (!pico_dev) {
		DEBUG_PRINT("Error creating pico device, exiting...\n");
		exit(1);
	}

	pico_ipv4_link_add(pico_dev, my_ip, netmask);

	pico_http_server_start(0, serverWakeup);

	while (1) {
		pico_stack_tick();
		PICO_IDLE();
	}
}

int main() {
	picoTickTask();

	return 0;
}
