/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 5.1 $   $State: Exp $
 *
 * 			Copyright (c) 1993 USENET Community Trust
 *******************************************************************************/

/* I've deleted a lot of stuff for xbuffy */

#include <stdio.h>
#include <ctype.h>
#include "xbuffy.h"

#ifdef WFP_DEBUG
#include "/home/wfp5p/bin/debug_include/malloc.h"
#endif


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define whitespace(c)    (c == ' ' || c == '\t')


static char *month_name[13] = {
	"jan", "feb", "mar", "apr", "may", "jun",
	"jul", "aug", "sep", "oct", "nov", "dec", NULL
};

static char *day_name[8] = {
	"sun", "mon", "tue", "wed", "thu", "fri", "sat", 0
};

static int len_next_part(char *s)
{
	char *c, quot;

	quot = *s;

	if (quot == '\0')
		return (0);

	if (quot == '\\')
		return (*++s != '\0' ? 2 : 1);

	if (quot != '"')
		return (1);

	for (c = s + 1; *c; c++)
	{
		if (*c == quot)
			return (1 + c - s);

		if (*c == '\\')
		{
			if (*c++)
				c++;
		}
	}

	return (c - s);
}

static int get_word(char *buffer, int start, char *word, int wordlen)
{
	/*
	   Extracts the next white-space delimited word from the "buffer" starting
	   at "start" characters into the buffer and skipping any leading
	   white-space there.  Handles backslash-quoted characters and double-quote
	   bracked strings as an atomic unit.  The resulting word, up to "wordlen"
	   bytes long, is saved in "word".  Returns the buffer index where
	   extraction terminated, e.g. the next word can be extracted by starting
	   at start+<return-val>.  If no words are found in the buffer then -1 is
	   returned.
	*/

	register int len;
	register char *p;

	for (p = buffer + start; isspace(*p); ++p)
		;

	if (*p == '\0')
		return (-1);			/* nothing IN buffer! */

	while (*p != '\0')
	{
		len = len_next_part(p);
		if (len == 1 && isspace(*p))
			break;

		while (--len >= 0)
		{
			if (--wordlen > 0)
				*word++ = *p;
			++p;
		}
	}

	*word = '\0';
	return (p - buffer);
}


#ifdef _TEST
main()
{
	char buf[1024], word[1024], *bufp;
	int start, len;

	while (gets(buf) != NULL)
	{

		puts("parsing with front of buffer anchored");
		start = 0;
		while ((len = get_word(buf, start, word, sizeof(word))) > 0)
		{
			printf("start=%d len=%d word=%s\n", start, len, word);
			start = len;
		}
		putchar('\n');

		puts("parsing with front of buffer updated");
		bufp = buf;
		while ((len = get_word(bufp, 0, word, sizeof(word))) > 0)
		{
			printf("start=%d len=%d word=%s\n", 0, len, word);
			bufp += len;
		}
		putchar('\n');

	}

	exit(0);
}

#endif


int real_from(char *buffer, BoxType_t type)
{

	/*
	   Breakup and validate the "From_" line in the "buffer".  If "entry" is
	   not NULL then the structure is filled in with sender and time
	   information.  Returns TRUE if the "From_" line is valid, otherwise
	   FALSE.

	A valid from line will be in the following format:

	From <user> <weekday> <month> <day> <hr:min:sec> [TZ1 [TZ2]] <year> [remote
	   from sitelist]

	We insist that all of the <angle bracket> fields are present. If two
	   timezone fields are present, the first is used for date information.  We
	   do not look at anything beyond the <year> field. We just insist that
	   everything up to the <year> field is present and valid.
	*/

	char field[255];			/* buffer for current field of line	 */
	int len;					/* length of current field		 */
	int day;
	int i;
	int found;


	/* From */

	if (strncmp(buffer, "From ", 5) != 0)
		goto failed;

	buffer += 5;


	/* <user> */
	if ((len = get_word(buffer, 0, field, sizeof(field))) < 0)
		goto failed;
	buffer += len;

	/* <weekday> */
	if ((len = get_word(buffer, 0, field, sizeof(field))) < 0)
		goto failed;
	else
	{
		found = 0;
		for (i = 0; day_name[i] != NULL; i++)
			found = found || (strncasecmp(day_name[i], field, 3) == 0);
		if (!found)
			goto failed;
	}

	buffer += len;

	/* <month> */
	if ((len = get_word(buffer, 0, field, sizeof(field))) < 0)
		goto failed;
	else
	{
		found = 0;
		for (i = 0; month_name[i] != NULL; i++)
			found = found || (strncasecmp(month_name[i], field, 3) == 0);
		if (!found)
			goto failed;
	}
	buffer += len;

	/* <day> */
	if ((len = get_word(buffer, 0, field, sizeof(field))) < 0 ||
		(day = atoi(field)) < 0 || day < 1 || day > 31)
		goto failed;
	buffer += len;


	/* The line is parsed and valid.  There might be more but we don't care. */
	return TRUE;

failed:
	return FALSE;
}
