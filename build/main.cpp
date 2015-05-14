#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "fastcgi.h"

#ifdef __cplusplus 
extern "C" {
#endif

#include "fpm.h"
#include "fpm_request.h"

#ifdef __cplusplus 
}
#endif


#define  PHP_FASTCGI_PM 1

#ifndef PHP_WIN32
struct sigaction act, old_term, old_quit, old_int;
static int request_body_fd;
#endif


#define STDOUT_FILENO 1
static inline size_t sapi_cgibin_single_write(fcgi_request *req, const char *str, uint str_length)
{
	if (fcgi_is_fastcgi()) {
		fcgi_request *request = req;
		int ret = fcgi_write(request, FCGI_STDOUT, str, str_length);
		if (ret <= 0) {
			return 0;
		}
		return ret;
	}
}

static int sapi_cgibin_ub_write(fcgi_request * req, const char *str, uint str_length)
{
	const char *ptr = str;
	uint remaining = str_length;
	size_t ret;

	while (remaining > 0) {
		ret = sapi_cgibin_single_write(req, ptr, remaining);
		if (!ret) {
			return str_length - remaining;
		}
		ptr += ret;
		remaining -= ret;
	}

	return str_length;
}


static void sapi_cgibin_flush(fcgi_request* req)
{
	if (fcgi_is_fastcgi()) {
		fcgi_request *request = req;
		if ( request && !fcgi_flush(request, 0)) {
			
		}
		return;
	}
}

#define SAPI_CGI_MAX_HEADER_LENGTH 1024

static int sapi_cgi_read_post(fcgi_request* req, char *buffer, uint count_bytes)
{
	int read_bytes=0, tmp_read_bytes;

	while (read_bytes < count_bytes) 
	{
		if (fcgi_is_fastcgi()) {

			fcgi_request *request = req;

			/* If REQUEST_BODY_FILE variable not available - read post body from fastcgi stream */
			if (request_body_fd < 0) {
				tmp_read_bytes = fcgi_read(request, buffer + read_bytes, count_bytes - read_bytes);
			} else {
				tmp_read_bytes = read(request_body_fd, buffer + read_bytes, count_bytes - read_bytes);
			}
		} else {
			tmp_read_bytes = read(0, buffer + read_bytes, count_bytes - read_bytes);
		}

		if (tmp_read_bytes <= 0) {
			break;
		}
		read_bytes += tmp_read_bytes;
	}
	return read_bytes;
}

static char * sapi_cgibin_getenv(fcgi_request* req, char *name, size_t name_len)
{
	if (fcgi_is_fastcgi()) {
		fcgi_request *request = req;
		return fcgi_getenv(request, name, name_len);
	}
}

static char * sapi_cgibin_putenv(fcgi_request *req, char *name, char *value )
{
	int name_len;
	if (!name) {
		return NULL;
	}
	name_len = strlen(name);

	if (fcgi_is_fastcgi()) {
		fcgi_request *request = req;
		return fcgi_putenv(request, name, name_len, value);
	}
}

static void sapi_cgi_log_message(fcgi_request* req, char *message)
{

	if (fcgi_is_fastcgi() ) {

		fcgi_request *request = req;

		if (request) {			

			int len = strlen(message);
			char *buf = (char*)malloc(len+2);

			memcpy(buf, message, len);
			memcpy(buf + len, "\n", sizeof("\n"));
			fcgi_write(request, FCGI_STDERR, buf, len+1);
			free(buf);
		} else {
			fprintf(stderr, "%s\n", message);
		}
		/* ignore return code */
	} else

	fprintf(stderr, "%s\n", message);
}

/* init_request_info

  initializes request_info structure

  specificly in this section we handle proper translations
  for:

  PATH_INFO
	derived from the portion of the URI path following 
	the script name but preceding any query data
	may be empty

  PATH_TRANSLATED
    derived by taking any path-info component of the 
	request URI and performing any virtual-to-physical 
	translation appropriate to map it onto the server's 
	document repository structure

	empty if PATH_INFO is empty

	The env var PATH_TRANSLATED **IS DIFFERENT** than the
	request_info.path_translated variable, the latter should
	match SCRIPT_FILENAME instead.

  SCRIPT_NAME
    set to a URL path that could identify the CGI script
	rather than the interpreter.  PHP_SELF is set to this.

  REQUEST_URI
    uri section following the domain:port part of a URI

  SCRIPT_FILENAME
    The virtual-to-physical translation of SCRIPT_NAME (as per 
	PATH_TRANSLATED)

  These settings are documented at
  http://cgi-spec.golux.com/

  Based on the following URL request:
  
  http://localhost/info.php/test?a=b 
 
  should produce, which btw is the same as if
  we were running under mod_cgi on apache (ie. not
  using ScriptAlias directives):
 
  PATH_INFO=/test
  PATH_TRANSLATED=/docroot/test
  SCRIPT_NAME=/info.php
  REQUEST_URI=/info.php/test?a=b
  SCRIPT_FILENAME=/docroot/info.php
  QUERY_STRING=a=b
 
  but what we get is (cgi/mod_fastcgi under apache):
  
  PATH_INFO=/info.php/test
  PATH_TRANSLATED=/docroot/info.php/test
  SCRIPT_NAME=/php/php-cgi  (from the Action setting I suppose)
  REQUEST_URI=/info.php/test?a=b
  SCRIPT_FILENAME=/path/to/php/bin/php-cgi  (Action setting translated)
  QUERY_STRING=a=b
 
  Comments in the code below refer to using the above URL in a request

 */
static void init_request_info( )
{
	/* initialize the defaults */
	/***
	request_info.path_translated = NULL;
	request_info.request_method = NULL;
	request_info.proto_num = 1000;
	request_info.query_string = NULL;
	request_info.request_uri = NULL;
	request_info.content_type = NULL;
	request_info.content_length = 0;
	***/
}

/** Clean up child processes upon exit */

void fastcgi_cleanup(int signal)
{
	fprintf(stderr, "FastCGI shutdown, pid %d\n", getpid());
	sigaction(SIGTERM, &old_term, 0);
	
	/* We should exit at this point, but MacOSX doesn't seem to */
	exit(0);
}

extern char **environ;

static void PrintEnv(fcgi_request* req, char *label, char **envp) 
{
	char text[1024];
	sprintf(text, "%s:<br>\n<pre>\n", label); 
	fcgi_write(req, FCGI_STDOUT, text, strlen(text) ); 

	for ( ; *envp != NULL; envp++) {
		memset(text, 0, 1024);
		sprintf(text, "%s\n", *envp); 
		fcgi_write(req, FCGI_STDOUT, text, strlen(text) );
	}
	memset(text, 0, 1024);
	sprintf(text, "</pre><p>\n");
	fcgi_write(req, FCGI_STDOUT, text, strlen(text) ); 
}


int main(int argc, char *argv[])
{
	int exit_status = 0;
	int max_requests = 500;
	int requests = 0;
	int fastcgi = 0;
	int fcgi_fd = 0;

	fcgi_request request;

	int benchmark = 1;
	int repeats = 10;
	struct timeval start, end;

	char *fpm_config = "/usr/local/src/fpmcrowd/build/fpmcrowd.conf";


	/*** 
	 ignore SIGPIPE in standalone mode so
	 that sockets created via fsockopen()
	 don't kill PHP if the remote site
	 closes it.  in apache|apxs mode apache
	 does that for us!  thies@thieso.net
	***/
	signal(SIGPIPE, SIG_IGN); 

	/***
    -b <address:port>|<port> Bind Path for external FASTCGI Server mode
    -x, --fpm        Run in FastCGI process manager mode.
    -y, --fpm-config <file> Specify alternative path to FastCGI process manager config file.
	***/
	fastcgi = fcgi_is_fastcgi();
	fpm = 1; 

	if (fpm) {
		if (0 > fpm_init(argc, argv, fpm_config)) {
			return -1;
		}

		/*** fcgi_fd is really master process listen socket fd ( fcgi_fd == 0  refer to rfc ) ***/
		fcgi_fd = fpm_run(&max_requests);

		fcgi_set_is_fastcgi(fastcgi = 1);
	}

	if ( fastcgi ) {
		max_requests = 500;
		fprintf(stdout, "child pid is %u\tand listening on socket %u", getpid(), fcgi_fd);
		fcgi_init_request(&request, fcgi_fd);
	}


	gettimeofday(&start, NULL);

	/*** start of FAST CGI loop, initialise FastCGI request structure ***/
	while ( !fastcgi || fcgi_accept_request(&request) >= 0) 
	{
	
#if PHP_FASTCGI_PM
		if (fpm) fpm_request_info();
#endif

#if PHP_FASTCGI_PM
		if (fpm) fpm_request_executing();
#endif
		if (fastcgi) 
		{
			fprintf(stderr, "child '%u' is processing request.\n", getpid() );
		//	fprintf(stderr, "parent pid is %d \n", fpm_globals.parent_pid);
			goto fastcgi_request_done;
		}

fastcgi_request_done:

		/* only fastcgi will get here */
		requests++;
		if (max_requests && (requests == max_requests)) 
		{
       		fcgi_finish_request(&request);

			if ( fcgi_is_fastcgi() ) 
			{
				fcgi_flush(&request, 1);
				fcgi_close(&request, 0, 0);
			}
		}
		else
		{

			char html_text[ 8192 ] = { 0 };
		    sprintf(html_text, "Content-type: text/html\r\n"
		  							"\r\n"
		        				"<title>FastCGI echo</title>"
		        				"<h1>FastCGI echo</h1>\n"
	            				"Request number %d,  Process ID: %d<p>\n", requests, getpid());

			fcgi_write(&request, FCGI_STDOUT, html_text, strlen(html_text));
			PrintEnv(&request, "Request environment", request.envp);
		//	fcgi_write(&request, FCGI_STDOUT, "", 0);

		}
	}

	/***
	  fcgi_accept_request -> fpm_request_accepting -> fpm_request_reading_headers
	  fpm_request_info
	  fpm_request_executing
	fcgi_close
	fpm_request_finished
	***/

	fcgi_shutdown();

	if ( fcgi_in_shutdown() || (max_requests && (requests == max_requests))	) 
	{
		exit_status = 0;
		goto out;
	}
	else 
	{
		exit_status = 255;
	}

out:

	if (benchmark) 
	{
		int sec;
		int usec;
		gettimeofday(&end, NULL);
		sec = (int)(end.tv_sec - start.tv_sec);
		if (end.tv_usec >= start.tv_usec) 
		{
			usec = (int)(end.tv_usec - start.tv_usec);
		} 
		else 
		{
			sec -= 1;
			usec = (int)(end.tv_usec + 1000000 - start.tv_usec);
		}
		fprintf(stdout, "\nElapsed time: %d.%06d sec\n", sec, usec);
	}

	return exit_status;

}

