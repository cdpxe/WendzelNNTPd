If you want to know how to build from source under win32
you should take a look into the file called 'INSTALL'.

If you're using WendzelNNTPd (as pre-compiled version) under
Windows you maybe don't know why there are so many different
*.exe files. Here is an overview:

You primary only need to start two files:

	* start_daemon	-  this file starts the server and
			   gives you the possibility to see
			   all error messages if the start
			   fails.

	* WendzelNNTPGUI - This is the GUI used for the
			   administration of the server.

Other binaries:

	* wendzelnntpd	-  The server (you should start it
			   by running start_daemon.bat).

	* wendzelnntpadm - The console based admin tool.

The Win32 version does neither support IPv6 nor thread-
safe strtok_r() due to the limitations of the windoze
environment. I recommend you to use the *nix/Linux/BSD
version.

Please read the LICENSE.txt file and report any kind of
bugs.

You can ask questions on our forms. More information on
http://www.wendzel.de

				--Steffen Wendzel

