
/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 5.1 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 ******************************************************************************/

/**

**/

/** removes first word of string, ie up to first non-white space
	    following a white space! **/

/*
remove_first_word(string)
char *string;
{

	register int loc;

	for (loc = 0; string[loc] != ' ' && string[loc] != '\0'; loc++)
	    ;

	while (string[loc] == ' ' || string[loc] == '\t')
	  loc++;

	move_left(string, loc);
}

*/

static void move_left(char *string, int chars)
{
	/** moves string chars characters to the left DESTRUCTIVELY **/

	register char *source, *destination;

	source = string + chars;
	destination = string;
	while (*source != '\0' && *source != '\n')
		*destination++ = *source++;

	*destination = '\0';
}

void remove_header_keyword(char *string)
{								/** removes a RFC822 header keyword from the string.
								    i.e. removes up to (and including) the first colon,
								    plus any white-space immediately following it.  **/

	register int loc;

	for (loc = 0; string[loc] != ':' && string[loc] != '\0'; loc++)
		;

	if (string[loc] == ':')
	{
		loc++;					/* move beyond the colon */
		while (string[loc] == ' ' || string[loc] == '\t')
			loc++;
	}

	move_left(string, loc);
}
