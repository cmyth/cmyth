//
//  api.m
//  cmyth
//
//  Created by Jon Gettler on 12/17/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#include <cmyth/cmyth.h>
#include <refmem/refmem.h>

#import "api.h"

@implementation cmythProgram

-(cmythProgram*)program:(cmyth_proginfo_t)program
{
	self = [super init];

	prog = program;

	return self;
}

#define proginfo_method(type)					\
-(NSString*)type						\
{								\
	char *type;						\
	NSString *result = nil;					\
								\
	type = cmyth_proginfo_##type(prog);			\
								\
	if (type != NULL) {					\
		result = [NSString initWithUTF8String:type];	\
		ref_release(type);				\
	}							\
								\
	return result;						\
}

proginfo_method(title)
proginfo_method(subtitle)
proginfo_method(description)
proginfo_method(category)

-(void) dealloc
{
	ref_release(prog);

	[super dealloc];
}

@end

@implementation cmythProgramList

-(cmythProgramList*)control:(cmyth_conn_t)control
{
	cmyth_proglist_t list;
	int i, count;

	if ((list=cmyth_proglist_get_all_recorded(control)) == NULL) {
		return nil;
	}

	self = [super init];

	count = cmyth_proglist_get_count(list);

	array = [[NSMutableArray alloc] init];

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		cmythProgram *program;

		prog = cmyth_proglist_get_item(list, i);
		program = [[cmythProgram alloc] program:prog];

		[array addObject: program];
	}

	return self;
}

-(cmythProgram*)progitem:(int)n
{
	return [array objectAtIndex:n];
}

-(int) count
{
	return [array count];
}

-(void) dealloc
{
	[array release];
	[super dealloc];
}

@end

@implementation cmyth

-(cmyth*) server:(NSString*) server
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
	} else {
		control = NULL;
	}

	return self;
}

-(int) protocol_version
{
	return cmyth_conn_get_protocol_version(control);
}

-(cmythProgramList*)programList
{
	cmythProgramList *list;

	list = [[cmythProgramList alloc] control:control];

	return list;
}

-(void) dealloc
{
	ref_release(control);

	[super dealloc];
}

@end
