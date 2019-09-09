#include <stdio.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <switch.h>

#define MAX_USERS 10
#define PROFILE_LEN 82 
#define PROFILE_OFF_START 80584
#define PROFILE_OFF_INTERVAL 63448‬

std::streamoff off = PROFILE_OFF_START;
u64 titleID=0x01006A800016E000; //titleID of Smash Ultimate
u128 userID=0; //Blank user to be filled
u128 uIds[MAX_USERS];
size_t numUsrs = 0;
FsFileSystem tmpfs;
DIR* dir;
std::fstream saveStr;
char mem[PROFILE_LEN];
char name[10];

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
			saveStr.seekg(off, std::ios::beg );
			saveStr.read(mem, PROFILE_LEN);
			for (int i = 0; i < 10; i++) {
				name[i] = mem[i+12];
			}
			printf("Found profile, name ");
			for (int i = 0; i < 10; i++) {
				if (name[i] != 0)
				printf("%c", name[i]);
			}
			printf(":\n");
			for (int i = 0; i < PROFILE_LEN; i++) {
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
	char raw[PROFILE_LEN], //raw copy of the profile
	name[20], //profile name, bytes 13-32
	id[4], //id, bytes 5 - 8, unknown generation method
	uk1 = 0x03, //unknown, byte 9, might be either 03, 04, or 0A
	gc[15], //Gamecube controls, bytes 37 - 51
	pc[16], //Pro Controller controls, bytes 52 - 67
	jc[12], //JoyCon controls, bytes 68 - 79
	edit, end = 0x0E; //Edit count and ending byte (0E)
	
	std::string getNameAsStr() {
		std::string nm;
		for (char i : name) {
			if (i != 0)
				nm += i;
		}
		return nm;
	}
	char getControlOpt(int con, int btn) {
		switch (con) {
		case GC:return gc[btn];
		case PC:return pc[btn];
		case JC:return jc[btn];
		default:return 0xff;	
		}
	}
	void setControlOpt(int con, int btn, int opt) {
		switch (con) {
		case GC:gc[btn] = static_cast<char>(opt);
			break;
		case PC:pc[btn] = static_cast<char>(opt);
			break;
		case JC:jc[btn] = static_cast<char>(opt);
			break;
		default:break;
		}
	}

	CProfile(char pf[PROFILE_LEN]) {
		for (int i = 0; i < PROFILE_LEN; i++) 
			raw[i] = pf[i];
		for (int i = 0; i < 4; i++) 
			id[i] = pf[i+4];
		for (int i = 0; i < 20; i++) 
			name[i] = pf[i+12];
		for (int i = 0; i < GCOFF; i++)
			gc[i] = pf[i+GCOFF-1];
		for (int i = 0; i < PCOFF; i++)
			pc[i] = pf[i+PCOFF-1];
		for (int i = 0; i < JCOFF; i++)
			jc[i] = pf[i+JCOFF-1];
		edit = pf[80];
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
	enum conType {
		GC = 0,
		PC = 1,
		JC = 2
	};
	enum gcBtn {
		GCOFF = 37, //Offset relative to start of profile, subtract 1 to use with arrays
		GCL = 0,
		GCR = 1,
		GCZ = 2,
		GCDPU = 3,
		GCDPS = 4,
		GCDPD = 5,
		GCA = 6,
		GCB = 7,
		GCC = 8,
		GCX = 9,
		GCY = 10,
		GCRMB = 11, //RUMble, SMasH A+B, TAP to jump, stick SENsativity
		GCSMH = 12,
		GCTAP = 13,
		GCSEN = 14
	};
	enum pcBtn {
		PCOFF = 52,
		PCL = 0,
		PCR = 1,
		PCZL = 2,
		PCZR = 3,
		PCDPU = 4,
		PCDPS = 5,
		PCDPD = 6,
		PCA = 7,
		PCB = 8,
		PCC = 9,
		PCX = 10,
		PCY = 11,
		PCRMB = 12,
		PCSMH = 13,
		PCTAP = 14,
		PCSEN = 15
	};
	enum jcBtn {
		JCOFF = 68,
		JCL = 0,
		JCZL = 1,
		JCSL = 2,
		JCSR = 3,
		JCDU = 4,
		JCDL = 5,
		JCDR = 6,
		JCDD = 7,
		JCRMB = 8,
		JCSMH = 9,
		JCTAP = 10,
		JCSEN = 11
	};
};
