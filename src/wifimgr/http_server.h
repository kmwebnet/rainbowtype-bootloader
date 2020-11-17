#ifndef HTTP_SERVER_H_INCLUDED
#define HTTP_SERVER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_SERVER_START_BIT_0	( 1 << 0 )


void http_server(void *pvParameters);
void http_server_netconn_serve(struct netconn *conn);
void http_server_set_event_start();

/**
 * @brief gets a char* pointer to the first occurence of header_name withing the complete http request request.
 *
 * For optimization purposes, no local copy is made. memcpy can then be used in coordination with len to extract the
 * data.
 *
 * @param request the full HTTP raw request.
 * @param header_name the header that is being searched.
 * @param len the size of the header value if found.
 * @return pointer to the beginning of the header value.
 */
char* http_server_get_header(char *request, char *header_name, int *len);

#ifdef __cplusplus
}
#endif

#endif
