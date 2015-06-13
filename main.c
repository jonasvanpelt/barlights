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
const struct Www_file * find_www_file(char * filename);
/*------------------------------------------------------------------------------
 global variable declarations
 ------------------------------------------------------------------------------*/

static struct pico_device *pico_dev;
char http_buffer[SIZE];

static int autoColor = 0;
static uint8_t sR, sG, sB;
/*R, G and B are each 1 byte, so we have 2^24 color combinations*/
static uint32_t colorCounter = 0;


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

uint8_t hexCharToNumber(char c) {

	uint8_t value = 0;

	/*NOTE: Function only accepts lower case hex signs*/
	switch (c) {
	case '0':
		value = 0;
		break;
	case '1':
		value = 1;
		break;
	case '2':
		value = 2;
		break;
	case '3':
		value = 3;
		break;
	case '4':
		value = 4;
		break;
	case '5':
		value = 5;
		break;
	case '6':
		value = 6;
		break;
	case '7':
		value = 7;
		break;
	case '8':
		value = 8;
		break;
	case '9':
		value = 9;
		break;
	case 'a':
		value = 10;
		break;
	case 'b':
		value = 11;
		break;
	case 'c':
		value = 12;
		break;
	case 'd':
		value = 13;
		break;
	case 'e':
		value = 14;
		break;
	case 'f':
		value = 15;
		break;
	default:
		value = 0;
		break;
	}

	return value;
}

void sendPage(uint16_t conn, char * resource) {
	/* search in flash resources */
	const struct Www_file * www_file;
	www_file = find_www_file((char *) resource + 1);
	if (www_file != NULL) {
		uint16_t flags;
		flags = HTTP_RESOURCE_FOUND | HTTP_STATIC_RESOURCE;
		if (www_file->cacheable) {
			flags = flags | HTTP_CACHEABLE_RESOURCE;
		}
		pico_http_respond(conn, flags);
		pico_http_submitData(conn, (void *) www_file->content,
				(int) *www_file->filesize);
	} else { /* not found */
		/* reject */
		DEBUG_PRINT("Rejected connection...\n");
		pico_http_respond(conn, HTTP_RESOURCE_NOT_FOUND);
	}
}

void getRgbFromHttpBody(char *resource, uint8_t *r,
		uint8_t *g, uint8_t *b) {

	/* This is the kind of string we will receive:
	 * colorHex=%23000000
	 */
	*r = (hexCharToNumber(resource[12]) << 4) | hexCharToNumber(resource[13]);
	*g = (hexCharToNumber(resource[14]) << 4) | hexCharToNumber(resource[15]);
	*b = (hexCharToNumber(resource[16]) << 4) | hexCharToNumber(resource[17]);
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
		char * body;
		DEBUG_PRINT("Header request was received...\n");
		DEBUG_PRINT("> Resource : %s\n", pico_http_getResource(conn));
		resource = pico_http_getResource(conn);

		if (strstr(resource, "setColor")) {

			DEBUG_PRINT("Accepted connection...\n");

			/*Get the POST data*/
			body = pico_http_getBody(conn);

			getRgbFromHttpBody(&body[0], &sR, &sG, &sB);

			pico_http_respond_redirect(conn);


			DEBUG_PRINT("body: %s\n", body);
			DEBUG_PRINT("Received rgb values: %d %d %d\n", sR, sG, sB);

		} else if (strstr(resource, "autoColor")) {
			pico_http_respond_redirect(conn);

			if (autoColor == 0) {

				DEBUG_PRINT("Enabled automatic color selection\n");


				autoColor = 1;

				/*Start the color counter from the current selected color*/
				colorCounter = (sR << 16) | (sG << 8) | sB;
			} else {
				DEBUG_PRINT("Disabled automatic color selection\n");
				autoColor = 0;
			}
		} else {
			if (strcmp(resource, "/") == 0) {
				resource = "/index.html";
			}
			sendPage(conn, resource);
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

	pico_string_to_ipv4("192.168.4.1", &my_ip.addr);
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
