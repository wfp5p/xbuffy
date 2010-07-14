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


#ifndef _XBUFFY_H_
#define _XBUFFY_H_

#include "config.h"


#include <unistd.h>
#include <X11/Intrinsic.h>
#include <libHX/deque.h>

#ifdef HAVE_CCLIENT
#include <c-client/mail.h>
#include <c-client/osdep.h>
#endif

#include "patchlevel.h"

#define MAX_STRING 256

#define NEW_MSG 1
#define OLD_MSG 2
#define READ_MSG 4
#define UNKNOWN 8
#define LINEFEED (char) 10

#define X_RESOURCE_CLASS "XBuffy"

enum BoxType_e {
  MAILBOX = 0,
  NNTPBOX,
  CCLIENTBOX,
  NOBOX,
};

typedef enum BoxType_e BoxType_t;

enum BoxNameType_e {NONE = 0, SHORT, LONG, USR, UNDEF};
typedef enum BoxNameType_e BoxNameType_t;

struct articles_s
{
	long firstNum;
	long lastNum;
};

typedef struct articles_s Articles_t;


struct boxinfo {
  Widget w;
  int boxNum;			/* the box number */
  char *box;			/* the box filename or newsgroup */
  BoxType_t type;		/* the box type (mail, news, etc) */

	struct HXdeque *articles; /* for newsgroups, the read pairs  */

  time_t box_mtime;		/* last time read */
  off_t st_size;		/* size of file on last read */
  int n;				/* the number of messages in the box */

  char *command;
  char *audioCmd;
  char *boxTitle;
  int last;
  int pollTime;
  int headerTime;
  int nobeep;
  int origMode;
  char *bgName, *fgName; /* these are only used in the boxfile function */
  Pixel bg,fg;
  BoxNameType_t BoxNameType;

#ifndef HAVE_CCLIENT
#define MAILSTREAM void
#endif

   MAILSTREAM *stream;               /* an IMAP connection */
  char *uname, *passwd;              /* cached auth credentials */
  int keepopen;                      /* keep connection open? */
  unsigned int countperiod;          /* how often to correct the estimate */
  unsigned int cycle;
  unsigned long num_seen_estimate;  /* used to estimate # unseen using only
				       mail_ping() */

};

struct ApplicationData_s
{
    char *mailBoxes;
    char *newsBoxes; /* only used with NNTP */
    Boolean horiz;
    Boolean shortNames;
    Boolean longNames;
    Boolean origMode;
    Boolean nobeep;
    Boolean nofork;
    Boolean center;
    Boolean fill;
    char *pollTime;
    char *headerTime;
    char *audioCmd;
    char *command;
    char *boxFile;
    char *priority;
    Pixel bg, fg;
};

typedef struct ApplicationData_s ApplicationData_t;

#ifdef _AIX
#include <sys/access.h>
#define exists(fname) (access(fname, E_ACC) == 0)
#define canChange(fname) (accessx(fname, (R_ACC | W_ACC), ACC_SELF) == 0)
#else
#define exists(fname) (access(fname, F_OK) == 0)
#define canChange(fname) (access(fname, (R_OK | W_OK)) == 0)
#endif

#define NEWstrlen(s) (s == NULL ? 0 : strlen(s))

extern char *header_cmp();
#endif /* _XBUFFY_H_ */
