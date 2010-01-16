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

typedef enum {
	CMYTH_TRANSCODE_INVALID = 0,
	CMYTH_TRANSCODE_UNKNOWN,
	CMYTH_TRANSCODE_ERROR,
	CMYTH_TRANSCODE_STARTING,
	CMYTH_TRANSCODE_IN_PROGRESS,
	CMYTH_TRANSCODE_COMPLETE,
	CMYTH_TRANSCODE_CONNECT_FAILED,
	CMYTH_TRANSCODE_STOPPING,
	CMYTH_TRANSCODE_STOPPED,
} cmythTranscodeState;

@interface cmythFile : NSObject {
	cmyth_conn_t conn;
	cmyth_file_t file;
	int sockfd;
	int portno;
	long long length;
	NSString *vlc_host;
	NSString *vlc_path;
	cmythTranscodeState state;
	volatile float progress;
	cmythProgram *program;
	NSString *srcPath;
	NSString *dstPath;
	NSString *vlc;
	NSLock *lock;
	volatile int done;
}

-(cmythFile*)openWith:(cmythProgram*)program;
-(cmythFile*)transcodeWith:(cmythProgram*)program vlcHost:(NSString*)host vlcPath:(NSString*)path;

-(void)server;
-(void)transcoder;
-(void)transcodeStop;

-(int)portNumber;
-(cmythTranscodeState)transcodeState;
-(float)transcodeProgress;

@property (retain,nonatomic) NSLock *lock;

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
