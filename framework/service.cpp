#ifdef WINDOWS
/*
   Copyright 2013 Broadcom Corporation

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License version 2.1  as published by the Free Software Foundation.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <utility>
using namespace std;
#include "windows.h"
#include "time.h"
#include "generic_plugin.h"
#include "generic_server.h"

VOID start_service(LPSTR *argv);
SERVICE_STATUS m_ServiceStatus;
SERVICE_STATUS_HANDLE m_ServiceStatusHandle;
BOOL bRunning=true;
void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
void WINAPI ServiceCtrlHandler(DWORD Opcode);
BOOL InstallService(char *,char *);
BOOL DeleteService(char *);
BOOL notify_server();

int main(int argc, char* argv[])
{
	if(argc > 1)
	{
		if(strcmp(argv[1],"-i")==0)
		{
			if(argc != 4)
			{
				cout << "Usage: generic_server -i <GENERIC_SERVER_NAME> <GENERIC_SERVER_CONF_FILE>" << endl;
				return(1);	
			}
			if(InstallService(argv[2],argv[3]))
				cout << "Service Installed Successfully: " << argv[2] << endl;
			else
				cout << "Error Installing Service: " << argv[2] << endl;
			return(1);	
		}
		else
		if(strcmp(argv[1],"-d")==0)
		{
			if(argc != 3)
			{
				cout << "Usage: generic_server -d <GENERIC_SERVER_NAME>" << endl;
				return(1);	
			}
			if(DeleteService(argv[2]))
				cout << "Service UnInstalled Successfully: " << argv[2] << endl;
			else
				cout << "Error UnInstalling Service: " << argv[2] << endl;
			return(1);	
		}
	}
 	SERVICE_TABLE_ENTRY DispatchTable[] = {{argv[1],ServiceMain},{NULL,NULL}};
	if((StartServiceCtrlDispatcher(DispatchTable)) == 0)
		cout << "Error starting service: " << GetLastError() << endl;
	return 0;
}
/*************************************************************************************/

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	m_ServiceStatus.dwServiceType = SERVICE_WIN32;
	m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_PAUSE_CONTINUE;
	m_ServiceStatus.dwWin32ExitCode = 0;
	m_ServiceStatus.dwServiceSpecificExitCode = 0;
	m_ServiceStatus.dwCheckPoint = 0;
 	m_ServiceStatus.dwWaitHint = 0;

	m_ServiceStatusHandle = RegisterServiceCtrlHandler(argv[0], 
                                            ServiceCtrlHandler); 
	if (m_ServiceStatusHandle == (SERVICE_STATUS_HANDLE)0)
		return;
	if(argc == 0)
	{
		m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(m_ServiceStatusHandle, &m_ServiceStatus);
		return;
	}
	m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	m_ServiceStatus.dwCheckPoint = 0;
	m_ServiceStatus.dwWaitHint = 0;
	SetServiceStatus(m_ServiceStatusHandle, &m_ServiceStatus);
	bRunning=true;
	while(bRunning)
		start_service(argv);
	return;
}
/*************************************************************************************/

void WINAPI ServiceCtrlHandler(DWORD Opcode)
{
  generic_server *fwork = generic_server::instance();
  ostringstream os;
  
  switch(Opcode)
  {
    case SERVICE_CONTROL_PAUSE: 
      m_ServiceStatus.dwCurrentState = SERVICE_PAUSED;
	  SetServiceStatus(m_ServiceStatusHandle,&m_ServiceStatus);
      break;
    
	case SERVICE_CONTROL_CONTINUE:
      m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	  notify_server();	
	  SetServiceStatus(m_ServiceStatusHandle,&m_ServiceStatus);
      break;
    
	case SERVICE_CONTROL_STOP:
      os << fwork->get_cur_time() << " Thr.ID: SERVICE STOP signal received. Shutting down.. ";
      fwork->log(LOG_LOW,LOG_LOW,(char *) os.str().c_str());
      m_ServiceStatus.dwWin32ExitCode = 0;
      m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
      m_ServiceStatus.dwCheckPoint = 0;
      m_ServiceStatus.dwWaitHint = 0;

      SetServiceStatus(m_ServiceStatusHandle,&m_ServiceStatus);
      bRunning=false;
      break;
    
	case SERVICE_CONTROL_INTERROGATE:
	  GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT,0);		

      break; 
  }
  return;
}
/*************************************************************************************/

BOOL notify_server()
{
	SOCKET sock;
	int status;
	ostringstream os;
	struct sockaddr_in sockaddr;
	generic_server *fwork = generic_server::instance();

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr.sin_family = AF_INET;                               /* Internet/IP */
    sockaddr.sin_addr.s_addr = inet_addr((const char *)"127.0.0.1");
    sockaddr.sin_port = (u_short)(u_short)htons(fwork->get_command_port());
    if ( (status = connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) < 0)
    {
        os << fwork->get_cur_time() << " Thr.ID: SERVICE 07 cannot connect to COMMAND_PORT: " << fwork->get_command_port() << " " << status;
        fwork->log(LOG_LOW,LOG_LOW,(char *) os.str().c_str());
        return(0);
    }
	shutdown(sock,2);
#ifdef WINDOWS
	closesocket(sock);
#else
	close(sock);
#endif
	return(0);
}
/*************************************************************************************/

BOOL InstallService(char *service_name,char *conf_file)
{
  char strDir[1024],strExe[1024];
  SC_HANDLE schSCManager,schService;
 
  memset(strExe,0,1024);
  GetCurrentDirectory(1024,strDir);
  strcat(strExe,strDir);
  strcat(strExe,"\\GENERIC_SERVER.exe ");
  strcat(strExe,strDir);
  strcat(strExe,"\\");
  strcat(strExe,conf_file);
  schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

  if (schSCManager == NULL) 
    return false;
  LPCTSTR lpszBinaryPathName=strExe;

  schService = CreateService(schSCManager,service_name, 
        service_name, // GENERIC_SERVER name to display
     SERVICE_ALL_ACCESS, // desired access 
     SERVICE_WIN32_OWN_PROCESS, // GENERIC_SERVER type 
     SERVICE_AUTO_START, // start type 
     SERVICE_ERROR_NORMAL, // error control type 
     lpszBinaryPathName, // GENERIC_SERVER's binary 
     NULL, // no load ordering group 
     NULL, // no tag identifier 
     NULL, // no dependencies
     NULL, // LocalSystem account
     NULL); // no password
  if (schService == NULL)
    return false; 

  CloseServiceHandle(schService);
  return true;
}
/*************************************************************************************/

BOOL DeleteService(char *service_name)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	schSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

	if (schSCManager == NULL)
		return false;
	schService=OpenService(schSCManager,service_name,SERVICE_ALL_ACCESS);
	if (schService == NULL)
		return false;
	if(DeleteService(schService)==0)
		return false;
	if(CloseServiceHandle(schService)==0)
		return false;
return true;
}
/*************************************************************************************/
#endif
