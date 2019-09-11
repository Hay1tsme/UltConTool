#include <stdio.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <switch.h>

#include "uct.h"

int main(int argc, char **argv) {
	
	consoleInit(NULL);
	
	userID = getPreUsrAcc();
	CProfile profs[60];
	
	printf("Loading Ultimate Controller Tools...\n");
	printf("Selected User: 0x%lx %lx\n", (u64)(userID>>64), (u64)userID);
	printf("Mounting save...\n");
	if (R_FAILED(mntSave())) {
		printf("Failed to mount save file, stopping...\n");
	}
	getProfiles(profs);
	printf("\nLoaded profiles\n");
	
	 // Main loop
    while(appletMainLoop())
    {
        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

        consoleUpdate(NULL);
    }
	closedir(dir);
	saveStr.close();
	accountExit ();
	consoleExit(NULL);
	return 0;
}

void getProfiles(CProfile pfs[60]) {
	int tmp = 0;
	printf("Loading profiles\n");
	saveStr.seekg(0, std::ios::beg);
	for (int i = 0; i < MAX_PROFILES; i++) {
		tmp = PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * i);
		saveStr.seekg(tmp, std::ios::beg);
		saveStr.read(mem, PROFILE_LEN);
		if (mem[0] == 1) {
			pfs[i] = CProfile(mem);
			printf("Profile %u, Name: ", i);
			std::cout << pfs[i].getNameAsString() << std::endl;
		}
	}
}

Result mntSave() {
	int ret;
	bool found = false;
	Result rc = fsMount_SaveData(&tmpfs, titleID, userID);
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
	}
	if R_SUCCEEDED(rc) {
		saveStr.open("save:/save_data/system_data.bin", std::ios::in | std::ios::out | std::ios::binary);
		if (!saveStr.is_open()) {
			rc = 1;
			printf("Failed to open save file\n");
		}
		else {
			printf("Save file opened successfully\n");
						
		}
		
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
