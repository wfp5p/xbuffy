/*******************************************************************************

     Copyright (c) 1994,1995,1999    William Pemberton (wfp5p@virginia.edu)

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



/*
    xbuffy - Bill's version of the multiple mailbox biff

    Author: Bill Pemberton, wfp5p@virginia.edu

    This is a modified version of xmultibiff 2.0 by:

     John Reardon, Midnight Networks, badger@midnight.com, 1993.

 */



#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <libHX/defs.h>
#include <libHX/init.h>
#include <libHX/string.h>
#include <gmime/gmime.h>
#include "xbuffy.h"
#ifndef MOTIF
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Paned.h>
#else
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/PushB.h>
/*
#define XtNbackground XmNbackground
#define XtNforeground XmNforeground
#define XtNwidth XmNwidth
#define XtNheight XmNheight
#define XtNlabel XmNvalue
*/
#endif


#include "xbuffy.xbm"


void CheckBox(long i);
void TimerBreakPopup(int i);
static int CountUnixMail(struct boxinfo *mailBox,
		  hxmc_t **headerString,
		  Boolean *beenTouched);

void ParseMailPath();
int makeBoxTitle(struct boxinfo *currentBox);
void initBox(char *box, BoxType_t BoxType, int pollTime, int headerTime,
	     BoxNameType_t BoxNameType, char *command, char *audioCmd,
	     char *title, Boolean origMode, Boolean nobeep,
	     char *bgName, char *fgName, int countperiod, Boolean keepopen);
Pixel convertColor(char *colorname, Pixel defValue);

#ifdef HAVE_CCLIENT
int CountIMAP();
extern struct boxinfo *CurrentBox;
#endif

extern int real_from(char *buffer, BoxType_t type);
extern void remove_header_keyword(char *string);
extern void readBoxfile(char *boxFile);

void ButtonDownHandler(Widget w, int *i, XEvent *event, Boolean *cont);
void ButtonUpHandler(Widget w, int *i, XEvent *event, Boolean *cont);
void BreakPopup(Widget w, int i, XEvent *event, Boolean *cont);
void ExecuteCommand(Widget w, long i, XEvent *event, Boolean *cont);
void setBoxColor(struct boxinfo *box, int status);
static void PopupHeader(Widget w, long i, XEvent *event, Boolean *cont);
void UpdateBoxNumber(struct boxinfo *box);



/** globals **/
char versionString[MAX_STRING];
char *programName;
Widget toplevel;
Widget *header;
ApplicationData_t data;
XtAppContext app;
struct HXdeque *boxmap;
struct boxinfo **boxinfo;
int *headerUp;
int nBoxes = 0;
int envPolltime = 0;
int envPriority = 0;
int envHeadertime = 0;
int NNTPinit = 0;
int maxBoxSize = 0;
FILE *NNTP_fIn, *NNTP_fOut;
extern char **environ;

XtResource resources[] = {
    {"mailboxes", "Mailboxes", XtRString, sizeof(String),
    XtOffset(ApplicationData_t *, mailBoxes), XtRString, 0},
    {"nobeep", "Nobeep", XtRBoolean, sizeof(int),
    XtOffset(ApplicationData_t *, nobeep), XtRString, "FALSE"},
    {"nofork", "Nofork", XtRBoolean, sizeof(int),
    XtOffset(ApplicationData_t *, nofork), XtRString, "FALSE"},
    {"horiz", "Horiz", XtRBoolean, sizeof(int),
    XtOffset(ApplicationData_t *, horiz), XtRString, "FALSE"},
    {"command", "Command", XtRString, sizeof(String),
    XtOffset(ApplicationData_t *, command), XtRString, 0},
    {"names", "Names", XtRBoolean, sizeof(int),
    XtOffset(ApplicationData_t *, longNames), XtRString, "FALSE"},
    {"shortnames", "Shortnames", XtRBoolean, sizeof(int),
    XtOffset(ApplicationData_t *, shortNames), XtRString, "FALSE"},
    {"polltime", "Polltime", XtRString, sizeof(String),
    XtOffset(ApplicationData_t *, pollTime), XtRString, "60"},
    {"priority", "Priority", XtRString, sizeof(String),
    XtOffset(ApplicationData_t *, priority), XtRString, "15"},
    {"headertime", "Headertime", XtRString, sizeof(String),
    XtOffset(ApplicationData_t *, headerTime), XtRString, 0},
    {"orig", "Orig", XtRBoolean, sizeof(int),
    XtOffset(ApplicationData_t *, origMode), XtRString, "FALSE"},
    {"audiocmd", "Audiocmd", XtRString, sizeof(String),
    XtOffset(ApplicationData_t *, audioCmd), XtRString, 0},
    {"boxfile", "Boxfile", XtRString, sizeof(String),
    XtOffset(ApplicationData_t *, boxFile), XtRString, 0},
    {"center", "Center", XtRBoolean, sizeof(int),
    XtOffset(ApplicationData_t *, center), XtRString, "FALSE"},
    {"fill", "Fill", XtRBoolean, sizeof(int),
    XtOffset(ApplicationData_t *, fill), XtRString, "FALSE"},
    {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    XtOffset(ApplicationData_t *, fg), XtRString, XtDefaultForeground},
    {XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
    XtOffset(ApplicationData_t *, bg), XtRString, XtDefaultBackground},
#if 0
    {"newsboxes", "Newsboxes", XtRString, sizeof(String),
    XtOffset(ApplicationData_t *, newsBoxes), XtRString, 0},
#endif                          /* NNTP */

};

XrmOptionDescRec options[] = {
    {"-nobeep", "*nobeep", XrmoptionNoArg, "TRUE"},
    {"-nofork", "*nofork", XrmoptionNoArg, "TRUE"},
    {"-center", "*center", XrmoptionNoArg, "TRUE"},
    {"-fill", "*fill", XrmoptionNoArg, "TRUE"},
    {"-horiz", "*horiz", XrmoptionNoArg, "TRUE"},
    {"-names", "*names", XrmoptionNoArg, "TRUE"},
    {"-shortnames", "*shortnames", XrmoptionNoArg, "TRUE"},
    {"-acmd", "*audiocmd", XrmoptionSepArg, 0},
    {"-poll", "*polltime", XrmoptionSepArg, 0},
    {"-priority", "*priority", XrmoptionSepArg, 0},
    {"-header", "*headertime", XrmoptionSepArg, 0},
    {"-orig", "*orig", XrmoptionNoArg, "TRUE"},
    {"-boxfile", "*boxfile", XrmoptionSepArg, 0},
    {"-command", "*command", XrmoptionSepArg, 0},
};

static inline struct boxinfo *getbox(long i)
{
	return boxinfo[i];
}

void CheckBox(long i)
{
    int num = 0;
    Arg args[5];
    int nargs;
    struct boxinfo *currentBox;
    Boolean beenTouched;
    Boolean isIcon = FALSE;

    currentBox = getbox(i);

   switch (currentBox->type)
   {

    case NNTPBOX:
#if 0
        num = CountNNTP(currentBox, NULL, &beenTouched);
#endif
        break;

    case CCLIENTBOX:
#ifdef HAVE_CCLIENT
        num = CountIMAP(currentBox, NULL, &beenTouched);
#endif
        break;

    case MAILBOX:
        num = CountUnixMail(currentBox, NULL, &beenTouched);
        break;

    case NOBOX:break;

   }

    nargs = 0;
    XtSetArg(args[nargs], XtNiconic, &isIcon);


    if ((num > currentBox->n) ||
        ((!currentBox->origMode) && ((num != currentBox->n) || (beenTouched))))
    {

        if ((currentBox->headerTime != 0) && (!isIcon) && (num > 0))
        {
            PopupHeader(currentBox->w, currentBox->boxNum, 0, 0);
        }

        if ((!currentBox->nobeep) && (num > 0))
        {
            if (currentBox->audioCmd != NULL)
            {
                system(currentBox->audioCmd);
            }
            else
            {
                XBell(XtDisplay(currentBox->w), 0);
            }
        }
    }

    if (currentBox->n != num)
    {
        currentBox->n = num;
        UpdateBoxNumber(currentBox);
    }

    if (currentBox->pollTime != 0)
       XtAppAddTimeOut(app,(unsigned long)(currentBox->pollTime * 1000),
		       (XtTimerCallbackProc) CheckBox,
		       (XtPointer) i);
}


void setBoxColor(struct boxinfo *box, int status)
{
   Arg args[5];
   int nargs;

   nargs = 0;
   if (status)
   {
      XtSetArg(args[nargs], XtNbackground, box->fg);
      nargs++;
      XtSetArg(args[nargs], XtNforeground, box->bg);
      nargs++;
   }
   else
   {
      XtSetArg(args[nargs], XtNbackground, box->bg);
      nargs++;
      XtSetArg(args[nargs], XtNforeground, box->fg);
      nargs++;
   }

   XtSetValues(box->w, args, nargs);
}




void UpdateBoxNumber(struct boxinfo *box)
{
    char amt[MAX_STRING];
    char fmtString[MAX_STRING];
    char *ptr;
    int offset;
    Arg args[5];
    int nargs;

#ifdef MOTIF
    XmString label;

#endif

    if (box->boxTitle != NULL)
    {
        sprintf(amt, "%s: %d", box->boxTitle, box->n);
    }
    else
    {
        sprintf(amt, "%d", box->n);
    }

    memset(fmtString, ' ',MAX_STRING);


    if (data.center) /* center implies fill */
   {

    offset = ((maxBoxSize+4) - NEWstrlen(amt))/2;
      if ( (offset >0) && (offset < MAX_STRING) )
         ptr = fmtString+offset;
      else
      {
	 ptr = fmtString;
	 offset = 0;
      }
    strcpy(ptr,amt);
    ptr = fmtString+NEWstrlen(fmtString);
    *ptr = ' ';
    *(ptr+offset+1-(offset%2))='\0';
   }
   else if (data.fill)
   {


   offset = maxBoxSize+4-NEWstrlen(amt);
   strcpy(fmtString,amt);
   ptr = fmtString+NEWstrlen(fmtString);
   while (offset-- >0)
      *ptr++ = ' ';

   *ptr = '\0';
   }
   else
   {
      strcpy(fmtString, amt);
   }


    nargs = 0;

    if (!box->origMode)
    {
        if (box->n > 0)
        {
	   setBoxColor(box,1);
	}
        else
        {
	   setBoxColor(box,0);

        }
    }


#ifdef MOTIF
    label = XmStringCreateSimple(amt);
    XtSetArg(args[nargs], XmNlabelString, label);
    nargs++;
#else
    XtSetArg(args[nargs], XtNlabel, fmtString);
    nargs++;
#endif
    XtSetValues(box->w, args, nargs);

#ifdef MOTIF
    XmStringFree(label);
#endif
}


#ifdef MOTIF
static void dimension_text(char *hdrPtr, int *rows, int *cols)
{
    int curwidth;

    *rows = 0;
    *cols = 0;
    curwidth = 0;
    while (*hdrPtr != '\0')
    {
        if (*hdrPtr == '\n')
        {
            (*rows)++;
            if (curwidth > *cols)
                *cols = curwidth;
            curwidth = 0;
        }
        else
            curwidth++;
        hdrPtr++;
    }

}

#endif

/* event handlers that decides what to do with button clicks */
void ButtonDownHandler(Widget w, int *i, XEvent *event, Boolean *cont)
{
    if (event->xbutton.button == 1)
    {
        PopupHeader(w, *i, event, cont);
    }
    /* don't do anything else for other button clicks */
}


void ButtonUpHandler(Widget w, int *i, XEvent *event, Boolean *cont)
{
    if (event->xbutton.button == 1)
    {
        BreakPopup(w, *i, event, cont);
    }
    else if (event->xbutton.button == 2)
    {
        ExecuteCommand(w, *i, event, cont);
    }
    else if (event->xbutton.button == 3)
    {
       struct boxinfo *currentBox;
       currentBox = getbox(*i);

       setBoxColor(currentBox,0);
    }

}



static void PopupHeader(Widget w, long i, XEvent *event, Boolean *cont)
{
    Arg args[5];
    int nargs;
    Widget tmpCommand;
    Position biff_x, biff_y, root_x, root_y;
    static XtIntervalId timerID;
    static int rootH = 0;
    static int rootW = 0;
    int number = 0;
    static Boolean firstTime = TRUE;
    hxmc_t *mailHeaders = HXmc_meminit(NULL, 0);
        static char *hdrPtr;
    Dimension headerW, headerH;
    struct boxinfo *currentBox;
    Boolean beenTouched;

#ifdef MOTIF
    int rows, cols;

#endif

    currentBox = getbox(i);

    if (rootH == 0)
    {
        rootH = DisplayHeight(XtDisplay(w), DefaultScreen(XtDisplay(w)));
        rootW = DisplayWidth(XtDisplay(w), DefaultScreen(XtDisplay(w)));
    }


    firstTime = FALSE;

     switch (currentBox->type)
     {
      case NOBOX:break;
      case NNTPBOX:
#if 0
	 number = CountNNTP(currentBox, mailHeaders, &beenTouched);
#endif
	 break;


    case CCLIENTBOX:
#ifdef HAVE_CCLIENT
      number = CountIMAP(currentBox, mailHeaders, &beenTouched);
#endif
      break;

    case MAILBOX:
        number = CountUnixMail(currentBox, &mailHeaders, &beenTouched);break;
     }

     hdrPtr = (char *) mailHeaders;

    /* if the number is different, update it */
    currentBox->n = number;
    UpdateBoxNumber(currentBox);

    /* if the number is zero, there's no header, so leave */
    if (currentBox->n == 0)
    {
        return;
    }

    /* if its already up, pop it down, because we must update it */
    if (headerUp[i] == TRUE)
    {
        XtRemoveTimeOut(timerID);
        BreakPopup(0, i, 0, 0);
    }

    /* Calculate Relative position -> Root absolute position */
    nargs = 0;
    XtSetArg(args[nargs], XtNwidth, &biff_x);
    nargs++;
    XtSetArg(args[nargs], XtNheight, &biff_y);
    nargs++;
    XtGetValues(w, args, nargs);
    XtTranslateCoords(w, biff_x, biff_y, &root_x, &root_y);


    header[i] = XtCreatePopupShell(currentBox->boxTitle, transientShellWidgetClass,
                                   currentBox->w, 0, 0);

    nargs = 0;
#ifndef MOTIF
    XtSetArg(args[nargs], XtNlabel, hdrPtr);
    nargs++;
    XtSetArg(args[nargs], XtNhighlightThickness, 0);
    nargs++;

    tmpCommand = XtCreateManagedWidget("popup", commandWidgetClass, header[i], args, nargs);

    XtAddCallback(tmpCommand, XtNcallback, (XtCallbackProc) BreakPopup, (XtPointer) i);
#else
    dimension_text((char *) hdrPtr, &rows, &cols);
    XtSetArg(args[nargs], XmNvalue, hdrPtr);
    nargs++;
    XtSetArg(args[nargs], XmNeditMode, XmMULTI_LINE_EDIT);
    nargs++;
    XtSetArg(args[nargs], XmNeditable, False);
    nargs++;
    XtSetArg(args[nargs], XmNrows, (short) rows);
    nargs++;
    XtSetArg(args[nargs], XmNcolumns, (short) cols);
    nargs++;
    tmpCommand = XmCreateText(header[i], "popup", args, nargs);
    XtManageChild(tmpCommand);
    XtAddCallback(tmpCommand, XmNactivateCallback, (XtCallbackProc) BreakPopup, (XtPointer) i);
#endif

    if (!XtIsRealized(header[i]))
    {
        XtRealizeWidget(header[i]);
    }

    /* see where we should put this thing so its on the screen */
    /* i.e. make sure we can see it */
    nargs = 0;
    XtSetArg(args[nargs], XtNwidth, &headerW);
    nargs++;
    XtSetArg(args[nargs], XtNheight, &headerH);
    nargs++;
    XtGetValues(header[i], args, nargs);
    if (((int) root_x + (int) headerW) >= (rootW - 20))
    {
        root_x = (Position) rootW - ((Position) headerW + 5);
    }
    if (((int) root_y + (int) headerH) >= (rootH - 20))
    {
        root_y = (Position) rootH - ((Position) headerH + 40);
    }
   nargs = 0;
    XtSetArg(args[nargs], XtNx, root_x);
    nargs++;
    XtSetArg(args[nargs], XtNy, root_y);
    nargs++;
    XtSetValues(header[i], args, nargs);

    XtPopup(header[i], XtGrabNone);

    headerUp[i] = TRUE;
    /* free alloc'ed string */

    /* register a routine to pop it down if it was invoked from a routine */
    if (event == 0)
    {
        timerID = XtAppAddTimeOut(app, (currentBox->headerTime * 1000),
                                  (XtTimerCallbackProc) TimerBreakPopup, (XtPointer) i);
    }
}

void TimerBreakPopup(int i)
{
    BreakPopup(0, i, 0, 0);
}

void BreakPopup(Widget w, int i, XEvent *event, Boolean *cont)
{
    if (headerUp[i] != TRUE)
    {
        return;
    }
    XtPopdown(header[i]);
    XtDestroyWidget(header[i]);
    headerUp[i] = FALSE;
}


void ExecuteCommand(Widget w, long i, XEvent *event, Boolean *cont)
{
    struct boxinfo *currentBox;

    currentBox = getbox(i);

    if (currentBox->command != NULL)
    {
        system(currentBox->command);
    }

}


int isLocked(char *mbox)
{

/* right now this is a REAL stupid function, it just looks for a .lock file */

   char *lockfile;
   int retVal;

   lockfile = (char *) malloc( (NEWstrlen(mbox)+15)*sizeof(char));

   strcpy(lockfile, mbox);
   strcat(lockfile, ".lock");

   retVal = exists(lockfile);
   free(lockfile);
   return(retVal);
}

static int CountUnixMail(struct boxinfo *mailBox,
		  hxmc_t **headerString,
		  Boolean *beenTouched)
{
    FILE *fp = 0;
    char buffer[MAX_STRING];
    char *From;
    char *Subject;
    register int count = 0;
    int status = UNKNOWN;
    register Boolean in_header = FALSE;
    struct stat f_stat;

    *beenTouched = FALSE;

    if (isLocked(mailBox->box))
       return (mailBox->n);

    stat(mailBox->box, &f_stat);

    if ((f_stat.st_size != mailBox->st_size) ||
        (f_stat.st_mtime > mailBox->box_mtime))
    {
        mailBox->st_size = f_stat.st_size;
        mailBox->box_mtime = f_stat.st_mtime;
        *beenTouched = TRUE;
    }

    if ((!*beenTouched) && (headerString == NULL))
        return (mailBox->n);

    fp = fopen(mailBox->box, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Could not open mailbox %s\n",mailBox->box);
        return 0;
    }

    while (fgets(buffer, MAX_STRING - 2, fp) != 0)
    {
       long CL = 0L;
       int has_CL = FALSE;

        buffer[MAX_STRING - 1] = '\0';  /* just in case */
        if ((strchr(buffer, '\n') == NULL) && (!feof(fp)))
        {
            int c;

            while ((c = getc(fp)) != EOF && c != '\n'); /* keep reading */
        }

        if ((!in_header) && (real_from(buffer,mailBox->type)))
        {
	    has_CL = FALSE;
            in_header = TRUE;
            status = NEW_MSG;
        }
        else if (in_header)
        {
            if (header_cmp(buffer, "From", NULL))
		    From = g_mime_utils_header_decode_text(buffer);

	   if (header_cmp(buffer, "Content-Length", NULL))
	   {
	      has_CL = TRUE;
	      CL = atol(buffer+15);
	   }

            if (header_cmp(buffer, "Subject", NULL))
		    Subject = g_mime_utils_header_decode_text(buffer);

            if (header_cmp(buffer, "Status", NULL))
            {
                remove_header_keyword(buffer);
                if (*buffer == 'N')
                    status = NEW_MSG;
                else
                    status = READ_MSG;
            }
            else if (buffer[0] == LINEFEED)
            {
#ifdef USE_CONTENT_LENGTH
	       if (has_CL)
	        fseek(fp,CL,SEEK_CUR);
#endif
                in_header = FALSE;
                if ((status == NEW_MSG) || (mailBox->origMode))
                {
                    count++;
                    if (headerString != NULL)
                    {
                        if (NEWstrlen(From) != 0)
				HXmc_strcat(headerString, From);

                        if (NEWstrlen(Subject) != 0)
				HXmc_strcat(headerString, Subject);
                    }
                }
                free(From);
		free(Subject);
            }

        }
    }
    fclose(fp);

    return count;
}


Pixel convertColor(char *colorname, Pixel defValue)
{
   XrmValue namein, pixelout;

   namein.addr = colorname;
   namein.size = NEWstrlen(colorname) + 1;
   pixelout.size = 0;

   XtConvert(toplevel, XtRString, &namein, XtRPixel, &pixelout);

   if (pixelout.size == 0) /* it failed */
      return(defValue);
   else
      return(*(Pixel *)pixelout.addr);
}

void initBox(char *box, BoxType_t BoxType, int pollTime, int headerTime,
	     BoxNameType_t BoxNameType, char *command, char *audioCmd,
	     char *title, Boolean origMode, Boolean nobeep,
	     char *bgName, char *fgName, int countperiod, Boolean keepopen)
{

    struct boxinfo *tempBox;
    int boxSize;

    tempBox = malloc(sizeof(*tempBox));
    memset(tempBox, 0, sizeof(*tempBox));

    HX_strrtrim(box);

#ifdef DEBUG
    fprintf(stderr, "Init Box = *%s*\n", box);
    fprintf(stderr, "nboxes = %i\n", nBoxes);
    fprintf(stderr, "type = %i\n", BoxType);
    fprintf(stderr, "command    = *%s*\n", command);
    fprintf(stderr, "audio = *%s*\n", audioCmd);
    fprintf(stderr, "boxTitle = *%s*\n", title);
    fprintf(stderr, "pollTime = %i  headerTime = %i\n", pollTime, headerTime);
    fprintf(stderr, "nobeep = %i  origMode = %i \n", nobeep, origMode);
    fprintf(stderr, "nametype = %i\n", BoxNameType);
    fprintf(stderr, "countperiod = %i\n", countperiod);
    fprintf(stderr, "keepopen = %i\n\n",keepopen);
#endif

    tempBox->box = HX_strdup(box);
    tempBox->type = BoxType;
    tempBox->boxNum = nBoxes;

    if (BoxType == NNTPBOX)
    	    tempBox->articles = HXdeque_init();

    if ((pollTime < 0) || (pollTime >= 3600))
        tempBox->pollTime = envPolltime;
    else
        tempBox->pollTime = pollTime;

    if ((tempBox->type == NNTPBOX) && (tempBox->pollTime < 180))
        tempBox->pollTime = 180;

    if ((headerTime < 0) || (headerTime >= 60))
        tempBox->headerTime = envHeadertime;
    else
        tempBox->headerTime = headerTime;

    tempBox->BoxNameType = BoxNameType;

    tempBox->boxTitle = HX_strdup(title);

    if (tempBox->BoxNameType == UNDEF)
    {
        if (data.shortNames)
            tempBox->BoxNameType = SHORT;
        if (data.longNames)
            tempBox->BoxNameType = LONG;
    }

    boxSize = makeBoxTitle(tempBox);
    if (boxSize > maxBoxSize)
       maxBoxSize = boxSize;

    tempBox->command = HX_strdup(command);
    tempBox->audioCmd = HX_strdup(audioCmd);
    tempBox->origMode = origMode;
    tempBox->nobeep = nobeep;

    if (bgName != NULL)
       tempBox->bg = convertColor(bgName,data.bg);
    else
       tempBox->bg = data.bg;

    if (fgName != NULL)
       tempBox->fg = convertColor(fgName,data.fg);
    else
       tempBox->fg = data.fg;


    tempBox->box_mtime = tempBox->st_size = 0;

#ifdef HAVE_CCLIENT
    if (BoxType == CCLIENTBOX)
      {
	tempBox->stream = NULL;
        tempBox->uname = tempBox->passwd = NULL;
	tempBox->keepopen = keepopen;

	 if (tempBox->keepopen)
	 {
	    CurrentBox = &tempBox;

	    while (!tempBox->stream)
	       tempBox->stream = mail_open(NIL, tempBox->box, OP_READONLY);

            CurrentBox = NULL;

            if (!tempBox->stream)
            {
	      fprintf(stderr,"Can't open IMAP mailbox %s\n",tempBox->box);
	    }
	 }
	 else
	 {
	   tempBox->stream = NULL;
	 }

	tempBox->num_seen_estimate = 0;
	tempBox->countperiod = countperiod;
	tempBox->cycle = countperiod-1;
      }
#endif

    HXdeque_push(boxmap, tempBox);
    nBoxes++;
}

void ParseMailPath(void)
{
    char *mailPath = 0;
    char *boxes = 0;
    char *str = 0;

    /* get mail path */
    if ((mailPath = getenv("MAILPATH")) != 0)
    {
        boxes = mailPath;
    }
    else if ((mailPath = getenv("MAIL")) != 0)
    {
        boxes = mailPath;
    }
    else if (data.mailBoxes != 0)
    {
        boxes = data.mailBoxes;
    }

    str = (char *) strtok(boxes, ":, ");
    while (str != NULL)
    {
        initBox(str, MAILBOX, envPolltime, envHeadertime, UNDEF, data.command,
                data.audioCmd, NULL, data.origMode, data.nobeep,NULL,NULL,0,0);

        str = (char *) strtok(NULL, ":, ");
    }
}

#if 0
void ParseNewsPath()
{
    char *newsPath = 0;
    char *boxes = 0;
    char *str = 0;

    /* get nntp path */
    if ((newsPath = getenv("NEWSPATH")) != 0)
    {
        boxes = newsPath;
    }
    else if (data.newsBoxes != 0)
    {
        boxes = data.newsBoxes;
    }

    str = strtok(boxes, ":, ");
    while (str != NULL)
    {
        struct boxinfo tempBox;

        tempBox.boxNum = nBoxes;
        tempBox.box = HX_strdup(str);
        boxInfo[nBoxes].type = NNTPBOX;
        boxInfo[nBoxes].n = CountNNTP(&boxInfo[nBoxes], NULL, NULL);
        nBoxes++;
        str = strtok(NULL, ":, ");
    }
}

#endif                          /* NNTP */




/* the the icon if it is not already loaded */
void LoadIcon(Widget w)
{
    Display *display = XtDisplay(w);
    int screen;
    Pixmap icon_pixmap = (Pixmap) 0;
    Arg arg;

    screen = DefaultScreen(display);

    /* User sets iconPixmap resource, converter does the right thing.. */
    XtSetArg(arg, XtNiconPixmap, &icon_pixmap);
    XtGetValues(w, &arg, 1);
    if (icon_pixmap == (Pixmap) 0)
    {
        XtSetArg(arg, XtNiconPixmap,
                 XCreateBitmapFromData(display,
                                       RootWindow(display, screen),
                                       xbuffy_bits, xbuffy_width,
                                       xbuffy_height));
        XtSetValues(w, &arg, 1);
    }
}


int makeBoxTitle(struct boxinfo *currentBox)
{
    char line[MAX_STRING];

    line[0] = '\0';

    if (currentBox->type == MAILBOX)
    {
        switch (currentBox->BoxNameType)
        {
        case SHORT:
		strcpy(line, HX_basename(currentBox->box));
            break;
        case LONG:
		strcpy(line, currentBox->box);
            break;
        case NONE:
        case USR:
        case UNDEF:
            break;
        }

        if ( (line[0] != '\0') && (!currentBox->boxTitle) )
            currentBox->boxTitle = HX_strdup(line);
    }
    else
    {
        switch (currentBox->BoxNameType)
        {
        case SHORT:
        case LONG:
            strcpy(line, currentBox->box);
            break;
        case NONE:
        case USR:
        case UNDEF:
            break;
        }

        if ( (line[0] != '\0') && (!currentBox->boxTitle) )
            currentBox->boxTitle = HX_strdup(line);
    }

   if ( currentBox->boxTitle != NULL)
	   return NEWstrlen(currentBox->boxTitle);
   else
     return 0;

}


void Usage(void)
{
    printf("Usage: %s [toolkit options] [options] <file> ...\n\n", programName);
    printf("Options are:\n");
    printf("  -help           print this message\n");
    printf("  -version        print the version number\n");
    printf("  -poll <secs>    how often to poll the file(s); default: 60\n");
    printf("  -header <secs>  popup header when mail is received\n");
    printf("                   (use '0' for mouse press only)\n");
    printf("  -acmd <command> command for audio instead of <bell>\n");
    printf("  -boxfile <file> filename containing names of mailboxes\n");
    printf("  -horiz          place the boxes horizontally; default: vertical\n");
    printf("  -nobeep         don't ring bell when mail is received\n");
    printf("  -nofork         don't run in background\n");
    printf("  -names          display full path of mail files in the boxes\n");
    printf("  -shortnames     display names of mail files in the boxes\n");
    printf("  -center         center the names of the boxes\n");
    printf("  -fill           make all the boxes the same size\n");
    printf("  -orig           original mode - display all messages in the boxes\n");
    printf("  -command <cmd>  system command to execute when middle button is pushed\n");
    printf("  -mail <files>   specify a mailbox(s) to watch\n");
#ifdef USE_NNTP
    printf("  -news <groups> specify a newsgroup(s) to watch\n");
#endif                          /* NNTP */
#ifdef HAVE_CCLIENT
    printf("C-Client is enabled.\n");
#endif
    printf("\n");
    printf("If there are any files specified on the command line, it will\n");
    printf("monitor those mail files, otherwise it will use your MAILPATH\n");
    printf("environment variable.\n");
    printf("\n");
}


int main(int argc, char **argv)
{
#ifdef MOTIF
    static String fallback_resources[] = {
        "*.popup.translations: <Btn1Up>:Activate()",
        "*XmPushButton.translations: #override \
      <Btn2Down>:Arm()\n\
      <Btn2Up>:Disarm()",
        (String) NULL
    };

#endif
    static Boolean mailArgs;
    Widget form;
    int i;
    char *check;
    char name[MAX_STRING];
    Arg args[5];
    int nargs;
    int pid;
    int ret;

#ifdef DEBUG
   char pause_string[10];
/*  gets(pause_string);*/
#endif

#ifdef HAVE_CCLIENT
#include <c-client/linkage.c>
#endif

   ret = HX_init();
   if (ret <= 0) {
	   fprintf(stderr, "libHX init failed: %s\n", strerror(errno));
	   exit(1);
   }

    /* initialize program name and version string */
    programName = HX_basename(argv[0]);
    sprintf(versionString, "%s v%s.%s",
            programName, MAJOR_VERSION, MINOR_VERSION);

    mailArgs = TRUE;

    boxmap = HXdeque_init();

    /* init gmime lib */
    g_mime_init(0);

    nargs = 0;
    XtSetArg(args[nargs], XtNallowShellResize, TRUE);
    nargs++;

#ifdef MOTIF
    toplevel = XtAppInitialize(&app, X_RESOURCE_CLASS, options,
             XtNumber(options), &argc, argv, fallback_resources, args, nargs);
#else
    toplevel = XtAppInitialize(&app, X_RESOURCE_CLASS, options,
                               XtNumber(options), &argc, argv, 0, args, nargs);
#endif

    XtGetApplicationResources(toplevel, &data, resources, XtNumber(resources),
                              0, 0);

    /* initialize some values */
    if (data.pollTime != NULL)
        envPolltime = atoi(data.pollTime);

    if ((data.pollTime == NULL) && ((check = getenv("MAILCHECK")) != 0))
    {
        if ((envPolltime = atoi(check)) < 0)
        {
            fprintf(stderr, "MAILCHECK has illegal value\n");
        }
    }

    if ((envPolltime <= 0) || (envPolltime >= 3600))
        envPolltime = 60;

    if (data.priority != NULL)
        envPriority = atoi(data.priority);

    if ((envPriority < 0) || (envPriority >= 20))
        envPriority = 15;

    if (data.headerTime != NULL)
        envHeadertime = atoi(data.headerTime);

    if (envHeadertime <= 0)
        envHeadertime = 0;
    if (envHeadertime >= 60)
        envHeadertime = 60;

    argc--;
    ++argv;
    while (argc)
    {
        if (strcmp("-help", *argv) == 0)
        {
            Usage();
            exit(0);
        }
        else if (strcmp("-version", *argv) == 0)
        {
            printf("%s\n", versionString);
            exit(0);
        }
        else if (strcmp("-news", *argv) == 0)
        {
#ifndef USE_NNTP
            fprintf(stderr, "program not compiled with -DUSE_NNTP ignoring %s\n", *argv);
#else
            mailArgs = FALSE;
#endif                          /* !NNTP */
        }
        else if (strcmp("-mail", *argv) == 0)
        {
            mailArgs = TRUE;
        }
        else
        {
            if (*argv[0] == '-')
            {
                fprintf(stderr, "Bad option: %s\n\n", *argv);
                Usage();
                exit(-1);
            }

            if (mailArgs)
            {
                initBox(*argv, MAILBOX, envPolltime, envHeadertime,
                        UNDEF, data.command, data.audioCmd, NULL,
			data.origMode, data.nobeep, NULL, NULL, 0, 0);

            }

#ifdef USE_NNTP
            else
            {

                initBox(*argv, NNTPBOX, envPolltime, envHeadertime,
                        UNDEF, data.command, data.audioCmd, NULL,
			data.origMode, data.nobeep, NULL, NULL, 0, 0);

            }
#endif                          /* NNTP */

        }
        argc--;
        ++argv;
    }


    if ((data.boxFile != 0) && (nBoxes == 0))
    {
        readBoxfile(data.boxFile);
    }


    if (nBoxes == 0)
    {
        ParseMailPath();
#ifdef USE_NNTP
        ParseNewsPath();
#endif                          /* NNTP */
    }

    /* if there are still no boxes, what's the point? */
    if (nBoxes == 0)
    {
        fprintf(stderr, "nothing to watch is specified\n");
        fprintf(stderr, "check $MAILPATH / XBuffy.mailboxes\n");
#ifdef USE_NNTP
        fprintf(stderr, "check $NEWSPATH / XBuffy.newsboxes\n");
#endif                          /* NNTP */
        Usage();
        exit(-1);
    }

    boxinfo = reinterpret_cast(struct boxinfo **, HXdeque_to_vec(boxmap, NULL));

    LoadIcon(toplevel);

    nargs = 0;
#ifndef MOTIF
    if (data.horiz)
    {
        XtSetArg(args[nargs], XtNorientation, XtorientHorizontal);
        nargs++;
    }
      form = XtCreateManagedWidget("box", boxWidgetClass, toplevel, args, nargs);
/*      form = XtCreateManagedWidget("box", panedWidgetClass, toplevel, args, nargs);*/
#else
    if (data.horiz)
    {
        XtSetArg(args[nargs], XmNorientation, XmHORIZONTAL);
        nargs++;
        XtSetArg(args[nargs], XmNpacking, XmPACK_COLUMN);
        nargs++;
    }
    else
    {
        XtSetArg(args[nargs], XmNorientation, XmVERTICAL);
        nargs++;
        XtSetArg(args[nargs], XmNpacking, XmPACK_TIGHT);
        nargs++;
    }
    XtSetArg(args[nargs], XmNisAligned, True);
    nargs++;
    XtSetArg(args[nargs], XmNentryVerticalAlignment, XmALIGNMENT_CENTER);
    nargs++;

    form = XmCreateRowColumn(toplevel, "box", args, nargs);
    XtManageChild(form);
#endif



    if ((header = (Widget *) malloc(nBoxes * sizeof(Widget))) == 0)
    {
        fprintf(stderr, "Can't malloc header widgets\n");
        exit(-1);
    }
    if ((headerUp = (int *) malloc(nBoxes * sizeof(int))) == 0)
    {
        fprintf(stderr, "Can't malloc header flags\n");
        exit(-1);
    }

    for (i = 0; i < nBoxes; i++)
    {
        Boolean dummy;
	struct boxinfo *currentBox = getbox(i);

        headerUp[i] = FALSE;

        if (currentBox->type == MAILBOX)
		currentBox->n = CountUnixMail(currentBox, NULL, &dummy);

#ifdef USE_NNTP
        if (currentBox->type == NNTPBOX)
		currentBox->n = CountNNTP(currentBox, NULL, &dummy);
#endif

#ifdef HAVE_CCLIENT
        if (currentBox->type == CCLIENTBOX)
		currentBox->n = CountIMAP(currentBox, NULL, &dummy);
#endif

        sprintf(name, "box%d", i);

#ifndef MOTIF
        nargs = 0;
        XtSetArg(args[nargs], XtNleft, XtChainLeft);
        nargs++;
        XtSetArg(args[nargs], XtNright, XtChainLeft);
        nargs++;
        XtSetArg(args[nargs], XtNresizable, True);
        nargs++;
/*7!*/  XtSetArg(args[nargs], XtNshowGrip, False);
        nargs++;
        XtSetArg(args[nargs], XtNallowResize, True);
        nargs++;

        currentBox->w = XtCreateManagedWidget(name, commandWidgetClass, form, args, nargs);


        XtAddEventHandler(currentBox->w, ButtonPressMask, True,
                          (XtEventHandler) ButtonDownHandler,
			  &currentBox->boxNum);
        XtAddEventHandler(currentBox->w,
			  ButtonReleaseMask,
			  True,
                          (XtEventHandler) ButtonUpHandler,
			  &currentBox->boxNum);

#else
        nargs = 0;
        XtSetArg(args[nargs], XmNresizable, TRUE);
        nargs++;
        currentBox->w = XmCreatePushButton(form, name, args, nargs);
        XtManageChild(currentBox->w);
        XtAddEventHandler(currentBox->w, ButtonPressMask, True,
                          ButtonDownHandler, currentBox->boxNum);
        XtAddEventHandler(currentBox->w, ButtonReleaseMask, True,
                          ButtonUpHandler, currentBox->boxNum);

#endif

        UpdateBoxNumber(currentBox);

        CheckBox(i);
    }

#ifdef DEBUG
    fprintf(stderr, "bg = %i, fg = %i maxSize = %i\n", data.bg, data.fg,maxBoxSize);
    for (i = 0; i < nBoxes; i++)
    {
	struct boxinfo *currentBox = getbox(i);
        fprintf(stderr, "box = %s\n", currentBox->box);
        fprintf(stderr, "pollTime = %i\n", currentBox->pollTime);
        fprintf(stderr, "headerTime = %i\n", currentBox->headerTime);
        fprintf(stderr, "origMode = %i\n", currentBox->origMode);
        fprintf(stderr, "nobeep = %i\n", currentBox->nobeep);
        fprintf(stderr, "command = %s\n", currentBox->command);
        fprintf(stderr, "nameType = %i\n", currentBox->BoxNameType);

    }
#endif
    XtRealizeWidget(toplevel);

#ifdef HAS_SETPRIORITY
    if (setpriority(PRIO_PROCESS, 0, envPriority) == -1)
        perror("Proirity change Failed");
#endif

if (!data.nofork)
{
    /* put ourself in the background */
    switch (pid = fork())
    {
    case 0:
        XtAppMainLoop(app);     /* in child do the stuff */
        break;
    case -1:
        perror("Fork failure");
        XtAppMainLoop(app);     /* fork failed - carry on in the parent instead */
        break;
    default:
        exit(0);                /* ok its going we can stop now */
        break;
    }
}
else
    XtAppMainLoop(app);

return 0;


}
