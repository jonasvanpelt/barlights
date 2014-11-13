#include <stdio.h>
#include <stdint.h>

#include "pico_stack.h"
#include "pico_config.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_device.h"
#include "pico_dev_tun.h"

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

/*------------------------------------------------------------------------------
		global variable declarations
------------------------------------------------------------------------------*/

static struct pico_device *pico_dev;
char http_buffer[SIZE];

/*------------------------------------------------------------------------------
		implementation code
------------------------------------------------------------------------------*/

/*** START HTTP server ***/
//static void serverWakeup(uint16_t ev, uint16_t conn)
//{
//    char * body;
//    uint32_t read = 0;
//
//    if(ev & EV_HTTP_CON)
//    {
//        dbg("New connection received\n");
//        pico_http_server_accept();
//
//    }
//
//    if(ev & EV_HTTP_REQ) /* new header received */
//    {
//        char *resource;
//        int method;
//        dbg("Header request received\n");
//        resource = pico_http_getResource(conn);
//        if(strcmp(resource, "/") == 0)
//        {
//            resource = "/lpc1830_xplorer.html";
//        }
//        method = pico_http_getMethod(conn);
//
//        if(strcmp(resource, "/board_info") == 0)
//        {
//            pico_http_respond(conn, HTTP_RESOURCE_FOUND);
//            strcpy(http_buffer, "{\"uptime\":");
//            pico_itoa(PICO_TIME(), http_buffer + strlen(http_buffer));
//
//            strcat(http_buffer, ", \"led1_state\":\"");
//            strcat(http_buffer, (led1_state ? "on" : "off"));
//
//            strcat(http_buffer, "\", \"led2_state\":\"");
//            strcat(http_buffer, (led2_state ? "on" : "off"));
//
//            strcat(http_buffer, "\", \"filesize\":");
//            if(length_max == 0)
//                length_max = 1;
//            pico_itoa(length_max, http_buffer + strlen(http_buffer));
//
//            strcat(http_buffer, ", \"progress\":");
//            if(http_not_found)
//            {
//                strcat(http_buffer, "-1");
//            }
//            else
//            {
//                if(download_progress == 0)
//                    download_progress = 1;
//                pico_itoa(download_progress, http_buffer + strlen(http_buffer));
//            }
//
//            strcat(http_buffer, "}");
//            pico_http_submitData(conn, http_buffer, strlen(http_buffer));
//        }
//        else if(strcmp(resource, "/ip") == 0)
//        {
//            pico_http_respond(conn, HTTP_RESOURCE_FOUND);
//
//            struct pico_ipv4_link * link;
//            link = pico_ipv4_link_by_dev(pico_dev_eth);
//            if (link)
//                pico_ipv4_to_string(http_buffer, link->address.addr);
//            else
//                sprintf(http_buffer, "0.0.0.0");
//            pico_http_submitData(conn, http_buffer, strlen(http_buffer));
//        }
//        else if(strcmp(resource, "/download") == 0 && method == HTTP_METHOD_POST)
//        {
//            const char download_url_field [] = "url=";
//            char *download_url = NULL;
//            //char *download_basename = NULL;
//            char *decoded_download_url = NULL;
//            char *http_body = NULL;
//
//            http_body = pico_http_getBody(conn);
//            if(http_body != NULL)
//            {
//                download_url = strstr(http_body, download_url_field);
//                if(download_url != NULL)
//                {
//                    download_url = download_url + strlen(download_url_field);
//                    decoded_download_url = pico_zalloc(strlen(download_url) + 1);
//                    pico_http_url_decode(decoded_download_url, download_url);
//                    dbg("Download url: %s\n", decoded_download_url);
//
//                    if(pico_http_client_open(decoded_download_url, wget_callback) < 0)
//                    {
//                        dbg(" error opening the url : %s, please check the format\n", decoded_download_url);
//                        pico_http_respond(conn, HTTP_RESOURCE_NOT_FOUND);
//                    }
//
//                    pico_free(decoded_download_url);
//
//                    pico_http_respond(conn, HTTP_RESOURCE_FOUND);
//                    strcpy(http_buffer, "Download started");
//                    pico_http_submitData(conn, http_buffer, (uint16_t)strlen(http_buffer));
//                }
//                else
//                {
//                    dbg("no download url\n");
//                    pico_http_respond(conn, HTTP_RESOURCE_NOT_FOUND);
//                }
//            }
//            else
//            {
//                dbg("no http body\n");
//                pico_http_respond(conn, HTTP_RESOURCE_NOT_FOUND);
//            }
//        }
//        else /* search in flash resources */
//        {
//            struct Www_file * www_file;
//            www_file = find_www_file(resource + 1);
//            if(www_file != NULL)
//            {
//                uint16_t flags;
//                flags = HTTP_RESOURCE_FOUND | HTTP_STATIC_RESOURCE;
//                if(www_file->cacheable)
//                {
//                    flags = flags | HTTP_CACHEABLE_RESOURCE;
//                }
//                pico_http_respond(conn, flags);
//                pico_http_submitData(conn, www_file->content, (int) *www_file->filesize);
//            } else { /* not found */
//                /* reject */
//                dbg("Rejected connection...\n");
//                pico_http_respond(conn, HTTP_RESOURCE_NOT_FOUND);
//            }
//        }
//
//    }
//
//    if(ev & EV_HTTP_PROGRESS) /* submitted data was sent */
//    {
//        uint16_t sent, total;
//        pico_http_getProgress(conn, &sent, &total);
//        dbg("Chunk statistics : %d/%d sent\n", sent, total);
//    }
//
//    if(ev & EV_HTTP_SENT) /* submitted data was fully sent */
//    {
//        dbg("Last chunk post !\n");
//        pico_http_submitData(conn, NULL, 0); /* send the final chunk */
//    }
//
//    if(ev & EV_HTTP_CLOSE)
//    {
//        dbg("Close request: %p\n", conn);
//        if (conn)
//            pico_http_close(conn);
//        else
//            printf(">>>>>>>> Close request w/ conn=NULL!!\n");
//    }
//
//    if(ev & EV_HTTP_ERROR)
//    {
//        dbg("Error on server: %p\n", conn);
//        //TODO: what to do?
//        //pico_http_close(conn);
//    }
//}
/* END HTTP server */

static void picoTickTask(void) {

	struct pico_ip4 my_ip;
	struct pico_ip4 netmask;

	pico_string_to_ipv4("192.168.5.10", &my_ip.addr);
	pico_string_to_ipv4("255.255.255.0", &netmask.addr);

	DEBUG_PRINT("picoTest start!\n");

	pico_stack_init();

	pico_dev = (struct pico_device *) pico_tun_create("tun0");
	if (!pico_dev) {
		DEBUG_PRINT("Error creating pico device, exiting...\n");
		exit(1);
	}

	//pico_http_server_start(0, serverWakeup);

	for (;;) {
		pico_stack_tick();
		PICO_IDLE();
	}
}

int main() {
	picoTickTask();

	return 0;
}
