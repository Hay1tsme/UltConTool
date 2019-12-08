#pragma once
#include <dirent.h>
#include <switch.h>
#include <stdio.h>
#include <cstring>
#include <fstream>

const int MAX_USERS = 10;
const int PROFILE_LEN = 82;
const int PROFILE_OFF_START = 80584;
const int PROFILE_OFF_INTERVAL = 63448;
const int MAX_PROFILES = 60;
const u64 TITLE_ID=0x01006A800016E000;
const char DEFAULT_GC_BUTTONS[15] = {03, 03, 04, 0x0A, 0x0B, 0x0C, 00, 01, 05, 02, 02, 01, 01, 01, 01};
const char DEFAULT_PC_BUTTONS[16] = {04, 04, 03, 03, 0x0A, 0x0B, 0x0C, 00, 01, 05, 02, 02, 01, 01, 01, 01};
const char DEFAULT_JC_BUTTONS[12] = {0x0D, 0x0D, 04, 03, 02, 00, 02, 01, 01, 01, 01, 01};
const char INACTIVE_PROFILE[82] = {00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
	00, 00,	00, 00, 00, 00, 00, 00, 00, 00, 00, 03, 03, 04, 0x0A, 0x0B, 0x0C, 00, 01, 05, 02, 02, 01, 01, 01, 01, 04, 04, 03, 03,
	0x0A, 0x0B, 0x0C, 00,01, 05, 02, 02, 01, 01, 01, 01, 0x0D, 0x0D, 04, 03, 02, 00, 02, 01, 01, 01, 01, 01, 00, 00, 00};

struct CProfile;
struct dirent* ent;

std::streamoff off = PROFILE_OFF_START;
u128 userID=0; //Blank user to be filled
size_t numPfs = 0;

inline Result mntSaveDir();

struct CProfile {
	bool active = false;
	char raw[PROFILE_LEN], //raw copy of the profile
	header[4] = { 0 }, //Header, should be 1 0 0 0 or 0 0 0 0, sometimes different?
	name[20]= { 0 }, //profile name, bytes 13-32
	id[5]= { 0 }, //id, bytes 5 - 9, unknown generation method
	gc[15]= { 0 }, //Gamecube controls, bytes 37 - 51
	pc[16]= { 0 }, //Pro Controller controls, bytes 52 - 67
	jc[12]= { 0 }, //JoyCon controls, bytes 68 - 79
	edit = 0, //Edit count, shared across profiles, keeps track of the order in the edit menu
	end = 0; // Ending byte, probably to track which is "in use"?
	
	std::string getNameAsString() {
		// Check for inactive profile
		if (!active) {
			return "Inactive Profile";
		}
		std::string nm;
		if (name == 0) {
			memcpy(&name[0], &raw[12], 20);
		}
		int zc = 0;
		for (char i : name) {
			if (i != 0) {
				nm += i;
				zc = 0;
			}
			else if (zc > 1){
				break;
			}
			zc++;
		}
		return nm;
	}
	char getControlOpt(int con, int btn) {
		switch (con) {
		case GC:return gc[btn];
		case PC:return pc[btn];
		case JC:return jc[btn];
		default:return '-';
		}
	}
	void setControlOpt(int con, int btn, int opt) {
		switch (con) {
		case GC:gc[btn] = static_cast<char>(opt);
				raw[btn + GCOFF] = gc[btn];
			break;
		case PC:pc[btn] = static_cast<char>(opt);
			raw[btn + PCOFF] = gc[btn];
			break;
		case JC:jc[btn] = static_cast<char>(opt);
			raw[btn + JCOFF] = gc[btn];
			break;
		default:break;
		}
	}
	void resetButtons() {
		memcpy(&gc[0], DEFAULT_GC_BUTTONS, 15);
		memcpy(&pc[0], DEFAULT_PC_BUTTONS, 16);
		memcpy(&jc[0], DEFAULT_JC_BUTTONS, 12);
	}
	void deactivate() {
		memcpy(&raw[0], INACTIVE_PROFILE, PROFILE_LEN);
		memcpy(&header[0], 0, 4);
		memcpy(&id[0], 0, 5);
		memcpy(&gc[0], DEFAULT_GC_BUTTONS, 15);
		memcpy(&pc[0], DEFAULT_PC_BUTTONS, 16);
		memcpy(&jc[0], DEFAULT_JC_BUTTONS, 12);
		memcpy(&name[0], 0, 20);
		edit = 0;
		end = 0;
		active = false;
	}

	CProfile() {}
	CProfile(char *pf) {
		memcpy(&raw[0], &pf[0], PROFILE_LEN);
		memcpy(&header[0], &pf[0], 4);
		memcpy(&id[0], &pf[4], 5);
		memcpy(&name[0], &pf[12], 20);
		memcpy(&gc[0], &pf[GCOFF], 15);
		memcpy(&pc[0], &pf[PCOFF], 16);
		memcpy(&jc[0], &pf[JCOFF], 12);
		edit = pf[80];
		end = pf[81];

		if (end != 0 && header != 0) {
			active = true;
		}
	}

	enum controls {
		ATTACK = 0x00,
		TILT = 0x00,
		OFF = 0x00,
		LOW = 0x00,
		SPECIAL = 0x01,
		ON = 0x01,
		MED = 0x01,
		JUMP = 0x02,
		HIGH = 0x02,
		SHIELD = 0x03,
		GRAB = 0x04,
		SMASH = 0x05,
		UTAUNT = 0x0A,
		STAUNT = 0x0B,
		DTAUNT = 0x0C,
		UNUSED = 0x0D
	};
	enum conType {
		GC = 0,
		PC = 1,
		JC = 2
	};
	enum gcBtn {
		
		GCL = 0,
		GCR = 1,
		GCZ = 2,
		GCDPU = 3,
		GCDPS = 4,
		GCDPD = 5,
		GCA = 6,
		GCB = 7,
		GCC = 8,
		GCY = 9,
		GCX = 10,
		GCRMB = 11, //RUMble, SMasH A+B, TAP to jump, stick SENsativity
		GCSMH = 12,
		GCTAP = 13,
		GCSEN = 14,
		GCOFF = 36 //Offset relative to start of profile
	};
	enum pcBtn {
		
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
		PCSEN = 15,
		PCOFF = 51
	};
	enum jcBtn {
		
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
		JCSEN = 11,
		JCOFF = 67
	};
};
CProfile profs[60];

/**
 * Mounts the save directory for read/write ops
 */
inline Result mntSaveDir() {
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
			while ((ent = readdir(dir)) && !found) {
			if(strcmp(ent->d_name, "system_data.bin") == 0) {
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

/**
 * Launches the profile selector to get user accounts
 */
inline u128 getPreUsrAcc() {
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

/**
 * Loads controller profiles from the console into memory
 * @param pfs CProfile array to write the profiles to
 */
inline void loadProfilesFromConsole(CProfile *pfs) {
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
            //printf("Profile %u, Name: %s\n", i, pfs[i].getNameAsString());
            numPfs++;
        }        
    }
    printf("Found %lu profiles in save.\n", numPfs);
    if (fclose(save) != 0) {
	    printf("fclose failed in loadProfilesFromConsole\n");
    }
	if (R_FAILED(fsdevUnmountDevice("save"))) {
		printf("fsdevUnmountDevice failed in loadProfilesFromConsole\n");
	}
}

/**
 * Loads a controller profile from the console into memory
 * @param pf CProfile to write the profile to
 * @param index index of the profile to get, must be < 60
 */
inline void loadProfileFromConsole(CProfile pf, int index) {
	if (index > MAX_PROFILES) {
		printf("ERROR @ dumpProfileToConsole: System cannot hold more then %x profiles, but index %x was requested", MAX_PROFILES, index);
		return;
	}
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

/**
 * Loads a UCP controller profile file from the uct directory on the SD card to memory
 * @param pf CProfile to write the profile to
 * @param file The UCP file to load
 */
inline void loadProfileFromFile(CProfile pf, std::string file) {
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

/**Dumps all profiles from memory to the console
 *	@param buffer Raw profile bits to write
 *	@param index What index to write the buffer to
 *	@return true on success, false on fail with error printf'd
 */
inline bool dumpProfileToConsole(char* buffer, int index) {
	//yoinked from https://github.com/WerWolv/EdiZon/blob/d5e4d35d89051c134003ec6d681c10ef2cc8e365/source/helpers/save.cpp#L414

	//Prevent writing to profiles beyond the max of 60
	if (index > MAX_PROFILES) {
		printf("ERROR @ dumpProfileToConsole: System cannot hold more then %x profiles, but index %x was requested", MAX_PROFILES, index);
		return false;
	}
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

/**Dumps all profiles from memory to the console
 *	@param pfs Array of CProfiles to write
 *	@return true on success, false on fail with error printf'd
 */
inline bool dumpProfilesToConsole(CProfile* pfs) {
	//TODO: Test
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
	printf("Writing %lu profiles to the save file", sizeof(pfs));
	for (int i = 0; i < sizeof(pfs); i++) {
		fseek(file, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * i), SEEK_SET);
		printf("Writing to offset %hX\n", ftell(file));
		fwrite(pfs[i].raw, PROFILE_LEN, 1, file);
	}	
	fclose(file);

	if (R_FAILED(fsdevCommitDevice("save"))) {
	printf("Committing failed.\n");
	return false;
	}

	fsdevUnmountDevice("save");
	fsFsClose(&fs);

	return true;
	return 0;
}

/**
 * Dumps a profile to a UCP file. Filename is generated based of profile name
 * and the timestamp of hwen the file was created
 * @param pf Profile to dump to file
 * @return true on success, false on fail with error printf'd
 */
inline bool dumpProfileToFile(CProfile pf) {
	//TODO: Fill out
	return true;
}

/**
 * Prints info about all profiles stored on the save file
 */
inline void showProfilesFromConsole() {
	//TODO: Fill out
}

/**
 * Prints info about a profile stored on the save file
 * @param index index of profile to show
 */
inline void showProfileFromConsole(int index) {
	//TODO: Fill out
}

/**
 * Prints info about all profiles loaded into memory
 */
inline void showProfilesFromMemory() {
	//TODO: Fill Out
	for(int i = 0; i < MAX_PROFILES; i++) {
		if (profs[i].active == true) {
			printf("Found loaded profile in slot %u named %s\n", i, profs[i].getNameAsString().c_str());
		}
	}
}

/**
 * Prints info about a profile loaded into memory
 * @param index index of profile to show
 */
inline void showProfileFromMemory(int index) {
	//TODO: Fill out
}

/**
 * Prints info about a profile from a specified file
 * @param file File to show info about
 */
inline void showProfileFromFile(std::string file) {
	//TODO: Fill out
}

/**
 * Sanity checks a profile on the save file
 * @return true if file is good, false otherwise
 */
inline bool checkProfileFromConsole() {
	//TODO: Fill out
	return true;
}

/**
 * Sanity checks a profile loaded into memory
 * @return true if file is good, false otherwise
 */
inline bool checkProfileFromMemory() {
	//TODO: Fill out
	return true;
}

/**
 * Sanity checks a UCP file
 * @param file File to check
 * @return true if file is good, false otherwise
 */
inline bool checkProfileFromFile(std::string file) {
	//TODO: Fill out
	return true;
}

//TODO: Better naming convention for these, clear is unclear
/**
 * Sets all profiles in memory to inactive. Need to call
 * dumpProfilesToConsole to actually write it
 */
inline bool clearProfilesFromMemory() {
	//TODO: Fill in
	return true;
}

/**
 * Sets a profile in memory to inactive. Need to call
 * dumpProfileToConsole to actually write it
 * @param index index of profile to clear
 */
inline bool clearProfileFromMemory(int index) {
	//TODO: Fill in
	return true;
}
