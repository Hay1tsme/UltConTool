#include <stdio.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <switch.h>

#define MAX_USERS 10

u64 titleID=0x01006A800016E000; //titleID of Smash Ultimate
u128 userID=0; //Blank user to be filled
u128 uIds[MAX_USERS];
size_t numUsrs = 0;
FsFileSystem tmpfs;
DIR* dir;
std::fstream saveStr;

struct dirent* ent;
struct CProfile;

Result mntSave();
u128 getPreUsrAcc();

int main(int argc, char **argv) {
	
	consoleInit(NULL);
	
	userID = getPreUsrAcc();
	
	printf("Loading Ultimate Controller Tools...\n");
	printf("Selected User: 0x%lx %lx\n", (u64)(userID>>64), (u64)userID);
	printf("Mounting save...\n");
	if (R_FAILED(mntSave())) {
		printf("Failed to mount save file, stopping...\n");
	}
	
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

Result mntSave() {
	int ret=0;
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
        dir = opendir("save:/save_data");//Open the "save:/" directory.
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
			std::streamoff off = 80584;
			char mem[82];
			char name[10];
			
			printf("Save file opened successfully\n");
			saveStr.seekg(off, std::ios::beg );
			saveStr.read(mem, 82);
			for (int i = 0; i < 10; i++) {
				name[i] = mem[i+12];
			}
			printf("Found profile, name ");
			for (size_t i = 0; i < 10; i++) {
				if (name[i] != 0)
				printf("%c", name[i]);
			}
			printf(":\n");
			for (size_t i = 0; i < 82; i++) {
				printf("%02x ", mem[i]);
			}
			printf("\n");
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

struct CProfile {
	char name[20]; //name, bytes 13-32
	char id[4]; //id, bytes 5 - 8, unknown generation method
	unsigned char uk1 = 0x03; //unknown, byte 9, might be either 03, 04, or 0A
	//Gamecube controls, bytes 37 - 51, RUMble, SMasH A+B, TAP to jump, stick SENsativity
	unsigned char gcA, gcB, gcX, gcY, gcZ, gcR, gcL, gcDPU, gcDPD, gcDPS, gcC, gcRUM, gcSMH, gcTAP, gcSEN;
	//Pro Controller controls, bytes 52 - 67
	unsigned char pcA, pcB, pcX, pcY, pcRS, pcL, pcR, pcZL, pcZR, pcDPU, pcDPS, pcDPD, pcRUM, pcSMH, pcTAP, pcSEN;
	//YouCon controls, bytes 68 - 79
	unsigned char jcDU, jcDL, jcDR, jcDD, jcSR, jcSL, jcL, jcZL, jcRUM, jcSMH, jcTAP, jcSEN;
	//Edit count and ending byte (0E)
	unsigned char edit, end = 0x0E;
	char raw[82];
	
	std::string getNameAsStr() {
		std::string nm;
		for (size_t i = 0; i < 20; i++) {
			if (name[i] != 0)
			//printf("%c", name[i]);
				nm += name[i];
		}
		return nm;
	}
	CProfile(char pf[82]) {
		for (int i = 0; i < 82; i++) 
			raw[i] = pf[i];
		for (int i = 0; i < 4; i++) 
			id[i] = pf[i+4];
		for (int i = 0; i < 20; i++) 
			name[i] = pf[i+12];

		
	}

	enum controls {
		ATTACK = 0x00,
		TILT = 0x00, //RStick only
		SPECIAL = 0x01,
		JUMP = 0x02,
		SHIELD = 0x03,
		GRAB = 0x04,
		SMASH = 0x05,
		UTAUNT = 0x0A,
		STAUNT = 0x0B,
		DTAUNT = 0x0C,
		UNUSED = 0x0D,//Joycon L/R and ZL/ZR only
		OFF = 0x00, //Other Options only
		ON = 0x01,	//-
		LOW = 0x00,	//Stick Sensitivity only
		MED = 0x01,	//-
		HIGH = 0x02	//-
	};
};
