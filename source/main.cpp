#include <cstring>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <switch.h>
#include <vector>
#include <stdio.h>
#include "uct.h"


int main(int argc, char **argv) {
	bool didWrite = false;
	consoleInit(NULL);
	
	userID = getPreUsrAcc();
	std::vector<CProfile> files;
	
	printf("Loading Ultimate Controller Tools...\n");
	printf("Selected User: 0x%lx %lx\n", (u64)(userID>>64), (u64)userID);	
	loadProfilesFromConsole(profs);
	printf("Press - to test file load\n");
	
	// Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		if ((kDown & KEY_MINUS) && !didWrite) {
			
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
			didWrite = true;
			
		}
        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }
	
	consoleExit(NULL);
	return 0;
}

void loadProfilesFromConsole(CProfile *pfs) {
	FILE *save;
	char mem[PROFILE_LEN];
    if (R_FAILED(mntSaveDir())) {
        printf("Failed to mount save dir, stopping...\n");
        return;
    }
    save = fopen("save:/save_data/system_data.bin", "rb");
    if (save == nullptr) {
        printf("ERR! Failed to open system_data.bin for reading\n");
        return;
    }
    for (int i = 0; i < MAX_PROFILES; i++) {
        fseek(save, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * i), SEEK_SET);
        fread(mem, PROFILE_LEN, 1, save);
        if (mem[0] == 1 && mem[1] == 0 && mem[2] == 0 && mem[3] == 0) {
			pfs[i] = CProfile(mem);
            printf("Profile %u, Name: ", i);
            std::cout << pfs[i].getNameAsString() << std::endl;
            numPfs++;
        }        
    }
    printf("Found %u profiles in save.\n", numPfs);
    if (fclose(save) != 0) {
	    printf("fclose failed in loadProfilesFromConsole\n");
    }
	if (R_FAILED(fsdevUnmountDevice("save"))) {
		printf("fsdevUnmountDevice failed in loadProfilesFromConsole\n");
	}
}

void loadProfileFromConsole(CProfile pf, int index) {
	FILE *save;
	char mem[PROFILE_LEN];
	
	if (R_FAILED(mntSaveDir())) {
        printf("Failed to mount save dir, stopping...\n");
        return;
    }
    save = fopen("save:/save_data/system_data.bin", "rb");
	if (save == nullptr) {
        printf("ERR! Failed to open system_data.bin for reading\n");
        return;
    }
	fseek(save, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * index), SEEK_SET);
	fread(mem, PROFILE_LEN, 1, save);
	if (mem[0] == 1 && mem[1] == 0 && mem[2] == 0 && mem[3] == 0) {
		pf = CProfile(mem);
        printf("Found Profile named: %s\n", pf.getNameAsString().c_str());
    }
	else {
		printf("No profile found at index %i (offset %hx)\n", index, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * index));
	}
}

void loadProfileFromFile(CProfile pf, std::string file) {
	char mem[PROFILE_LEN];
	FILE* f = fopen(file.c_str(), "r");
	if (!f) {
		printf("Couldn't open file in loadProfileFromFile");
		return;
	}
	fread(mem, PROFILE_LEN, 1, f);
	//TODO: Sanity checking
	CProfile pfm(mem);
	pf = pfm;
	fclose(f);
}

bool dumpProfileToConsole(char* buffer, int index) {
	//yoinked from https://github.com/WerWolv/EdiZon/blob/d5e4d35d89051c134003ec6d681c10ef2cc8e365/source/helpers/save.cpp#L414
	FsFileSystem fs;

	if (R_FAILED(mntSaveDir())) {
	printf("Failed to mount save.\n");
	fsdevUnmountDevice("save");
	fsFsClose(&fs);
	return false;
	}

	char filePath[0x100];

	strcpy(filePath, "save:/");
	strcat(filePath, "save_data/system_data.bin");


	FILE *file = fopen(filePath, "rb+");

	if (file == nullptr) {
	printf("Failed to open file.\n");
	fsdevUnmountDevice("save");
	fsFsClose(&fs);
	return false;
	}
	
	fseek(file, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * index), SEEK_SET);
	printf("Writing to offset %hX\n", ftell(file));
	fwrite(buffer, PROFILE_LEN, 1, file);
	printf("Wrote to offset, now at %hX\n", ftell(file));
	fclose(file);

	if (R_FAILED(fsdevCommitDevice("save"))) {
	printf("Committing failed.\n");
	return false;
	}

	fsdevUnmountDevice("save");
	fsFsClose(&fs);

	return true;
}

bool dumpProfilesToConsole(char *buffer) {
	//TODO: Fill out
	return 0;
}

bool dumpProfileToFile(char *buffer, CProfile pf) {
	//TODO: Fill out
	return 0;
}

void showProfilesFromConsole() {
	//TODO: Fill out
}

void showProfileFromConsole(int index) {
	//TODO: Fill out
}

void showProfilesFromMemory() {
	//TODO: Fill Out
}

void showProfileFromMemory(int index) {
	//TODO: Fill out
}

void showProfileFromFile() {
	//TODO: Fill out
}

bool checkProfileFromConsole() {
	//TODO: Fill out
	return true;
}

bool checkProfileFromMemory() {
	//TODO: Fill out
	return true;
}

bool checkProfileFromFile() {
	//TODO: Fill out
	return true;
}

Result mntSaveDir() {
	DIR *dir;
	int ret;
	FsFileSystem tmpfs;
	bool found = false;
	printf("Mounting save...\n");
	Result rc = fsMount_SaveData(&tmpfs, TITLE_ID, userID);
	if (R_FAILED(rc)) {
            printf("fsMount_SaveData() failed: 0x%x\n", rc);
        }
	if (R_SUCCEEDED(rc)) {
        ret = fsdevMountDevice("save", tmpfs);
        if (ret==-1) {
            printf("fsdevMountDevice() failed.\n");
            rc = ret;
        }
    }
	if (R_SUCCEEDED(rc)) {
		dir = opendir("save:/save_data");
		if(dir==NULL) {
			printf("Failed to open dir.\n");
		}
		else {
			while ((ent = readdir(dir)) && !found)
		{
			if(strcmp(ent->d_name, "system_data.bin")) {
				printf("Found save file\n");
				found = true;
			}
		}
		if(!found) {
			rc = 1;
			printf("Couldn't find save file.\n");
			}
		}
		closedir(dir);
	}
	return rc;
}

u128 getPreUsrAcc() {
	//from EdiZon https://github.com/WerWolv/EdiZon/blob/2decd3214f2f2187f4f9330909bb0ca662eb1e20/source/guis/gui.cpp#L623
	//Only works if launched as a full app, otherwise black screen til it runs out of memory and crashes the system
	AppletHolder aph;
	AppletStorage ast;
	AppletStorage hast1;
	LibAppletArgs args;
	u8 indata[0xA0] = { 0 };
	
	struct UserReturnData{
	      u64 result;
	      u128 UID;
	  } PACKED;
	
	struct UserReturnData outdata;
	
	indata[0x96] = 1;
	appletCreateLibraryApplet(&aph, AppletId_playerSelect, LibAppletMode_AllForeground);
	libappletArgsCreate(&args, 0);
	libappletArgsPush(&args, &aph);
	appletCreateStorage(&hast1, 0xA0);
	appletStorageWrite(&hast1, 0, indata, 0xA0);
	appletHolderPushInData(&aph, &hast1);
	appletHolderStart(&aph);

	while (appletHolderWaitInteractiveOut(&aph));

	appletHolderJoin(&aph);
	appletHolderPopOutData(&aph, &ast);
	appletStorageRead(&ast, 0, &outdata, 24);
	appletHolderClose(&aph);
	appletStorageClose(&ast);
	appletStorageClose(&hast1);
	return outdata.UID;
}
