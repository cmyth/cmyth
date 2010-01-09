/*
 *  Copyright (C) 2009-2010, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#import <Foundation/Foundation.h>

#include <cmyth/cmyth.h>

@interface cmythProgram : NSObject {
	cmyth_proginfo_t prog;
}

-(cmythProgram*)program:(cmyth_proginfo_t)program;
-(cmyth_proginfo_t)proginfo;
-(NSString*)title;
-(NSString*)subtitle;
-(NSString*)description;
-(NSString*)category;
-(NSString*)pathname;
-(NSString*)date;
-(int)seconds;

@end

@interface cmythFile : NSObject {
	cmyth_conn_t conn;
	cmyth_file_t file;
	int sockfd;
	int portno;
	long long length;
	NSString *vlc_host;
	NSString *vlc_path;
}

-(cmythFile*)openWith:(cmythProgram*)program;
-(cmythFile*)transcodeWith:(cmythProgram*)program vlcHost:(NSString*)host vlcPath:(NSString*)path;

-(void)server;

-(int)portNumber;

@end

@interface cmythProgramList : NSObject {
	NSMutableArray *array;
}

-(cmythProgramList*)control:(cmyth_conn_t)control;
-(cmythProgram*)progitem:(int)n;
-(int) count;

@end

@interface cmyth : NSObject {
	cmyth_conn_t control;
	cmyth_conn_t event;
}

-(cmyth*) server:(NSString*) server port: (unsigned short) port;
-(int) protocol_version;
-(cmythProgramList*)programList;
-(int)getEvent:(cmyth_event_t*)event;

@end
