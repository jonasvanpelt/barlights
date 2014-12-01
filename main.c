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
#include "www_files.h"

/*------------------------------------------------------------------------------
 global variable declarations
 ------------------------------------------------------------------------------*/

#define DEBUG 1
#define DEBUG_PRINT(...) do{ if(DEBUG > 0) fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#define SIZE 1 * 1024

/*------------------------------------------------------------------------------
 function prototypes
 ------------------------------------------------------------------------------*/
static void serverWakeup(uint16_t ev, uint16_t conn);
static void picoTickTask(void);
void getRgbFromResource(char *resource, uint8_t *r, uint8_t *g, uint8_t *b);
const struct Www_file * find_www_file(char * filename);
/*------------------------------------------------------------------------------
 global variable declarations
 ------------------------------------------------------------------------------*/

static struct pico_device *pico_dev;
char http_buffer[SIZE];

/*------------------------------------------------------------------------------
 implementation code
 --------------------------------------------------------------------------------*/
const struct Www_file * find_www_file(char * filename) {
	uint16_t i;
	for (i = 0; i < num_files; i++) {
		if (strcmp(www_files[i].filename, filename) == 0) {
			return &www_files[i];
		}
	}
	return NULL;
}

void getRgbFromResource(char *resource, uint8_t *r, uint8_t *g, uint8_t *b) {

	char* token = strtok(resource, "-");
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

	if (ev & EV_HTTP_CON) {
		DEBUG_PRINT("New connection received....\n");
		pico_http_server_accept();
	}
	if (ev & EV_HTTP_REQ) // new header received
	{
		//int read;
		char * resource;
		DEBUG_PRINT("Header request was received...\n");
		DEBUG_PRINT("> Resource : %s\n", pico_http_getResource(conn));
		resource = pico_http_getResource(conn);

		if (strstr(resource, "/rgb")) {
			uint8_t r, g, b;
			DEBUG_PRINT("Accepted connection...\n");

			pico_http_respond(conn, HTTP_RESOURCE_FOUND);
			getRgbFromResource(&resource[11], &r, &g, &b);
			DEBUG_PRINT("Received rgb values: %d %d %d\n", r, g, b);

		} else if (strcmp(resource, "/") == 0) {
			resource = "/index.html";
		} else {

			/* search in flash resources */
			struct Www_file * www_file;
			www_file = find_www_file((const char *)resource + 1);
			if (www_file != NULL) {
				uint16_t flags;
				flags = HTTP_RESOURCE_FOUND | HTTP_STATIC_RESOURCE;
				if (www_file->cacheable) {
					flags = flags | HTTP_CACHEABLE_RESOURCE;
				}
				pico_http_respond(conn, flags);
				pico_http_submitData(conn, www_file->content, (int) *www_file->filesize);
			} else { /* not found */
				/* reject */
				DEBUG_PRINT("Rejected connection...\n");
				pico_http_respond(conn, HTTP_RESOURCE_NOT_FOUND);
			}
		}
	}
	if (ev & EV_HTTP_PROGRESS) // submitted data was sent
	{
		uint16_t sent, total;
		pico_http_getProgress(conn, &sent, &total);
		DEBUG_PRINT("Chunk statistics : %d/%d sent\n", sent, total);
	}
	if (ev & EV_HTTP_SENT) // submitted data was fully sent
	{
		DEBUG_PRINT("Last chunk post !\n");
		  pico_http_submitData(conn, NULL, 0); /* send the final chunk */
	}
	if (ev & EV_HTTP_CLOSE) {
		DEBUG_PRINT("Close request...\n");
		pico_http_close(conn);
	}
	if (ev & EV_HTTP_ERROR) {
		DEBUG_PRINT("Error on server...\n");
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
