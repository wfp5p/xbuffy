/*******************************************************************************

     Copyright (c) 1994,1995    William Pemberton (wfp5p@virginia.edu)

     The X Consortium, and any party obtaining a copy of these files from
     the X Consortium, directly or indirectly, is granted, free of charge, a
     full and unrestricted irrevocable, world-wide, paid up, royalty-free,
     nonexclusive right and license to deal in this software and
     documentation files (the "Software"), including without limitation the
     rights to use, copy, modify, merge, publish, distribute, sublicense,
     and/or sell copies of the Software, and to permit persons who receive
     copies from any such party to do so.  This license includes without
     limitation a license to do the foregoing actions under any patents of
     the party supplying this software to the X Consortium.

*******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "xbuffy.h"

#define cmpTok(tok,str,tokVal) {if ( strcmp(tok,str) == 0) return(tokVal);}
#define checkAndFree(x) {if (x!=NULL) {free(x);x=NULL;}}

extern ApplicationData_t data;

static char *tokens[23] = {
	"box", "title", "command", "audio", "mailbox", "newsbox", "origmode",
	"newmode", "beep", "nobeep", "last", "headertime", "polltime",
        "shortname", "longname", "background", "foreground",
        "cclient", "countperiod", "keepopen", NULL
};

enum TokType
{
	UNK_T = 0, BOX_T, TITLE_T, COMMAND_T, AUDIO_T, MAILBOX_T, NEWSBOX_T,
	ORIGMODE_T, NEWMODE_T, BEEP_T, NOBEEP_T, LAST_T, HEADER_T, POLL_T,
	SHORT_T, LONG_T, BG_T, FG_T, CCLIENT_T, COUNTPERIOD_T, KEEPOPEN_T
};

typedef enum TokType TokenType;

static TokenType token(char *line, char *next)
{
	char tok[30];
	char *p1;
	int x;

	p1 = line;
	x = 0;

	while ((*p1 != '\0') && (isspace(*p1)))
		p1++;

	while ((*p1 != '\0') && (!isspace(*p1)) && (x < 29))
	{
		tok[x++] = (isupper(*p1) ? tolower(*p1) : *p1);
		p1++;
	}
	tok[x] = '\0';

	while ((*p1 != '\0') && (isspace(*p1)))
		p1++;

	strcpy(next, p1);
        if (NEWstrlen(next))
          next[NEWstrlen(next) - 1] = '\0';	/* strip the newline */

	for (x = 0; tokens[x] != NULL; x++)
	{
		if (strcmp(tok, tokens[x]) == 0)
		  return (x + 1);
	}
	return (UNK_T);

}


static void clearBox(struct boxinfo *tempBox)
{
        checkAndFree(tempBox->box);
	checkAndFree(tempBox->command);
	checkAndFree(tempBox->audioCmd);
	checkAndFree(tempBox->boxTitle);
	checkAndFree(tempBox->bgName);
	checkAndFree(tempBox->fgName);
        checkAndFree(tempBox->stream);
        checkAndFree(tempBox->uname);
        checkAndFree(tempBox->passwd);

	tempBox->type = 0;
	tempBox->last = 0;
	tempBox->headerTime = tempBox->nobeep = tempBox->origMode = 0;
	tempBox->pollTime = tempBox->headerTime = -1;
	tempBox->BoxNameType = UNDEF;


/* These only apply to cclient stuff, but don't hurt to clear.... */

        tempBox->keepopen = 0;
        tempBox->countperiod = 0;
        tempBox->cycle = 0;
        tempBox->num_seen_estimate = 0;


}


static char *parseTwiddle(char *str)
{
	static char retVal[MAX_STRING];
	char *ptr, *res;
	char *home;

	ptr = str;
	res = retVal;

	while (*ptr != '\0')
	{
		if (*ptr == '~')
		{
			home = (char *) getenv("HOME");
			strcpy(res, home);
			res += NEWstrlen(home);
		}
		else
			*(res++) = *ptr;

		++ptr;
	}
	*res = '\0';

	return (retVal);
}

static char *parseEnv(char *str)
{
   static char retVal[MAX_STRING];
   char envStr[MAX_STRING];
   char *ptr,*res;
   char *envValue,*envPtr;
   char *lbrace;

   ptr = str;
   envPtr = envStr;
   res = retVal;

   while (*ptr != '\0')
   {
      if (*ptr == '{')
      {
	 lbrace = ptr;
	 ptr++;
	 while ( (*ptr != '\0') && (*ptr != '}') )
	 {
	    *envPtr = *ptr;
	    ptr++;
	    envPtr++;
	 }
	 if (*ptr == '}')
	   ptr++;

	 *envPtr = '\0';
	 envValue = (char *)getenv(envStr);

 	 /* modified by culver: if getenv() returns 0, then just
	    copy over the stuff in {...} */

	 if (envValue) {
	   strcpy(res,envValue);
	   res+=NEWstrlen(envValue);
	 } else {
	   *(res++) = *lbrace;
	   ptr = lbrace+1;
	 }


      }
      else
      {
	 *(res++) = *ptr;
         ++ptr;
      }
   }
   *res = '\0';

   return(retVal);
}


#ifdef TESTBOX

char *showNull(w)
	char *w;

{
	if (w == NULL)
		return ("NULL");
	else
		return (w);
}

void dumpBox(tempBox)
	struct boxinfo tempBox;
{
	printf("Dumping Box = *%s*\n", tempBox.box);
	printf("type = %i\n", tempBox.type);
	printf("command	= *%s*\n", showNull(tempBox.command));
	printf("audio = *%s*\n", showNull(tempBox.audioCmd));
	printf("boxTitle = *%s*\n", showNull(tempBox.boxTitle));
	printf("pollTime = %i  headerTime = %i\n", tempBox.pollTime, tempBox.headerTime);
	printf("nobeep = %i  origMode = %i \n", tempBox.nobeep, tempBox.origMode);
	printf("nametype = %i\n\n", tempBox.BoxNameType);
}

#endif



void readBoxfile(char *boxFile)
{
	struct boxinfo tempBox;
	FILE *boxes;
	char line[MAX_STRING];
	int inBox;
	char next[MAX_STRING];

	tempBox.bgName = tempBox.fgName = tempBox.box = tempBox.command = tempBox.audioCmd = tempBox.boxTitle =tempBox.uname = tempBox.passwd = NULL;
        tempBox.stream = NULL;

	clearBox(&tempBox);

	if ((boxes = fopen(boxFile, "r")) == 0)
	{
		fprintf(stderr, "Could not open boxfile %s\n", boxFile);
		return;
	}

	inBox = 0;

	while (fgets(line, MAX_STRING - 2, boxes) != 0)
	{
	   	line[MAX_STRING - 1] = '\0'; /* just in case */

		if (line[0] == '#')		/* it's a comment */
			continue;

		switch (token(line, next))
		{
		case BOX_T:
			if (inBox)
			{
#ifndef TESTBOX

				initBox(tempBox.box,
						tempBox.type, tempBox.pollTime,
						tempBox.headerTime, tempBox.BoxNameType,
						tempBox.command,
						tempBox.audioCmd,
						tempBox.boxTitle,
						tempBox.origMode, tempBox.nobeep,tempBox.bgName, tempBox.fgName,
					        tempBox.countperiod,
					        tempBox.keepopen);

#else
				dumpBox(tempBox);
#endif
				clearBox(&tempBox);
			}

			tempBox.box = (char *) strdup(parseTwiddle(parseEnv(next)));
			inBox = TRUE;


			break;

		case TITLE_T:
			tempBox.boxTitle = (char *) strdup(next);
			tempBox.BoxNameType = USR;
			break;

		case COMMAND_T:
			tempBox.command = (char *) strdup(next);
			break;

		case AUDIO_T:
			tempBox.audioCmd = (char *) strdup(next);
			break;

		case MAILBOX_T:
			tempBox.type = MAILBOX;
			break;

		case NEWSBOX_T:
			tempBox.type = NNTPBOX;
			break;

	       case CCLIENT_T:
#ifdef HAVE_CCLIENT
		    tempBox.type = CCLIENTBOX;
#else
		   fprintf(stderr, "program not compiled with -DHAVE_CCLIENT, ignoring box\n");
		   tempBox.type = NOBOX;
#endif
  		    break;


	       case COUNTPERIOD_T:
		     tempBox.countperiod = atoi(next);
		     break;

 	       case KEEPOPEN_T:
		      tempBox.keepopen = TRUE;

		case ORIGMODE_T:
			tempBox.origMode = TRUE;
			break;

		case NEWMODE_T:
			tempBox.origMode = FALSE;
			break;

		case BEEP_T:
			tempBox.nobeep = FALSE;
			break;

		case NOBEEP_T:
			tempBox.nobeep = TRUE;
			break;

		case LAST_T:
			tempBox.last = atoi(next);
			break;

		case HEADER_T:
			tempBox.headerTime = atoi(next);
			break;

		case POLL_T:
			tempBox.pollTime = atoi(next);
			break;

		case SHORT_T:
			tempBox.BoxNameType = SHORT;
			break;

		case LONG_T:
			tempBox.BoxNameType = LONG;
			break;

                case BG_T:
		        tempBox.bgName = (char *) strdup(next);
		        break;

	        case FG_T:
		        tempBox.fgName = (char *) strdup(next);
		        break;

		default:
			break;
		}

	}							/* while */

	if (inBox)
#ifndef TESTBOX
		initBox(tempBox.box,
				tempBox.type, tempBox.pollTime,
				tempBox.headerTime, tempBox.BoxNameType,
				tempBox.command,
				tempBox.audioCmd,
				tempBox.boxTitle,
				tempBox.origMode, tempBox.nobeep,tempBox.bgName,tempBox.fgName,
			        tempBox.countperiod,
                                tempBox.keepopen);

#else
		dumpBox(tempBox);
#endif

	fclose(boxes);

}



#ifdef TESTBOX

main()
{

	readBoxfile("boxfile.sample");
}

#endif
