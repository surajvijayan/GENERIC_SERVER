/*
   Copyright 2013 Broadcom Corporation

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/*! \mainpage
*
*<h3>Table of Contents</h3> 
*@ref IN
<br> 
*@ref FE 
<br> 
*@ref MK
<br> 
*@ref WI
<br> 
*@ref LI
<br> 
*@ref SE
<br> 
*@ref AR
<br> 
*@ref mp
<br> 
*@ref au
<br> 
*@ref li
<br> 
*
*\section IN Introduction
*
*   GENERIC_SERVER is a cross-platform, pluggable, extensible, secure framework for deploying C++
*   plug-ins. Multi-threaded framework can support multiple plug-ins. GENERIC_SERVER 
*   makes extensive use of pluggable shared libraries to 'plugin' specific functionality to the 
*   framework. Framework dynamically loads,unloads plug-ins and dispatches each plug-in in a separate
*   thread.
*
*   Plug-ins would just focus on implementing business logic, framework handles plug-in lifecycle
*   management, network management, threads management and provides a host of other functionality that
*   any plug-in could utilitze.
*   
*
*\section FE Features
*
*\li    Built-in support for TLS encryption. A flag in generic_server.conf file will enable 
*       or disable TLS/SSL encryption for a specific plug-in. X509 Certificates installed 
*       under CERT sub-directory are used for TLS/SSL implementation
*
*\li	All plug-ins are derived classes from a base class GENERIC_PLUGIN.
*	Lot of functionality required for plug-ins are implemented in GENERIC_PLUGIN	
*
*\li    Plug-in specific conf files. Configuration parameters specific for a plug-in should be configured 
*       in a conf file residing in plug-in specific directory
*
*\li    Multiple instances of framework can be running simultaneously, serving different group of plug-ins. Each
*	instance would have a different process name
*
*\li	Framework managed log file, with log file rollover capability. Log can be redirected to 'syslog' on Linux and to Eventlog on Windows
*
*\li	Framework can authenticate all clients via X509 certificates. VERIFY_CLIENT option in main framework conf 
* 	file can be used to
*       disable/enable TLS certificate validation of client. Default is to disable TLS
*       certificate validation
*
*\li	Framework can authorize all clients by enforcing clients to send <PLUGIN_NAME> as part of request payload. 
*   	This is done by setting VALIDATE_PLUGIN flag to true for a specific  plug-in in generic server configuration file.
*
*\li	Framework can support / service multiple plug-ins of same plug-in_type on one port. This 
*	is termed as an 'Alias' plug-in.	
*	An alias plug-in should be of same plug-in_type, have plugin shared library
*	path and TLS flag as the primary plug-in. The first plug-in in
*	generic_server.conf file for any plug-in_type is considered as the primary
*	plug-in.
*
*<BR>
*<BR>
*\section MK Building GENERIC_SERVER
*\subsection BL Linux
*	GENERIC_SERVER has been packaged as an Autoconf package. You could just:
<PRE>
*	$ ./configure
*	$ make
*	$ make install
</PRE>
*	to build and install the framework. The following components would be built:
*\li libgeneric_plugin.1.0.so
*<BR>
*	 This is the shared library for generic_plugin. 
*<BR>
*<BR>
*\li generic_server
*<BR>
*	 This is the generic_server executable.
*<BR>
*<BR>
*\li libsample.1.0.so
*<BR>
*    This is the shared library for sample plug-in.
*<BR>
*<BR>
*\li sample_client
*<BR>
*    This is a sample client program that can be used to interface with GENERIC_SERVER.
*<BR>
*<BR>
*\subsection BW	Windows
*	A Visual Studio solution is available to build GENERIC_SERVER on Windows. This is generic_server.sln
*	under WINDOWS/generic_server directory. Visual Studio 2010 has been used to develop
*	and test on Windows. This solution has five projects:
*	\li generic_plugin
*<BR>
*		Builds generic_plugin.dll
*<BR>
*<BR>
*	\li generic_server
*<BR>
*		Builds generic_server.exe
*<BR>
*<BR>
*	\li sample_plugin
*<BR>
*	Builds sample_plugin.dll
*	When building on Windows, please start off by building generic_plugin project. generic_plugin.lib
*	is required by generic_server and sample_plugin projects.
*<BR>
*<BR>
*\li sample_client
*<BR>
*    This is a sample client program that can be used to interface with GENERIC_SERVER.
*<BR>
*<BR>
*\li GenericServer
*<BR>
*	 This is the Windows installer project. It creates an .msi file.
*<BR>
*<BR>
*\section WI WINDOWS server startup
*
*      GENERIC_SERVER is a true Windows service. Generic_Server cleanly handles Windows service events. Service
*      Manager can be used to have fine-grained control over generic_server process.
*
*\subsection IS Install new service
*
*	  Edit generic_server.conf file in install directory and ensure correct path is configured for different files and plug-ins.
*     Assuming framework is installed under C:\generic_server directory, the following command should be used to install GENERIC_SERVER
*     service:
*         C:\generic_server>generic_server -i <SERVICE_NAME> <INSTANCE_SPECIFIC_CONF_FILE>
*
*         Example:
*         C:\generic_server>generic_server -i MAIN_SERVER  generic_server.conf
*
*      Once the above step is done, a service named MAIN_SERVER will be created on Windows.
*
*\subsection  VE  Verify service Installation
*
*      Go to Windows Service Manager and confirm a service named <SERVICE_NAME> is installed.
*
*      Start --> Control Panel --> Administrative Tools --> Services
*
*\subsection TE Test the Server
*
*\li     Go to service manager and start one or more GENERIC_SERVER service(s)
*\li     Use sample_client program to connect to configured port and send request to framework / plug-in
*        being tested. 
<PRE>
*	Example:
*	$ ./sample_client x.x.x.x 60103 TEST_PLUGIN "Hello from sample_client.." 2 1
</PRE>
*\section LI LINUX server startup
*
*	 Run the command:
*	
*	 # generic_server <INSTANCE_NAME> <INSTANCE_SPECIFIC_CONF_FILE>
*
*	 Example:
*	 # generic_server MAIN_SERVER generic_server.conf
*
*<BR>
*<BR>
*\section SE Server configuration
<PRE>
*	A sample server configuration file is shown below:
*
*	LOG_FILE=/tmp/TEST/generic_server.log
*	#LOG_FILE=SYSLOG
*	# This is the seconds interval a thread will do idle wait client
*	TIMEOUT_SECONDS=300
*	# Max. concurrent threads the will framework will spawn for all devices
*	MAX_THREADS=100
*	# next 2 entries are self-explanatory
*	MAX_LOG_SIZE_IN_MB=100
*	MAX_LOGS_SAVED=5
*	# TLS certificate in PEM format, if TLS is enabled for any device
*	SERVER_CERTIFICATE=/tmp/TEST/CERT/generic_server.crt
*	# TLS private_key in PEM format, if TLS is enabled for any device
*	RSA_PRIVATE_KEY=/tmp/TEST/CERT/generic_server.key
*	# CA cert file
*	CA_CERTIFICATE=/tmp/TEST//CERT/CAfile.pem
*	#VERIFY_CLIENT option will validate client TLS certificate. Default is
*	#not to validate certificates. 
*	VERIFY_CLIENT=0
*	# COMMAND_PORT should be unique per instance. Please set any free port 
*	COMMAND_PORT=10000
*	# PLUGINS_PATH set the base DIR of plugins shared library / DLL
*	PLUGINS_PATH=/tmp/TEST/plugins
*	#
*	#PLUGIN_NAME|PLUGIN_TYPE|TCP_PORT|PLUGIN_NUMBER|PLUGIN_SHARED_LIBRARY|TLS_FLAG|PLUGIN_SPECIFIC_CONF_FILE
*	#PLUGIN_SPECIFIC_CONF_FILE and PLUGIN LIB path is relative to server install directory
*	#
*	SAMPLE1|SAMPLE_TYPE|60103|1|sample/libsample.1.0.so|1|sample/sample.conf
*	SAMPLE2|SAMPLE_TYPE|60103|2|sample/libsample.1.0.so|1|sample/sample.conf
*	SAMPLE3|SAMPLE3_TYPE|60105|3|sample3/libsample.1.0.so|1|sample3/sample3.conf
*	SAMPLE4|SAMPLE4_TYPE|60106|4|sample4/libsample.1.0.so|1|sample4/sample4.conf
</PRE>
*<BR>
*<BR>
*\section AR Architecture
*
*	GENERIC_SERVER has the following main components:
*
*\subsection FR	Framework
*
*	This is a singleton class and provides framework functionality. Source files are in: framework directory.
*
*\subsection GE	Generic Plugin 
*
*	This component provides functionality that are common across plug-ins. Source files are in
*	generic_plugin directory. Framework would instantiate and load objects of type GENERIC_PLUGIN 
*	to framework from plugin shared library. Generic_plugin is packaged into a shared library:
*	libgeneric_plugin.so or generic_plugin.dll.
*
*\subsection PL Plug-ins
*
*	Plug-ins are where business logic gets implemented. All plug-ins should derive from class GENERIC_PLUGIN
*	and should have a function named create_instance(). A sample plug-in is in directory: plugins/sample.
*       
*\subsection Control
*
*  Master program that bootstraps GENERIC_SERVER. Source is in control.cpp.
*  Listens and does 'select()'on all sockets configured in generic_server.conf file. 
*  Spawns a thread for each plug-in and runs message loop for that plug-in.
*<BR>
*<BR>
*\section  DY Dynamically re-load plug-ins
*    
*    Generic_Server has been designed to easily enable addition or deletion of
*    plug-ins to the framework without restartng the framework:
*
*\subsection wi    WINDOWS
*
*        Add or remove plug-ins from generic_server.conf file, then go to service manager and 
*        'pause' and 'resume' the service <SERVICE_NAME>. Check out log file to ensure server has
*        picked up the changes.
*
*\subsection lii   Linux
*        
*        Add or remove plug-ins from generic_server conf file and then send signal SIGHUP to 
*        generic_server process via 'kill' command. Check out log file to ensure server has
*        picked up the changes.
*
*\section  mp	Message Structure
*	GENERIC_SERVER has been designed to provide flexibility to plug-ins on messaging structure
*	and protocol. GENERIC_SERVER today implements a very simplified message structure shown below:
*\subsection hdr Message Header
*\li			SIGNATURE        - 4 bytes SIGNATURE for all messages
*\li			PAYLOAD_SIZE     - 4 bytes network byte order 
*\li			DATA             - PAYLOAD_SIZE data
*\subsection subhdr Data payload
*\li			PLUGIN_NAME_LEN  - 4 bytes network byte order (optional)
*\li			PLUGIN_NAME      - Size as defined by <PLUGIN_NAME_LEN> (optional)
*\li			APPLICATION_DATA - PAYLOAD_SIZE-([4+PLUGIN_NAME_LEN])
*
*   If VALIDATE_PLUGIN is set to 1 in  framework configuration file, then all client requests
*	should have PLUGIN_NAME_LEN and PLUGIN_NAME within the message.
*
*\section ssl OpenSSL
*
*\subsection sslwi	WINDOWS
*
*	OpenSSL 1.0 was used to develop and test GENERIC_SERVER for Windows. Please download and install
*	OpenSSL windows development package from: 
*	http://www.shininglightpro.com/download/Win32OpenSSL_Light-1_0_0a.exe
*   Once OpenSSL is installed, please ensure path to OpenSSL include and lib directories is correctly configured in all Visual Studio
*	projects
*\subsection sslli	Linux
*
*	Please use platform specific tools to install OpenSSL development package for Linux.
*
*\section  au	Author
*       This has been developed by @Author Suraj Vijayan suraj@broadcom.com.
*
*\section  li	License
*
*	This program is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	This library is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
*
**/
