//
//  api.h
//  cmyth
//
//  Created by Jon Gettler on 12/17/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <cmyth/cmyth.h>

@interface cmyth : NSObject {
	cmyth_conn_t control;
	cmyth_proglist_t proglist;
}

-(cmyth*) server:(NSString*) server port: (unsigned short) port;

-(int) protocol_version;
-(cmyth_proglist_t) get_proglist;
-(int) proglist_count:(cmyth_proglist_t) proglist;

@end
