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
	getProfiles(profs);
	printf("\nWriting Y on GameCube to be attack on Profile 0\n");
	profs[0].setControlOpt(CProfile::GC, CProfile::GCY, CProfile::ATTACK);
	toWrite[0] = true;
	writeToSave(profs);

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
	
	consoleExit(NULL);
	return 0;
}

void writeToSave(CProfile *pfs) {
	if (R_FAILED(mntSaveDir())) {
		printf("Failed to mount save dir, stopping...\n");
		return;
	}
	save = fopen("save:/save_data/system_data.bin", "wb");
	if (save == nullptr) {
		printf("ERR! Failed to open system_data.bin for writing");
		return;
	}
	printf("Writing profiles...\n");
	for (int i = 0; i < 60; i++) {
		if (toWrite[i]) {
			printf("Writing to profile %u\n", i);
			fseek(save, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * i), SEEK_SET);
			fwrite(pfs[i].raw, PROFILE_LEN, 1, save);
		}
	}	
	if (R_FAILED(fsdevCommitDevice("save"))) {
		printf("could not commit\n");
		fclose(save);
		fsdevUnmountDevice("save");
		return;
	}
	printf("Committed sucessfully.\n");
	fclose(save);
	fsdevUnmountDevice("save");
}

void getProfiles(CProfile *pfs) {
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
			pfs[i] = mem;
			printf("Profile %u, Name: ", i);
			std::cout << pfs[i].getNameAsString() << std::endl;
			numPfs++;
		}		
	}
	printf("Found %u profiles in save.\n", numPfs);
	fclose(save);
	fsdevUnmountDevice("save");
}

Result mntSaveDir() {
	int ret;
	bool found = false;
	printf("Mounting save...\n");
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
