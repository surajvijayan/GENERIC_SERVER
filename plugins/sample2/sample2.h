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
/**
 * \file sample2.h
 * \brief This is a plug-in to demostrate usage of generic_server.
 * This class is derived from GENERIC_PLUGIN, it implements a bunch of virtual functions.
 *
 * \author Suraj Vijayan
 *
 * \date : 2011/12/23 12:16:20
 *
 * Contact: suraj@broadcom.com
 *
 */

//$Author: suraj $

#ifndef SAMPLE2_H_
#define SAMPLE2_H_
#include "generic_plugin.h"

#define PLUGIN_VERSION "1_0_0_0"
#define SET_FILENAME  1
#define FILE_DATA     2
#define  CLOSE_FILE   3
/// Derived from GENERIC_PLUGIN. Implements virtual functions and all plug-in specific functionality.
class sample2_plugin : public generic_plugin
{
private:
	ofstream new_file;
	string ftp_dir,fname; 
public:
	
	sample2_plugin(void);
	sample2_plugin(char *,int);
	int shutdown_plugin(void);
	int plugin_init(int);
	int server_init(void);
	int server_shutdown(void);
	int process_request(void *,void *,unsigned int &);
	int init(void);
	int init(int);
	string get_plugin_version(void);
	int get_plugin_params(string line);
	sample2_plugin &operator=(const sample2_plugin &);
	~sample2_plugin() {};
};
typedef sample2_plugin SAMPLE2;
#endif 
