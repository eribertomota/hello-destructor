/* ----------------------------------------------------------------- *
 * - MailSMTP - Sends a SMTP-message through a SMTP server.        - *
 * - By Andreas Westling <shikaree@update.uu.se>                   - *
 * -                                                               - *
 * - License: Any free license (including Public Domain).          - *
 * -                                                               - *
 * - Remove main() and win32_init() to use the mainsmtp()-function - *
 * - in your application.                                          - *
 * -                                                               - *
 * - BSD/Linux:                                                    - *
 * -  * Just compile.                                              - *
 * -                                                               - *
 * - Solaris:                                                      - *
 * -  * Link with -lsocket -lnsl.                                  - *
 * -                                                               - *
 * - Windows:                                                      - *
 * -  * Add -D_MAILSMTP_WIN32 to compilation command line.         - *
 * -  * Initialize winsock before calling this function. Can be    - * 
 * -    done using this little function:                           - *
 * -      int win32_init() {                                       - *
 * -        WORD wVersionRequested;                                - *
 * -        WSADATA wsaData;                                       - *
 * -        wVersionRequested=MAKEWORD(1,1);                       - *
 * -        if (WSAStartup(wVersionRequested,&wsaData) != 0) {     - *
 * -          return 1;                                            - *
 * -        }                                                      - *
 * -        return 0;                                              - *
 * -      }                                                        - *
 * -  * Remember to call WSACleanup() when you are finished with   - *
 * -    network operations.                                        - *
 * -  * Link with -lwsock32.                                       - *
 * ----------------------------------------------------------------- */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MAILSMTP_WIN32
#  include <windows.h>
#  define uint32_t int
#else
#  include <sys/socket.h>
#  include <netdb.h>
#  include <sys/time.h>
#  include <netinet/in.h>
#endif

#define MAILSMTP_PORT                25	/* Port to connect to. */
#define MAILSMTP_TIMEOUT             10	/* Timeout (seconds). */
#define MAILSMTP_STATUS_OK          250
#define MAILSMTP_STATUS_READY       220
#define MAILSMTP_STATUS_START_INPUT 354

int mailsmtp (char *server, char *from, char *to, char *subject,
	      char *msg);

#ifdef _MAILSMTP_WIN32
int win32_init ()
{
   WORD wVersionRequested;
   WSADATA wsaData;

   wVersionRequested = MAKEWORD (1, 1);
   if (WSAStartup (wVersionRequested, &wsaData) != 0) {
      return 1;
   }
   return 0;
}
#endif

int main (void)
{
#ifdef _MAILSMTP_WIN32
   win32_init ();
#endif
   printf ("Returnvalue: %d\n",
	   mailsmtp ("nonexisting-mailserver.com",
		     "user@nonexisting-mailserver.com",
		     "anotheruser@nonexisting-mailserver.com",
		     "This is a subject", "This is a Testmail"));

#ifdef _MAILSMTP_WIN32
   WSACleanup ();
#endif

   return 0;
}

/* ----------------------------------------------------------------- *
 * - mailsmtp(): char ptr * char ptr * char ptr * char ptr         - *
 * - * char ptr -> int                                             - *
 * -                                                               - *
 * - Sends a SMTP mail message.                                    - *
 * -                                                               - *
 * -       Input:          server - SMTP server to use.            - *
 * -                         from - Email-address to send from.    - *
 * -                           to - Email-address to send to.      - *
 * -                      subject - The subject. May be NULL.      - *
 * -                          msg - The message. May be NULL.      - *
 * -      Output:           (int) - 0 = Successful.                - *
 * -                                1 = Unable to contact SMTP     - *
 * -                                    server.                    - *
 * -                                2 = SMTP server refused to     - *
 * -                                    accept message.            - *
 * -                                3 = Argument error.            - *
 * -                                4 = Out of memory.             - *
 * ----------------------------------------------------------------- */
int mailsmtp (char *server, char *from, char *to, char *subject,
	      char *msg)
{
   struct sockaddr_in saddr;
   struct hostent *he;
   struct timeval tv;
   char *domain,
    *buf;
   int s,
     bufsize,
     len = 0,
      state,
      err;
   fd_set fd;

   saddr.sin_family = AF_INET;
   saddr.sin_port = htons (MAILSMTP_PORT);

   if (!server || !from || !to) {
      return 3;
   }

   /* Only 20 needed really but added some extra chars
    * in case of small changes in the code. */
   bufsize = 32 + strlen (to);
   if (subject)
      bufsize += strlen (subject);
   if (msg)
      bufsize += strlen (msg);
   buf = malloc (bufsize);
   if (!buf) {
      return 4;
   }

   domain = strchr (from, '@');
   if (!domain) {
      free (buf);
      return 3;
   }
   domain++;

   /* Lookup IP-address of server. */
   if (!(he = gethostbyname (server))) {
      free (buf);
      return 1;
   }

   if ((s = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
      free (buf);
      return 1;
   }

   saddr.sin_addr.s_addr = *(uint32_t *) he->h_addr_list[0];

   /* Connect to server. */
   if (connect (s, (struct sockaddr *)&saddr, sizeof (saddr)) == -1) {
      free (buf);
      return 1;
   }

   /* Commloop. */
   for (state = 0; state < 6; state++) {

      /* Can we receive from server? */
      FD_ZERO (&fd);
      FD_SET (s, &fd);
      tv.tv_sec = MAILSMTP_TIMEOUT;
      tv.tv_usec = 0;
      if (select (s + 1, &fd, NULL, NULL, &tv) < 1) {
	 shutdown (s, 2);
	 close (s);
	 free (buf);
	 return 2;
      }

      /* Receive status from server and check it. */
      if (recv (s, buf, 3, 0) != 3) {
	 free (buf);
	 return 2;
      }
      buf[3] = 0;
      err = 0;
      switch (state) {
       case 1:
       case 2:
       case 3:
       case 5:
	  /* Ok. */
	  err += atoi (buf) != MAILSMTP_STATUS_OK;
	  break;
       case 0:
	  /* Service ready greeting. */
	  err += atoi (buf) != MAILSMTP_STATUS_READY;
	  break;
       case 4:
	  /* Start mail input. */
	  err += atoi (buf) != MAILSMTP_STATUS_START_INPUT;
       default:
	  break;
      }

      if (err) {
	 shutdown (s, 2);
	 close (s);
	 free (buf);
	 return 2;
      }

      /* Ignore everything after the status-number. */
      do {
	 FD_ZERO (&fd);
	 FD_SET (s, &fd);
	 tv.tv_sec = 0;
	 tv.tv_usec = 10;

	 err = select (s + 1, &fd, NULL, NULL, &tv);
	 if (err < 0) {
	    shutdown (s, 2);
	    close (s);
	    free (buf);
	    return 2;
	 }

	 if (!err) {
	    break;
	 }

	 if (recv (s, buf, 1, 0) < 1) {
	    free (buf);
	    return 2;
	 }
      } while (1);

      /* Can we send to server? */
      FD_ZERO (&fd);
      FD_SET (s, &fd);
      tv.tv_sec = MAILSMTP_TIMEOUT;
      tv.tv_usec = 0;
      if (select (s + 1, NULL, &fd, NULL, &tv) < 1) {
	 shutdown (s, 2);
	 close (s);
	 free (buf);
	 return 2;
      }

      /* Send command according to state. */
      err = 0;
      switch (state) {
       case 0:			/* Send domain hello. */
	  len = sprintf (buf, "HELO %s\x0d\x0a", domain);
	  break;
       case 1:			/* Send sender mail address. */
	  len = sprintf (buf, "MAIL FROM:<%s>\x0d\x0a", from);
	  break;
       case 2:			/* Send receiver mail address. */
	  len = sprintf (buf, "RCPT TO:<%s>\x0d\x0a", to);
	  break;
       case 3:			/* Initiate data transfer. */
	  len = sprintf (buf, "DATA\x0d\x0a");
	  break;
       case 4:			/* Send message header, subject and message. */
	  if (subject && msg) {
	     len =
		sprintf (buf,
			 "To:%s\x0d\x0aSubject:%s\x0d\x0a\x0d\x0a%s\x0d\x0a.\x0d\x0a",
			 to, subject, msg);
	  } else if (msg) {
	     len =
		sprintf (buf,
			 "To:%s\x0d\x0aSubject:\x0d\x0a\x0d\x0a%s\x0d\x0a.\x0d\x0a",
			 to, msg);
	  } else if (subject) {
	     len =
		sprintf (buf,
			 "To:%s\x0d\x0aSubject:%s\x0d\x0a\x0d\x0a.\x0d\x0a",
			 to, subject);
	  }
	  break;
       case 5:			/* Quit transaction. */
	  len = sprintf (buf, "QUIT\x0d\x0a");
	  break;
      }
      err += (send (s, buf, len, 0) != len);

      if (err) {
	 shutdown (s, 2);
	 close (s);
	 free (buf);
	 return 2;
      }
   }

   shutdown (s, 2);
   close (s);
   free (buf);
   return 0;
}
