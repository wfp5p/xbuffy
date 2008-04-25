Summary:  X-based multiple mailbox biff
Name: xbuffy
Version: 3.5
Release: 0
Copyright: X Consortium, copyrighted by author
Group: X11/Utilities
Source: ftp.virginia.edu:/pub/src/xbuffy/xbuffy-3.5.tar.gz
Buildroot: %{_tmppath}/%{name}-root

%description
Xbuffy is a program that watches multiple mailboxes and newsgroups
and displays a count of new mail or news, and optionally displays a pop-up
window containing the From: and Subject: lines when new mail or news
arrives.  Xbuffy can also run a program (such as a xterm with your mail reader)
when you click on the mailbox.  
%prep
%setup
%build
%configure
make
%install
install -D -s -m 755 -o 0 -g 0 xbuffy $RPM_BUILD_ROOT/usr/X11R6/bin/xbuffy
install -D -m 644 -o 0 -g 0 xbuffy.1 $RPM_BUILD_ROOT/usr/X11R6/man/man1/xbuffy.1x
install -D -m 644 -o 0 -g 0 XBuffy.ad $RPM_BUILD_ROOT/usr/X11R6/lib/X11/app-defaults/XBuffy
%files
%doc ChangeLog README README.imap README.cclient boxfile.fmt boxfile.sample

/usr/X11R6/bin/xbuffy
/usr/X11R6/man/man1/xbuffy.1x.gz
/usr/X11R6/lib/X11/app-defaults/XBuffy

