//
//  api.h
//  cmyth
//
//  Created by Jon Gettler on 12/17/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <cmyth.h>

@interface cmyth : NSObject {
	cmyth_conn_t control;
}

-(cmyth*) server:(NSString*) s port: (unsigned short) p;

-(int) protocol_version;

@end
