#pragma once
#include <dirent.h>
#include <switch.h>
#include <stdio.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

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
	0x0A, 0x0B, 0x0C, 00, 01, 05, 02, 02, 01, 01, 01, 01, 0x0D, 0x0D, 04, 03, 02, 00, 02, 01, 01, 01, 01, 01, 00, 00, 00};

std::streamoff off = PROFILE_OFF_START;
AccountUid accUid = {0};
size_t numPfs = 0;
u32 localIp = 0;
std::string localIpStr;
bool netActive = false;

enum PROFILE_CHECK_CODES {
	GOOD_PROFILE = 0b0000'0000,
	BAD_HEAD = 1,
	BAD_NAME = 2,
	BAD_GC = 4,
	BAD_PC = 8,
	BAD_JC = 16
};

struct CProfile {
	bool active = false;
	char raw[PROFILE_LEN], //raw copy of the profile
	header[4] = { 0 }, //Header, should be 1 0 0 0 or 0 0 0 0, sometimes different?
	name[20]= { 0 }, //profile name, bytes 13-32
	nameNoNull[10] = {0}, // A holder for the name sans the null bytes
	id[5]= { 0 }, //id, bytes 5 - 9, unknown generation method
	gc[15]= { 0 }, //Gamecube controls, bytes 37 - 51
	pc[16]= { 0 }, //Pro Controller controls, bytes 52 - 67
	jc[12]= { 0 }, //JoyCon controls, bytes 68 - 79
	edit = 0, //Edit count, shared across profiles, keeps track of the order in the edit menu
	end = 0; // Ending byte, probably to track which is "in use"?

	void printName() {
		// Check for inactive profile
		if (!active) {
			//return;
		}
		if (nameNoNull == 0) {
			printf("NoName");
		} else {
			int zeroCt = 0;
			for (char i : nameNoNull) {
				if (i != 0) {
					printf("%c", i);
					zeroCt = 0;
				}
				else if (zeroCt > 1){
					printf("Name is %u characters long.\n", i);
					break;
				}
				zeroCt++;
			}
		}
		printf("\n");
	}
	
	std::string getNameAsString() {
		// Check for inactive profile
		if (!active) {
			return "Inactive Profile";
		}
		std::string nm;
		if (name == 0) {
			memcpy(&name[0], &raw[12], 20);
		}
		int zeroCt = 0;
		for (char i : name) {
			if (i != 0) {
				//printf("%c\n", i);
				nm += i;
				zeroCt = 0;
			}
			else if (zeroCt > 1){
				break;
			}
			zeroCt++;
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
		memset(&header[0], 0, 4);
		memset(&id[0], 0, 5);
		memcpy(&gc[0], DEFAULT_GC_BUTTONS, 15);
		memcpy(&pc[0], DEFAULT_PC_BUTTONS, 16);
		memcpy(&jc[0], DEFAULT_JC_BUTTONS, 12);
		memset(&name[0], 0, 20);
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

		int nonNullPos = 0;
		for (int i = 0; i < 20; i++) {
			if (name[i] != 0) {
				name[nonNullPos] = name[i];
				nonNullPos++;
			}
		}
		printf("\n%c\n", nameNoNull[0]);
		if (header != 0) {
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
 * @details Mounts the save directory for read/write ops
 */
inline Result mntSaveDir() {
	DIR *dir;
	//int ret;
	struct dirent* ent;
	//FsFileSystem tmpfs;
	bool found = false;
	printf("Mounting save...\n");
	Result rc = fsdevMountSaveData("save", TITLE_ID, accUid);
	if (R_FAILED(rc)) {
            printf("fsdevMountSaveData() failed: 0x%x\n", rc);
        }
	/*if (R_SUCCEEDED(rc)) {
        ret = fsdevMountDevice("save", tmpfs);
        if (ret==-1) {
            printf("fsdevMountDevice() failed.\n");
            rc = ret;
        }
    }*/
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
 * @details Launches the profile selector to get user accounts
 * copied from EdiZon https://github.com/WerWolv/EdiZon/blob/2decd3214f2f2187f4f9330909bb0ca662eb1e20/source/guis/gui.cpp#L623
 */
inline u128 getPreUsrAcc() {
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
	appletCreateLibraryApplet(&aph, AppletId_LibraryAppletPlayerSelect, LibAppletMode_AllForeground);
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
	accUid.uid[1] = (u64)(outdata.UID>>64);
	accUid.uid[0] = (u64)(outdata.UID);
	return outdata.UID;
}

inline std::string formatIpFromInt(unsigned int ip) {
    unsigned char bytes[4];
	
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
	
	return std::to_string(bytes[0]) + "." + std::to_string(bytes[1]) + "." + std::to_string(bytes[2]) + "." + std::to_string(bytes[3]);
}

inline bool initNetwork() {
	Result rc = socketInitializeDefault();
	if (R_SUCCEEDED(rc)) {
		rc = nifmInitialize(NifmServiceType_User);
		if (R_SUCCEEDED(rc)) {
			rc = nifmGetCurrentIpAddress(&localIp);
			if (R_SUCCEEDED(rc) && localIp != 0) {
				localIpStr = formatIpFromInt(localIp);
				netActive = true;
				return true;
			}
			printf("Failed to get current IP");
			nifmExit();
			socketExit();
			return false;
		}
		printf("Failed to init NIFM services");
		socketExit();
		return false;
	}
	printf("Failed to init networking");
	return false;
}


inline void cleanNetwork() {
	nifmExit();
	socketExit();
}

/**
 * Sanity checks a controller profile
 * @param pf Raw char string of the profile to check
 * @return A bitmask that will be nonzero if there's a problem
 */
inline bool checkProfile(char* pf) {
	unsigned char mask{0b0000'0000};
	if (pf[0] != 1 && pf[0] != 0) {
		mask |= BAD_HEAD;
	}
	//TODO: Figure out name stuff
	/*for (int i = 12; i < 32; i++) {
		if (pf[i] < 32 && pf[i] > 3) {
			mask |= BAD_NAME;
			break;
		}
	}*/
	for(int i = CProfile::GCOFF; i < CProfile::PCOFF; i++) {
		if (pf[i] > 5) {
			if (!(i >=40 && i <= 42) && !(pf[i] >= 10 && pf[i] <= 12)) {
				mask |= BAD_GC;
			}
		}
	}
	for(int i = CProfile::PCOFF; i < CProfile::JCOFF; i++) {
		if (pf[i] > 5) {
			if (!(i >=55 && i <= 57) && !(pf[i] >= 10 && pf[i] <= 12)) {
				mask |= BAD_PC;
			}
		}
	}
	for(int i = CProfile::JCOFF; i < 79; i++) {
		if (pf[i] > 5) {
			if (!(i == 67 || i == 68) && pf[i] != 13) {
				mask |= BAD_JC;
			}
		}
	}
	if (mask == GOOD_PROFILE) {
		printf("Good profile\n");
		return true;
	}
	printf("Profile has the following errors:\n");
	if (mask & BAD_HEAD)
		printf("Header\n");
	if (mask & BAD_NAME)
		printf("Name\n");
	if (mask & BAD_GC)
		printf("Gamecube Controller Layout\n");
	if (mask & BAD_PC)
		printf("Pro Controller Layout\n");
	if (mask & BAD_JC)
		printf("Joycon Layout\n");
	printf("Here's a dump:\n");
	for (int i = 0; i < 82; i++) {
		printf("%hX ", pf[i]);
	}
	printf("\n\n");
	return false;
}

/**
 * @details Loads controller profiles from the console into memory
 * @param pfs CProfile array to write the profiles to
 */
inline void loadProfilesFromConsole(CProfile *pfs) {
	char mem[PROFILE_LEN];
    FILE* save = fopen("save:/save_data/system_data.bin", "rb");
    if (save == nullptr) {
        printf("ERR! Failed to open system_data.bin for reading\n");
        return;
    }
    for (int i = 0; i < MAX_PROFILES; i++) {
        fseek(save, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * i), SEEK_SET);
        fread(mem, PROFILE_LEN, 1, save);
        if (mem[0] == 1 && mem[1] == 0 && mem[2] == 0 && mem[3] == 0) {
			pfs[i] = CProfile(mem);
            printf("Profile %u, Name: %s\nRaw: ", i, pfs[i].nameNoNull);
        	for (char j : pfs[i].raw)
				printf("%hX ", j);
			printf("\n");
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
 * @details Loads a controller profile from the console into memory
 * @param pf CProfile to write the profile to
 * @param index index of the profile to get, must be < 60
 */
inline void loadProfileFromConsole(CProfile pf, int index) {
	if (index > MAX_PROFILES) {
		printf("ERROR @ dumpProfileToConsole: System cannot hold more then %x profiles, but index %x was requested", MAX_PROFILES, index);
		return;
	}
	char mem[PROFILE_LEN];
	
    FILE* save = fopen("save:/save_data/system_data.bin", "rb");
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
		printf("No profile found at index %i (offset 0x%08X)\n", index, PROFILE_OFF_START + (PROFILE_OFF_INTERVAL * index));
	}
}

/**
 * @details Loads a UCP controller profile file from the uct directory on the SD card to memory
 * @param file The UCP file to load
 */
inline CProfile loadProfileFromFile(std::string file) {
	char mem[PROFILE_LEN];
	FILE* f = fopen(file.c_str(), "r");
	if (!f) {
		printf("Couldn't open file in loadProfileFromFile");
		CProfile pf;
		return pf;
	}
	fread(mem, PROFILE_LEN, 1, f);
	fclose(f);

	//Sanity checking
	if(!checkProfile(mem)) {
		printf("Profile check failed\n");
		CProfile pf;
		return pf;
	}
		
	CProfile pf(mem);
	return pf;
}

/**
 *	@details Dumps all profiles from memory to the console
 *	copied from https://github.com/WerWolv/EdiZon/blob/d5e4d35d89051c134003ec6d681c10ef2cc8e365/source/helpers/save.cpp#L414
 *	@param buffer Raw profile bits to write
 *	@param index What index to write the buffer to
 *	@return true on success, false on fail with error printf'd
 */
inline bool dumpProfileToConsole(char* buffer, int index) {
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
	printf("Writing to offset 0x%08lX\n", ftell(file));
	fwrite(buffer, PROFILE_LEN, 1, file);
	printf("Wrote to offset, now at 0x%08lX\n", ftell(file));
	fclose(file);

	if (R_FAILED(fsdevCommitDevice("save"))) {
	printf("Committing failed.\n");
	return false;
	}

	fsdevUnmountDevice("save");
	fsFsClose(&fs);

	return true;
}

/**
 *	@details Dumps all profiles from memory to the console
 *	@param pfs Array of CProfiles to write
 *	@return true on success, false on fail with error printf'd
 */
inline bool dumpProfilesToConsole(CProfile* pfs) {
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
		printf("Writing to offset 0x%08lX\n", ftell(file));
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
}

/**
 *@details  Dumps a profile to a UCP file. Filename is generated based of profile name
 * and the timestamp of when the file was created
 * @param pf Profile to dump to file
 * @return true on success, false on fail with error printf'd
 */
inline bool dumpProfileToFile(CProfile pf, std::string file = "") {
	//Check to see if file exists. If yes, overwrite, if not, create new file with name
	//If no name specified, generate one
	int status;	
	DIR* dir;
	if (file == "") {
		u64 epoch;
		timeGetCurrentTime(TimeType_NetworkSystemClock, &epoch);
		file = "/uct/" +  pf.getNameAsString() + "-" + std::to_string(epoch) + + ".ucp";
	}

	printf("Dumping profile to file at %s\n", file.c_str());

	dir = opendir("/uct");
	if(dir==NULL) {
        status = mkdir("/uct", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		
		if (status < 0) {
			printf("Failed to create uct folder in SD root.\n");
			return false;
		}

		dir = opendir("/uct");
		if(dir==NULL) {
			printf("Failed to open uct folder in SD root.\n");
			return false;
		}
		
		printf("Successfully created uct folder in SD root.\n");
    }

	printf("Found uct folder at SD root.\n");

	FILE* f = fopen(file.c_str(), "wb");

	if (!f) {
		printf("Failed to open %s for writing in dumpProfileToFile %d\n", file.c_str(), errno);
		return false;
	}

	fwrite(pf.raw, sizeof(char), PROFILE_LEN, f);
	fclose(f);
	
	return true;
}

/**
 * @details Prints info about profiles loaded into memory
 * @param active weather or not to only show active profiles
 */
inline void showProfilesFromMemory(bool active) {
	if (active) {
		for(int i = 0; i < MAX_PROFILES; i++) {
			if (profs[i].active == true) {
				printf("Profile %u: %s\n", i, profs[i].getNameAsString().c_str());
			}
		}
	}
	else {
		for(int i = 0; i < MAX_PROFILES; i++) {
			printf("Profile %u: %s\n", i, profs[i].getNameAsString().c_str());
		}
	}
	
}

/**
 * @details Prints info about a profile from a specified file
 * @param file File to show info about
 */
inline void showProfileFromFile(std::string file) {
	//TODO: Test
	FILE* f = fopen(file.c_str(), "rb");
	char tmp[PROFILE_LEN];
	fread(tmp, sizeof(char), PROFILE_LEN, f);
	CProfile tmp1(tmp);
	printf("UCP file %s contained profile name %s", file.c_str(), tmp1.getNameAsString().c_str());
}

/**
 * Sets all profiles in memory to inactive. Need to call
 * dumpProfilesToConsole to actually write it
 */
inline bool deleteProfilesFromMemory() {
	int ct = 0;
	if (profs == nullptr) {
		printf("profs is null");
		return false;
	}
	for (auto& prof : profs) {
		if (prof.active) {
			prof.deactivate();
			ct++;
		}
	}
	printf("Deactivated %d profiles in memory", ct);
	return true;
}

/**
 * @details Gets the last active profile slot
 * @returns The the last active profile slow as an integer
 */
inline int getLastProfileIndex() {
	int tmp = 0;
	for (int i = 0; i < MAX_PROFILES; i++) {
		if(profs[i].active) {
			tmp = i;
		}
	}
	return tmp;
}

inline std::string selectUCP(PadState pad, std::string dirn = "/uct") {
	DIR* dir = opendir(dirn.c_str());
	std::vector<std::string> filesN;
	struct dirent* ent;
	int index = 0;
	char dirc[PATH_MAX] = "/uct/";
	if (!dir) {
		printf("%s could not be opened\n", dirn.c_str());
		return "";
	}
	while ((ent = readdir(dir))) {
        CProfile tmp;
        std::string s = ent->d_name;
        if (ent->d_name == nullptr) {
	        printf("Filename returned Nullptr\n");
        }
        else if (s.find(".ucp", s.size() - 4) != std::string::npos) {
	        strncpy(&dirc[strlen(dirn.c_str()) + 1], s.c_str(), s.length());
	        printf(dirc);
	        printf("\n");
        	filesN.push_back(dirc);
	        printf("\n");
        }
        memset(&dirc[strlen(dirn.c_str()) + 1], 0, sizeof(dirc) - (strlen(dirn.c_str()) + 1));
        memset(&dirc[strlen(dirn.c_str()) + 1], 0, sizeof(dirc));
    }
	if (filesN.empty()) {
		printf("No UCP files found in %s.\n", dirn.c_str());
		return "";
	}
	while (true) {
		consoleClear();
		printf("Select UCP file to view using Up/Down, confirm with A\n");
		int ct = 0;
		for (int i = 0; i < filesN.size(); i++) {
			if (i == index) {
				printf(">File %d: %s\n", i, filesN[i].c_str());
			}
			else {
				printf(" File %d: %s\n", i, filesN[i].c_str());
			}
			ct++;
		}
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);
		if ((kDown & HidNpadButton_AnyUp) && index > 0) index--;
		if ((kDown & HidNpadButton_AnyDown) && index < ct - 1) index++;
		if (kDown & HidNpadButton_A) return filesN[index];
		if (kDown & HidNpadButton_B) return "";
		consoleUpdate(NULL);
	}
}