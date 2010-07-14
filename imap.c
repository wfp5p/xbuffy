/* Copyright (c) 1999
 *
 * Thanks to Tim Culver <culver@cs.unc.edu> for doing the work to
 * support cclient.  This code is mostly his with a little mangling
 * by me.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "xbuffy.h"

#ifdef HAVE_CCLIENT

unsigned long ping_updated = 0;
DynObject message_list;
struct boxinfo *CurrentBox;
MAILSTATUS mailstatus;

void AddImapHeaders(stream, msgno, headerString)
	MAILSTREAM *stream;
	unsigned long msgno;
	DynObject headerString;

{
	char From[MAX_STRING], Subject[MAX_STRING];
	int k;

	From[0] = Subject[0] = '\0';

        strcpy(From, "From: ");
	mail_fetchfrom(&From[6], stream, msgno, MAX_STRING - 6 - 2);

        /* trim to actual length */
	for (k = MAX_STRING - 2; k >= 0; k--)
		if (From[k] == '\0' || isspace(From[k]) || k == MAX_STRING - 2)
		{
			From[k] = '\n';
			From[k + 1] = '\0';
		}
		else
		{
			break;
		}


        strcpy(Subject, "Subject: ");
	mail_fetchsubject(&Subject[9], stream, msgno, MAX_STRING - 9 - 2);
	/* no need to trim */
	Subject[MAX_STRING - 2] = '\0';
	strcat(Subject, "\n");

	if (NEWstrlen(From) != 0)
		DynInsert(headerString, ((DynHigh(headerString) > 0) ? (DynSize(headerString)) : 0), From, NEWstrlen(From));

	if (NEWstrlen(Subject) != 0)
		DynInsert(headerString, ((DynHigh(headerString) > 0) ? (DynSize(headerString)) : 0), Subject, NEWstrlen(Subject));

	From[0] = Subject[0] = '\0';

}


/* Subroutine for counting unseen messages or returning their headers. */

int CountIMAPunseen(mailBox, headerString)
	struct boxinfo *mailBox;
	DynObject headerString;
{
	unsigned long j, messages, *msgno_ptr;
	int i;
	static SEARCHPGM *program = NULL;

	/*
	   Use mail_search_full() to discover the set of unseen messages. This
	   sounds like an inefficent way to compute the *number* of unseen messages
	   in the case when headerString==NULL, but I don't think it's any less
	   efficient than any other way. The server still has to look at all the
	   messages to find their flags, but it only has to return their msgnos, so
	   only a little bandwidth waste.
	*/
	/* Assume mailBox->stream is open. */

	message_list = DynCreate(sizeof(unsigned long), 20);
	if (program == NULL)
	{
		program = mail_newsearchpgm();
#if 1
		program->unseen = 1;	/* message not read */
#else
		program->recent = 1;	/* message recent since last parse of mailbox */
#endif
	}

        mail_search_full(mailBox->stream, NIL, program, 0);
#ifdef DEBUG
	printf("mail_search_full()\n");
#endif
	for (i = 0; i < DynSize(message_list); i++)
	{
		msgno_ptr = (unsigned long *) DynGet(message_list, i);
		if (headerString)
			AddImapHeaders(mailBox->stream, *msgno_ptr, headerString);
	}
	/*
	   Update the cached number of seen messages, since right now we know the
	   true number.
	*/
	mailBox->num_seen_estimate = mailBox->stream->nmsgs - DynSize(message_list);
	DynDestroy(message_list);
	return (DynSize(message_list));
}


int CountIMAP(mailBox, headerString, beenTouched)
	struct boxinfo *mailBox;
	DynObject headerString;
	Boolean *beenTouched;
{
	long ping;
	unsigned long j, messages, *msgno_ptr;
	int i;
	static SEARCHPGM *program = NULL;
	int retval;

	if (!mailBox->keepopen)
	{							/* use mail_status() */

		CurrentBox = mailBox;
		ping = mail_status(NIL, mailBox->box,
						   mailBox->origMode ? SA_MESSAGES : SA_UNSEEN);
		if (ping)
		{
			if (headerString)
			{
				mailBox->stream = mail_open(NIL, mailBox->box, OP_READONLY);
				retval = CountIMAPunseen(mailBox, headerString);
				mailBox->stream = mail_close(mailBox->stream);
			}
			else
			{
				retval = mailBox->origMode ? mailstatus.messages : mailstatus.unseen;
			}
		}
		else
		{
			retval = -1;
		}
		CurrentBox = NULL;
		return (retval);

	}
	else
	{							/* keepopen mode */

		ping_updated = 0;

		if (!(mailBox->stream) || !(ping = mail_ping(mailBox->stream)))
		{

			/* server may have hung up on us */
			/* Try to reopen stream. */

			CurrentBox = mailBox;
			mailBox->stream = mail_open(mailBox->stream, mailBox->box, OP_READONLY);
			CurrentBox = NULL;
			if (!mailBox->stream || !mail_ping(mailBox->stream))
			{
				return (-1);
			}

		}

		/* Advance cycle. */

		if (mailBox->countperiod)
			mailBox->cycle = (mailBox->cycle + 1) % (mailBox->countperiod);
		else
			mailBox->cycle = 1;

		/*
		   ping_updated is not as accurate as the mtime comparison for unix
		   files.  Apparently, it is triggered only when the total number of
		   messages changes.
		*/

		*beenTouched = ping_updated;

#ifdef DEBUG
		{
			struct timeval tv;

			gettimeofday(&tv, 0);
			printf("ping %ld  period=%d cycle=%d\n", tv.tv_sec % 1000,
				   mailBox->imapcountperiod, mailBox->cycle);
		}
#endif							/* DEBUG */

		if (mailBox->origMode && headerString)
		{

			/* fetch headers from all messages */
			for (j = 1; j <= mailBox->stream->nmsgs; j++)
				AddImapHeaders(mailBox->stream, j, headerString);

		}

		if (!mailBox->origMode && (headerString || mailBox->cycle == 0))
		{

			CountIMAPunseen(mailBox, headerString);
			/* Reset the cycle to delay next mail_search(). */
			mailBox->cycle = 0;

		}

		/* Return # msgs or # new messages */

		if (mailBox->origMode)
		{

			return (mailBox->stream->nmsgs);

		}
		else
		{

			/*
			   Return the estimate for the number of new messages. Remember
			   that the estimate is correct in any of these situations: -
			   headerString != NULL - imapcountperiod == 0  (user-configurable
			   permanent flag) - cycle == 0  (every n'th call to CountIMAP(),
			   where n=imapcountperiod)
			*/
			if (mailBox->stream->nmsgs < mailBox->num_seen_estimate)
			{
				mailBox->num_seen_estimate = 0;
				/* to avoid returning a negative number */
			}
			return (mailBox->stream->nmsgs - mailBox->num_seen_estimate);

		}
	}
}

/* c-client callbacks */

void mm_searched(MAILSTREAM * stream, unsigned long msgno)
{
	DynAdd(message_list, &msgno);
}

void mm_exists(MAILSTREAM * stream, unsigned long number)
{
#ifdef DEBUG
	printf("mm_exists()\n");
#endif
	ping_updated = 1;
}

void mm_list(MAILSTREAM * stream, int delimiter, char *name, long attributes)
{
}

void mm_expunged(MAILSTREAM * stream, unsigned long number)
{
}

void mm_flags(MAILSTREAM * stream, unsigned long number)
{
}

void mm_lsub(MAILSTREAM * stream, int delimiter, char *name, long attributes)
{
}

void mm_status(MAILSTREAM * stream, char *mailbox, MAILSTATUS * status)
{
	mailstatus = *status;
}

void mm_notify(MAILSTREAM * stream, char *string, long errflg)
{
	mm_log(string, errflg);		/* just do mm_log action */
}

void mm_log(char *string, long errflg)
{
	static char authfail[] = "Can not authenticate";

	switch (errflg)
	{
	case BYE:
	case NIL:					/* no error */
#ifdef DEBUG
		fprintf(stderr, "[%s]\n", string);
#endif
		break;
	case PARSE:				/* parsing problem */
	case WARN:					/* warning */
#ifdef DEBUG
		fprintf(stderr, "%%%s\n", string);
#endif
		break;
	case ERROR:				/* error */
	default:
		fprintf(stderr, "?%s\n", string);
		if (0 == strncmp(string, authfail, strlen(authfail)))
			if (CurrentBox)
			{
				if (CurrentBox->passwd)
					free(CurrentBox->passwd);
				CurrentBox->passwd = NULL;
				fprintf(stderr, "Try again the next time around.\n");
			}
		break;
	}
}

void mm_dlog(char *string)
{
}

void mm_login(NETMBX * mb, char *uname, char *passwd, long trial)
{
	int i, needuname = 0, needpasswd = 0;

	if (trial > 0)
		needpasswd = 1;
	/* First figure out what we have */
	if (*mb->user)
		strcpy(uname, mb->user);
	else if (CurrentBox && CurrentBox->uname)
		strcpy(uname, CurrentBox->uname);
	else
		needuname = 1;
	if (CurrentBox && CurrentBox->passwd)
		strcpy(passwd, CurrentBox->passwd);
	else
		needpasswd = 1;
	/* Now, get credentials from user */
	if (needuname || needpasswd)
	{
		/* make an appropriate prompt */
		if (CurrentBox)
			printf("%s: ", CurrentBox->boxTitle);
		printf("{%s/%s", mb->host, mb->service);
		if (!needuname)
			printf("/user=%s", uname);
		printf("} ");
		/* get credentials */
		if (needuname)
		{
			printf("username: ");
			fgets(uname, 30, stdin);
			for (i = 0; i < 28 && !isspace(uname[i]); i++)
				;
			uname[i] = '\0';
			if (CurrentBox)
			{
				CurrentBox->uname = strdup(uname);
				printf("CurrentBox->uname=%s\n", CurrentBox->uname);
			}
		}
		if (needpasswd)
		{
			fflush(stdout);
			strcpy(passwd, getpass("password: "));
			if (CurrentBox)
				CurrentBox->passwd = strdup(passwd);
		}
	}
}

int critical = NIL;

void mm_critical(MAILSTREAM * stream)
{
	critical = T;
}

void mm_nocritical(MAILSTREAM * stream)
{
	critical = NIL;
}

long mm_diskerror(MAILSTREAM * stream, long errcode, long serious)
{
	return T;
}

void mm_fatal(char *string)
{
	char s[80];

	strncpy(s, string, 79);
	s[79] = '\0';
	fprintf(stderr, "?%s\n", string);
}


#endif
