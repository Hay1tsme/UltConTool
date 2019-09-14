#pragma once
#include <dirent.h>

const int MAX_USERS = 10;
const int PROFILE_LEN = 82;
const int PROFILE_OFF_START = 80584;
const int PROFILE_OFF_INTERVAL = 63448;
const int MAX_PROFILES = 60;

struct CProfile;
std::streamoff off = PROFILE_OFF_START;
u64 titleID=0x01006A800016E000; //titleID of Smash Ultimate
u128 userID=0; //Blank user to be filled
u128 uIds[MAX_USERS];
size_t numPfs = 0;
FsFileSystem tmpfs;
DIR *dir;
FILE *save;
char mem[PROFILE_LEN];
char name[10];
bool toWrite[60] = {false};

void writeToSave(CProfile *pfs);
u128 getPreUsrAcc();
Result mntSaveDir();
void getProfiles(CProfile *pfs);

struct dirent* ent;
struct CProfile {
	char raw[PROFILE_LEN], //raw copy of the profile
	name[20], //profile name, bytes 13-32
	id[4], //id, bytes 5 - 8, unknown generation method
	uk1 = 0x03, //unknown, byte 9, might be either < 10
	gc[15], //Gamecube controls, bytes 37 - 51
	pc[16], //Pro Controller controls, bytes 52 - 67
	jc[12], //JoyCon controls, bytes 68 - 79
	edit, end = 0x0E; //Edit count and ending byte (0E)
	
	std::string getNameAsString() {
		std::string nm;
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
		default:return 0xff;
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

	CProfile() {}
	CProfile(char pf[PROFILE_LEN]) {
		for (int i = 0; i < PROFILE_LEN; i++) 
			raw[i] = pf[i];
		for (int i = 0; i < 4; i++) 
			id[i] = pf[i+4];
		for (int i = 0; i < 20; i++) 
			name[i] = pf[i+12];
		for (int i = 0; i < 15; i++)
			gc[i] = pf[i+GCOFF];
		for (int i = 0; i < 16; i++)
			pc[i] = pf[i+PCOFF];
		for (int i = 0; i < 12; i++)
			jc[i] = pf[i+JCOFF];
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
		GCOFF = 36, //Offset relative to start of profile
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
		GCSEN = 14
	};
	enum pcBtn {
		PCOFF = 51,
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
		JCOFF = 67,
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
