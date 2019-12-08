#include <cstring>
#include <fstream>
#include <dirent.h>
#include <switch.h>
#include <vector>
#include <stdio.h>
#include "uct.h"

void demoReadUCPAndWriteSave();

int main(int argc, char **argv) {
	bool didWrite = false;
	consoleInit(NULL);
	userID = getPreUsrAcc();
	
	//If the user presses B on the profile select, quit the application gracefully
	if (userID == 0) {
		consoleExit(NULL);
		return 0;
	}
	printf("Loading Ultimate Controller Tools...\n");
	printf("Selected User: 0x%lu %lu\n", (u64)(userID>>64), (u64)userID);	
	loadProfilesFromConsole(profs);
	showProfilesFromMemory();
	printf("Press - to demo loading UCPs and writing them to save file\nPress + to exit");
	
	// Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		if ((kDown & KEY_MINUS) && !didWrite) {
			demoReadUCPAndWriteSave();
			didWrite = true;
		}
        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }
	
	consoleExit(NULL);
	return 0;
}

void demoReadUCPAndWriteSave() {
	std::vector<CProfile> files;
	DIR* dir = opendir("../uct");
	printf("Dir-listing for '../uct':\n");
	char dirc[PATH_MAX] = "/uct/";
	
    while ((ent = readdir(dir)))
    {
        CProfile tmp;
        std::string s = ent->d_name;
        if (ent->d_name == nullptr) {
	        printf("nullptr");
        }
        else if (s.find(".ucp") != std::string::npos) {
	        printf("Found possible UCP file: %s\n", ent->d_name);
	        memcpy(&dirc[5], ent->d_name, s.length());
	        printf(dirc);
	        printf("\n");
	        loadProfileFromFile(tmp, dirc);
	        files.push_back(tmp);
	        printf("\n");
        }
        memset(dirc, 0, strlen(dirc));
        strncpy(dirc, "/uct/", sizeof(dirc));
    }
    closedir(dir);
    printf("Overwriting first %d profiles\n", files.size());
	//profs[0] = files[0];
	for (int i = 0; i < files.size(); i++) {
		printf("Overwriting %s with ", profs[i].getNameAsString().c_str());
		profs[i] = files[i];
		printf("%s\n", profs[i].getNameAsString().c_str());
		dumpProfileToConsole(profs[i].raw, i);
	}
	printf("Done");
}
