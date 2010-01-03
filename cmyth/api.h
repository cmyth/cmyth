//
//  api.h
//  cmyth
//
//  Created by Jon Gettler on 12/17/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

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
