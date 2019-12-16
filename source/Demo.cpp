#include <cstring>
#include <fstream>
#include <dirent.h>
#include <switch.h>
#include <vector>
#include <stdio.h>
#include "uct.h"

void demoReadUCPAndWriteSave();
void demoWriteUCPFromConsole();
void showMainInfo();

int main(int argc, char **argv) {
	consoleInit(NULL);
	getPreUsrAcc();
	if (accUid.uid[0] == 0) {
		//If the user presses B at the profile select screen, quit the app
		consoleExit(NULL);
        return 0;
	}
	if (R_FAILED(mntSaveDir())) {
		//Random save loading fails will exist after 5 seconds
        printf("Failed to mount save dir, stopping... in 5 secs\n");
		svcSleepThread(5E9L);
		consoleExit(NULL);
        return 0;
    }
	loadProfilesFromConsole(profs);
	printf("Loading Ultimate Controller Tools...\n");
	showMainInfo();
	
	// Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		if (kDown & KEY_MINUS) {
			demoReadUCPAndWriteSave();
		}
    	if (kDown & KEY_DDOWN) {
    		demoWriteUCPFromConsole();
    	}
        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }
	
	consoleExit(NULL);
	return 0;
}

void showMainInfo() {
	//TODO: Allow reselecting of profiles
	consoleClear();
	printf("Selected User: 0x%llu %llu\n", accUid.uid[1], accUid.uid[0]);
	showProfilesFromMemory(true);
	printf("Press - to demo loading UCPs and writing them to save file.\n");
	printf("Press Down to demo writing a UCP from a console profile.\n");
	printf("Press + to exit.\n\n");
}

void demoWriteUCPFromConsole() {
	int p = selectProfile();
	bool f = false;
	if (p >= 0 ) {
		f = dumpProfileToFile(profs[p]);
	}
	
	showMainInfo();
	printf((f) ? "Sucessfully wrote to file\n" : "Failed to write to file\n");
}

void demoReadUCPAndWriteSave() {
	consoleClear();
	std::vector<CProfile> files;
	DIR* dir = opendir("/uct");
	printf("Dir-listing for '/uct':\n");
	char dirc[PATH_MAX] = "/uct/";
	struct dirent* ent;
	
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
	        tmp = loadProfileFromFile(dirc);
	        files.push_back(tmp);
	        printf("\n");
        }
        memset(dirc, 0, strlen(dirc));
        strncpy(dirc, "/uct/", sizeof(dirc));
    }
	
    closedir(dir);
	
    printf("Overwriting first %d profiles\n", files.size());
	
	for (int i = 0; i < files.size(); i++) {
		printf("Overwriting %s with ", profs[i].getNameAsString().c_str());
		profs[i] = files[i];
		printf("%s\n", profs[i].getNameAsString().c_str());
		dumpProfileToConsole(profs[i].raw, i);
	}
	
	printf("Done, press A to continue");
	
	while (true) {
		hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		if (kDown & KEY_A)
			break;
		consoleUpdate(NULL);
	}
	
	showMainInfo();
}
