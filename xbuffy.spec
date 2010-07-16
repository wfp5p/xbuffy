Summary:  X-based multiple mailbox biff
Name: xbuffy
Version: 3.6
Release: 1%{?dist}
License: X Consortium
Group: X11/Utilities
Source: xbuffy-3.6.tar.gz
BuildRequires: libHX-devel >= 3.4
BuildRequires: autoconf automake
Requires: libHX >= 3.4
Buildroot: %{_tmppath}/%{name}-root

%description
Xbuffy is a program that watches multiple mailboxes and newsgroups
and displays a count of new mail or news, and optionally displays a pop-up
window containing the From: and Subject: lines when new mail or news
arrives.  Xbuffy can also run a program (such as a xterm with your mail reader)
when you click on the mailbox.

%prep
%setup
autoreconf -fi

%build
%configure
make
%install
install -D -s -m 755 xbuffy $RPM_BUILD_ROOT/usr/bin/xbuffy
install -D -m 644 xbuffy.1 $RPM_BUILD_ROOT/usr/share/man/man1/xbuffy.1x
install -D -m 644 XBuffy.ad $RPM_BUILD_ROOT/usr/share/X11/app-defaults/XBuffy
%files
%doc ChangeLog README README.imap README.cclient boxfile.fmt boxfile.sample

/usr/bin/xbuffy
/usr/share/man/man1/xbuffy.1x.gz
/usr/share/X11/app-defaults/XBuffy

