#include <stdio.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <switch.h>

#include "uct.h"

int main(int argc, char **argv) {
	bool didWrite = false;
	consoleInit(NULL);
	
	userID = getPreUsrAcc();
	
	printf("Loading Ultimate Controller Tools...\n");
	printf("Selected User: 0x%lx %lx\n", (u64)(userID>>64), (u64)userID);	
	getProfiles(profs);
	printf("Press - to test write");
	
	// Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		if ((kDown & KEY_MINUS) && !didWrite) {
			printf("\nWriting Y on GameCube to be attack on Profile 0\n");
			profs[0].setControlOpt(CProfile::GC, CProfile::GCY, CProfile::ATTACK);
			storeSaveFile(profs[0].raw, PROFILE_LEN, 0);
			didWrite = true;
		}
        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }
	
	consoleExit(NULL);
	return 0;
}

void getProfiles(CProfile *pfs) {
	FILE *save;
	char mem[PROFILE_LEN];
    if (R_FAILED(mntSaveDir())) {
        printf("Failed to mount save dir, stopping...\n");
        return;
    }
    save = fopen("save:/save_data/system_data.bin", "rb");
    if (save == nullptr) {
        printf("ERR! Failed to open system_data.bin for reading");
        return;
    }
    for (int i = 0; i < MAX_PROFILES; i++) {
        fseek(save, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * i), SEEK_SET);
        fread(mem, PROFILE_LEN, 1, save);
        if (mem[0] == 1) {
			pfs[i] = CProfile(mem);
            printf("Profile %u, Name: ", i);
            std::cout << pfs[i].getNameAsString() << std::endl;
            numPfs++;
        }        
    }
    printf("Found %u profiles in save.\n", numPfs);
    if (fclose(save) != 0) {
	    printf("fclose failed in getProfiles");
    }
	if (R_FAILED(fsdevUnmountDevice("save"))) {
		printf("fsdevUnmountDevice failed in getProfiles");
	}
}

//yoinked from https://github.com/WerWolv/EdiZon/blob/d5e4d35d89051c134003ec6d681c10ef2cc8e365/source/helpers/save.cpp#L414
s32 storeSaveFile(char *buffer, size_t length, int prof) {
	FsFileSystem fs;

	if (R_FAILED(mntSaveDir())) {
	printf("Failed to mount save.\n");
	fsdevUnmountDevice("save");
	fsFsClose(&fs);
	return -1;
	}

	char filePath[0x100];

	strcpy(filePath, "save:/");
	strcat(filePath, "save_data/system_data.bin");


	FILE *file = fopen(filePath, "rb+");

	if (file == nullptr) {
	printf("Failed to open file.\n");
	fsdevUnmountDevice("save");
	fsFsClose(&fs);
	return -2;
	}
	
	fseek(file, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * prof), SEEK_SET);
	fwrite(buffer, length, 1, file);
	printf("Wrote to offset %hX", ftell(file));
	fclose(file);

	if (R_FAILED(fsdevCommitDevice("save"))) {
	printf("Committing failed.\n");
	return -3;
	}

	fsdevUnmountDevice("save");
	fsFsClose(&fs);

	return 0;
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
