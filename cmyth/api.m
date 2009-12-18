//
//  api.m
//  cmyth
//
//  Created by Jon Gettler on 12/17/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#include <cmyth.h>
#include <refmem.h>

#import "api.h"


@implementation cmyth

-(cmyth*)server:(NSString*) server
	    port: (unsigned short) port
{
	cmyth_conn_t c;
	char *host = [server UTF8String];
	int len = 16*1024;
	int tcp = 4096;

	if (port == 0) {
		port = 6543;
	}

	if ((c=cmyth_conn_connect_ctrl(host, port, len, tcp)) == NULL) {
		return nil;
	}

	self = [super init];

	if (self) {
		control = c;
	}

	return self;
}

-(void) dealloc
{
	ref_release(control);

	[super dealloc];
}

-(int) protocol_version
{
	return cmyth_conn_get_protocol_version(control);
}

@end
