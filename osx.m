/*
 * Coder: Mayur Pawashe (Zorg)
 * http://zorg.tejat.net
 */

#import "osx.h"
#import <Cocoa/Cocoa.h>

FILE *getUserDataFile(const char *mode)
{
	FILE *file = NULL;
	NSString *userLibraryPath = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) objectAtIndex:0];
	if (userLibraryPath)
	{
		NSString *appSupportPath = [userLibraryPath stringByAppendingPathComponent:@"Application Support"];
		
		if ([[NSFileManager defaultManager] fileExistsAtPath:appSupportPath])
		{
			NSString *skyCheckersPath = [appSupportPath stringByAppendingPathComponent:@"SkyCheckers"];
			
			if ([[NSFileManager defaultManager] fileExistsAtPath:skyCheckersPath] || [[NSFileManager defaultManager] createDirectoryAtPath:skyCheckersPath
																																 attributes:nil])
			{
				file = fopen([[skyCheckersPath stringByAppendingPathComponent:@"user_data.txt"] UTF8String], mode);
			}
		}
	}
	
	return file;
}

void getDefaultUserName(char *defaultUserName, int maxLength)
{
	NSString *fullUsername = NSFullUserName();
	if (fullUsername)
	{
		int spaceIndex = [fullUsername rangeOfString:@" "].location;
		if (spaceIndex != NSNotFound)
		{
			strncpy(defaultUserName, [[fullUsername substringToIndex:spaceIndex] UTF8String], maxLength);
		}
		else
		{
			strncpy(defaultUserName, [fullUsername UTF8String], maxLength);
		}
	}
}
