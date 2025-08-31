#pragma once

#include <cstdint>
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

const int chn[22] =
{
	0,1,2,0,1,2,-1,-1,3,4,5,3,4,5,-1,-1,6,7,8,6,7,8
};

const int opn[22] =
{
	0,0,0,1,1,1,-1,-1,0,0,0,1,1,1,-1,-1,0,0,0,1,1,1
};

/*
25.3.2008: IMPORTANT:
make opl2plr capable of outputting at half and quarter sample rates!
to enable playback on slower computers. or something.
a major problem will be feedback. as it is dependent on previous samples.
25.3.2008: DONE.
lol. actually it took like an hour to implement half- and quarter-rate emulation.
*/
//what should the sample rate be divided by?
//use 1 for full sample rate, 2 for a half, 4 for a quarter.
//catch my drift?
const int SAMPLERATE_DIV = 1;

// 28.2.2008: Every table has been changed to an integer table.
// 17.3.2008: Almost.
// 23.3.2008: I mean: Every table that is used real-time
//			  has been converted to an integer table

const unsigned int pow2[] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576};

extern unsigned int pow4rt2[256]; //filled in init();

// Found the frequency 49716 in Bisqwit's sources, Wikipedia says it's "about 49720"
// Furthermore, by calculating it from the information on the datasheet,
// I get 49722.2222222222222222222222222222222222222222222222222222222222...
// Go figure.

// okay. the clock of some chip near an OPL3 is 14.318 MHz. divided by 288, it's ~49715.
// I still don't know where the 288 (=72*4) comes from though.

// okay. the precise clock is 14318160 Hz, making the OPL2 clock ~49715.

const int OPL2_HZ = 14318160/288;

// This rocks. This really does.
// It's the frequency you get at octave=0 and freqnum=1 :)
// the rest can be derived from this.
//const double BASE_FREQUENCY = OPL2_HZ/pow(2.0,20.0);

/*
28.2.2008: I figured out a better way of generating the
needed frequencies, using only integer math!
*/
//const int SINE_SIZE = 1048576;

//16.9.2008: when i stopped using four two-megabyte tables for the sine waves,
//emulation speed increased by about 100%. lessons learned.
const int PERIOD_SIZE = 1<<20;
const int SINE_BLOCK = 11;
const int SINE_INTERVAL = 1<<SINE_BLOCK;
const int SINE_SIZE = PERIOD_SIZE/SINE_INTERVAL/SAMPLERATE_DIV;

const int FM_AMOUNT = PERIOD_SIZE/8192;

// yes. very long.
const double PI = 3.14159265358979323846264338327950288419716939937510;
//const double PI = 1.0;

// the Opl2 chip has 9 channels
const int OPL2_CHANNELS = 9;

const int FB[8] =
{
	0, 1, 2, 4, 8, 16, 32, 64
};

const unsigned int pow2_A[] = {0,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576};

//resolution of the adsr envelope, in conjunction with a lot of other things
const int ADSR_MAX = 8192;

//all tables are filled in init();
const int TOTLVL_SIZE = 64;
const int ADSR_S_SIZE = 16;

//these values are in samples. OPL2_HZ cycles is one second.
//163840 and 174080 are experimental values. if you know the true values, please tell me.
const int ADSR_A_SIZE = 163840/SAMPLERATE_DIV;
const int ADSR_DR_SIZE = 174080/SAMPLERATE_DIV;
const int AMPMOD_SIZE = 13*1024/SAMPLERATE_DIV; // length of one cycle in samples.
const int VIBRATO_SIZE = 8*1024/SAMPLERATE_DIV; // same here


const int LKS_SIZE = 131072;
const int LKS_N = 4;

//at which frequency value should we start turning the volume down with LKS?
//should be 1536 but it doesn't work equally well.
//1536 was attained by testing the OPL3
const int LKS_START = 1024;//3072;

extern int TOTLVL[TOTLVL_SIZE];
extern int ADSR_A[ADSR_A_SIZE];
extern int ADSR_S[ADSR_S_SIZE];
extern int ADSR_DR[ADSR_DR_SIZE];


//this table is only used in generating the LKS tables, it's not used real-time.
const double LKS_SPEED[4] = {0.0, 0.5, 0.25, 1.0};

//lks tables go here
extern int LKS[LKS_N][LKS_SIZE];

//registers that an OPL2 uses are marked with a 1
//used in write() to determine writes to unused registers (that should be ignored).
const bool USE_REG[256] =
{
	1,1,1,1,1,0,0,0,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,
	1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,
	1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,
	1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,
	1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0,
	1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,
	1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
};

extern int SONGSPEED_IMF; // ticks per second, ie. Hz
extern int SONGSPEED_DRO; // DRO files are 1000Hz

const int HARM[16] = {0,1,2,3,4,5,6,7,8,9,10,10,12,12,15,15};

const double AMPMOD_DEPTH[2] = {1.0, 4.8};

extern int AMPMOD[2][AMPMOD_SIZE];
//extern int VIBRATO[2][VIBRATO_SIZE];

//convert adsr_volume to dr_state&dr_div:
extern int VOL2STATE[ADSR_MAX];
extern int VOL2DIV[ADSR_MAX];

//FIXME: the members of this enum and
//some of the members in OP have the same name.
enum ADSR_STATE
{
	N,A,D,S,R
};

class Opl2;

struct OP
{
	void reset_ADSR();
	bool ampmod{};
	bool vibrato{};
	bool hold_instr{};//should we use the sustain phase?
	bool ksr{}; //keyboard scaling rate
	u8 harmonic{};
	u8 volume{};
	u8 lks{}; // level key scaling rate :DDDdd
	u8 A{},D{},S{},R{}; // envelope
	ADSR_STATE adsr{N}; //envelope state
	u16 ADSR_volume{};
	u32 A_state{};
	u32 DR_div{1};//when DR_state wraps around, this will be doubled and DR_state will start at 0
	u32 DR_state{};//decay and release phase volume state.
	u32 sinestate{}; // sine wave state, orly owl
	u8 wavetype{};
	u32 wanha1{}, wanha2{}; // old samples. used for feedback, thus only for op0
	u16 rof{};
	u32 freq_harmonic{}; //frequency adjusted with harmonic
	u32 freq{};
	int vibrato_amount{};
	u8 oct{};

	int update(int fm, Opl2& opl2);
	void update_phase();
	void keyoff();
	bool note{};
};

struct CHANNEL
{
	OP ops[2];
	u16 hifnum{}, lofnum{};
	u8 oct{};
	u8 feedback{};
	bool algo{}; //0 for FM, 1 for AM
	void keyon(Opl2& opl2);
	void keyoff(Opl2& opl2);

	u32 freq{};
	void updfreq(Opl2& opl);
	bool note{};
	bool rhythm{}; //rhythm mode on for this channel? if opl2.rhythm is true, channels 6-8 will have rhythm on, others will have it off
	int fnum3{};
};


class Opl2
{
public:
    static void Init();
    static void Quit();

	void write(unsigned char r, unsigned char d);

	CHANNEL chans[OPL2_CHANNELS];
	bool mode{}; // if true, act like OPL2, if false, act like OPL1;
	bool keysplit{};
	short update();
	void update_ADSR();

	bool ampmod_depth{};
	bool vibrato_depth{};
	bool rhythm{}; //is rhythm mode on or off? not implemented at all yet
	int ampmod_state{};
	int vibrato_state{}; //not implemented fully yet

	int writereg{};
	bool run_timer1{};
	bool run_timer2{};
	u16 timer1_state{};
	u16 timer2_state{};

	u8 status{}; //OPL2 Status byte!
	int rate;
	bool bass_on{}, snare_on{}, cymbal_on{}, hihat_on{};

	void writebyte(int port, int val);

	void reset()
	{
	    for(int i=0; i<256; ++i)
            writebyte(i,0);
	}
};

//vector<short> opl2_play_imf(const string& filename);
