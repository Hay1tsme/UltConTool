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
void demoClearProfiles();
void demoShowUCPFileIfo();
int selectProfile();
PadState pad;

int main(int argc, char **argv) {
	consoleInit(NULL);
	
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	
	padInitializeAny(&pad);

	getPreUsrAcc();	
	//If the user presses B on the profile select, quit the application gracefully
	if (accUid.uid[0] == 0) {
		consoleExit(NULL);
        return 0;
	}	
	if (R_FAILED(mntSaveDir())) {
        printf("Failed to mount save dir, closing in 5 seconds...\n");
		svcSleepThread(5E+9L);
		consoleExit(NULL);
        return 0;
    }
	loadProfilesFromConsole(profs);
	
	initNetwork();
	//nxlinkStdio();
	printf("Loading Ultimate Controller Tools...\n");

	showMainInfo();
	
	
	// Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        padUpdate(&pad);

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = padGetButtonsDown(&pad);
		if (kDown & HidNpadButton_AnyUp) demoReadUCPAndWriteSave();
    	if (kDown & HidNpadButton_AnyDown) demoWriteUCPFromConsole();
    	if (kDown & HidNpadButton_AnyLeft) demoClearProfiles();
    	if (kDown & HidNpadButton_AnyRight) demoShowUCPFileIfo();
        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }
	cleanNetwork();
	consoleExit(NULL);
	return 0;
}

/**
 * @details Select from all loaded profiles
 * @return the index of the selected profiles, or -1 if cancled
 */
int selectProfile() {
	int index = 0;
	while(true) {
		consoleClear();
		printf("Select a profile to dump (B to cancel):\n");
		for(int i = 0; i < MAX_PROFILES; i++) {
			if (profs[i].active) {
				if (i == index) {
					printf(">Profile %u: %s\n", i, profs[i].getNameAsString().c_str());
				}
				else {
					printf(" Profile %u: %s\n", i, profs[i].getNameAsString().c_str());
				}
			}
		}
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);
		if ((kDown & HidNpadButton_AnyDown) && index < getLastProfileIndex()) {
			index++;
			while (true) {
				if (!profs[index].active) {
					index++;
				}
				else {
					break;
				}
			}
		}
		if ((kDown & HidNpadButton_AnyUp) && index > 0) {
			index--;
			while (true) {
				if (!profs[index].active) {
					index--;
				}
				else {
					break;
				}
			}
		}
		if (kDown & HidNpadButton_A) {
			return index;	
		}
		if (kDown & HidNpadButton_B) {
			return -1;
		}
		consoleUpdate(NULL);
	}
}

void showMainInfo() {
	consoleClear();
	printf("Selected User: 0x%llu %llu\n", accUid.uid[1], accUid.uid[0]);
	netActive ? printf("Local IP: %s\n", localIpStr.c_str()) :  printf("Networking inactive, no IP\n");
	printf("\n");
	showProfilesFromMemory(true);
	printf("Press UP to demo loading UCPs and writing them to save file.\n");
	printf("Press Down to demo writing a UCP from a console profile.\n");
	printf("Press Left to demo clearing all profiles.\n");
	printf("Press Right to demo viewing UCP info.\n");
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
	DIR* uct = opendir("/uct");
	if (!uct) {
		consoleClear();
		showMainInfo();
		printf("\n\nFailed to open /uct directory.\n");
		return;
	}
	printf("Dir-listing for '/uct':\n");
	char dirc[PATH_MAX] = "/uct/";
	struct dirent* ent;
	
    while ((ent = readdir(uct))) {
        CProfile tmp;
        std::string s = ent->d_name;
        if (ent->d_name == nullptr) {
	        printf("Filename returned Nullptr\n");
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
	
    closedir(uct);

	printf("Press A to write UCPs to save.\n");
	printf("Press B to cancel.\n");
	while (true) {
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);
		if (kDown & HidNpadButton_B) {
			consoleClear();
			showMainInfo();
			return;
		}
		if (kDown & HidNpadButton_A) {
			break;
		}
		consoleUpdate(NULL);
	}
	
    printf("Overwriting first %d profiles\n", files.size());
	
	for (int i = 0; i < files.size(); i++) {
		printf("Overwriting %s with ", profs[i].getNameAsString().c_str());
		profs[i] = files[i];
		printf("%s\n", profs[i].getNameAsString().c_str());
		dumpProfileToConsole(profs[i].raw, i);
	}
	
	printf("Done, press A to continue");
	
	while (true) {
		padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
		if (kDown & HidNpadButton_A)
			break;
		consoleUpdate(NULL);
	}
	
	showMainInfo();
}

void demoClearProfiles() {
	consoleClear();
	//TODO: Button prompt before nuking
	printf("Clearing ALL profiles from memory.\n");
	if (!deleteProfilesFromMemory()) {
		consoleClear();
		showMainInfo();
		printf("\n\nFailed to clear profiles from memory.\n");
		return;
	}
	if (!dumpProfilesToConsole(profs)) {
		consoleClear();
		showMainInfo();
		printf("\n\nFailed to delete profiles from console.\n");
		return;
	}
	printf("Successfully deleted all profiles. Press A to continue\n");
	while (true) {
		padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
		if (kDown & HidNpadButton_A)
			break;
		consoleUpdate(NULL);
	}
	consoleClear();
	showMainInfo();
}

void demoShowUCPFileIfo() {
	std::string ucp = selectUCP(pad); //TODO: Move the input section to this file so I don't have this ugly pad code
	if (ucp.empty()) {
		showMainInfo();
		printf("\nOperation cancled or no files found.");
		return;
	}
	CProfile f = loadProfileFromFile(ucp);
	printf("\nInfo about %s:\nName:%s\nRaw:", ucp.c_str(), f.getNameAsString().c_str());
	for (char x : f.raw) {
		printf("%hX ", x);
	}
	printf("\n\nPress A to continue...");
	while (true) {
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);
		if (kDown & HidNpadButton_A) break;
		consoleUpdate(NULL);
	}
	showMainInfo();
}
