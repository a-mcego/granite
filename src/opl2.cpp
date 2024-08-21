#include "opl2.h"

#include <cmath>
#include <cstring>

int TOTLVL[TOTLVL_SIZE];
int ADSR_A[ADSR_A_SIZE];
int ADSR_S[ADSR_S_SIZE];
int ADSR_DR[ADSR_DR_SIZE];

int sine[4][SINE_SIZE];

int LKS[LKS_N][LKS_SIZE];

unsigned int pow4rt2[256];

int AMPMOD[2][AMPMOD_SIZE];
int VIBRATO[2][VIBRATO_SIZE];

int VOL2STATE[ADSR_MAX] = {};
int VOL2DIV[ADSR_MAX] = {};

void Opl2::Init()
{

	for(int i=0; i<SINE_SIZE; i++)
	{
        sine[0][i] = int(sin(i*2*PI/double(SINE_SIZE))*32767.0);
        if (i<SINE_SIZE/2)
            sine[1][i] = sine[0][i];
        else
            sine[1][i] = 0;

        sine[2][i] = abs(sine[0][i]);

        if (!((4*i/SINE_SIZE)%2))
            sine[3][i] = sine[2][i];
        else
            sine[3][i] = 0;
	}

	//cout << "adsr..";
	//adsr tables
	for(int i=0; i<ADSR_A_SIZE; i++)
	{
		ADSR_A[i] = int(double(i)*double(ADSR_MAX)/double(ADSR_A_SIZE));
	}

	for(int i=0; i<ADSR_DR_SIZE; i++)
	{
		ADSR_DR[i] = int(pow(2.0, -double(i)/double(ADSR_DR_SIZE))*double(ADSR_MAX));
	}

	for(int i=0; i<ADSR_S_SIZE; i++)
	{
		ADSR_S[i] = int(pow(2.0, -double(i)/2.0)*double(ADSR_MAX));
	}

	//cout << "lks..";
	//level key scaling tables
	for(int i=0; i<LKS_SIZE; i++)
		LKS[0][i] = ADSR_MAX;

	memcpy(LKS[1], LKS[0], sizeof(int)*LKS_SIZE);
	memcpy(LKS[2], LKS[0], sizeof(int)*LKS_SIZE);
	memcpy(LKS[3], LKS[0], sizeof(int)*LKS_SIZE);

	for(int i=LKS_START; i<LKS_SIZE; i++)
	{
		LKS[0][i] = ADSR_MAX;
		LKS[1][i] = int(pow(double(LKS_START)/double(i),LKS_SPEED[1])*double(ADSR_MAX));
		LKS[2][i] = int(pow(double(LKS_START)/double(i),LKS_SPEED[2])*double(ADSR_MAX));
		LKS[3][i] = int(pow(double(LKS_START)/double(i),LKS_SPEED[3])*double(ADSR_MAX));
	}

	//cout << "4rt2..";
	//(4th root of 2) to the power of i -table
	for(int i=0; i<256; i++)
	{
		pow4rt2[i] = (unsigned int)(pow(pow(2.0, 0.25), double(i)));
	}

	//cout << "totlvl..";
	//total level table
	for(int i=0; i<TOTLVL_SIZE; i++)
	{
		TOTLVL[i] = int(pow(2.0, -double(i)/8.0)*double(ADSR_MAX));
	}

	//cout << "ampmod..";
	//amplitude modulation table
	for(int j=0; j<2; j++)
	{
		for(int i=0; i<=AMPMOD_SIZE/2; i++)
		{
			AMPMOD[j][i] = int(pow(pow(2.0, AMPMOD_DEPTH[j]/6.0), -2.0*double(i)/double(AMPMOD_SIZE))*double(ADSR_MAX));
			AMPMOD[j][AMPMOD_SIZE-i] = AMPMOD[j][i];
		}
	}

	//cout << "vol2..";
	//adsr volume to dr_state and dr_div, lookup table
	int state=0,div=0, i=8191;
	while(i>=0)
	{
		if (state >= ADSR_DR_SIZE)
		{
			state-=ADSR_DR_SIZE;
			++div;
		}
		if ((ADSR_DR[state]>>div) == i+1)
		{
			VOL2DIV[i] = div;
			VOL2STATE[i] = state;
			i--;
		}
		state++;
	}
	//cout << "done." << endl;
}


//write shit to opl2
void Opl2::write(unsigned char r, unsigned char d)
{
	if (!USE_REG[r])
		return;

    Opl2& opl2 = *this;

	switch(r&0xE0)
	{
	case 0x00:
		if (r==0)
			opl2.mode = bit(d&0x20);
		if (r==0x08)
		{
			opl2.keysplit = bit(d&0x40);
			for(int c=0; c<OPL2_CHANNELS; c++)
				opl2.chans[c].updfreq(opl2);
		}
		break;
	case 0x20:
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].ampmod = d&0x80;
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].vibrato = d&0x40;
		//if (d&0x40)
		//	cout << "Vibrato not implemented yet" << endl;
		//TODO: copy vibrato over from the other project
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].hold_instr = d&0x20;
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].ksr = d&0x10;

		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].harmonic = uchar(HARM[d&0x0F]);
		opl2.chans[chn[r&0x1F]].updfreq(opl2);
		break;
	case 0x40:
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].volume = uchar(d&0x3F);
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].lks = uchar(d>>6);
		break;
	case 0x60:
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].A = uchar(d>>4);
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].D = uchar(d&0x0F);
		break;
	case 0x80:
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].S = uchar(d>>4);
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].R = uchar(d&0x0F);
		break;
	case 0xA0:
		if (r==0xBD)
		{
			opl2.ampmod_depth = (d>>7)&0x01;
			opl2.vibrato_depth = (d>>6)&0x01;
			opl2.rhythm = d&0x20;
			if (opl2.rhythm)
			{
				//this should be OK
				if (d&0x10)
				{
					//opl2.chans[6].ops[0].reset_ADSR();
					//opl2.chans[6].ops[1].reset_ADSR();
					opl2.chans[6].keyon(opl2);
					//cout << "BASSDRUM" << endl;
				}
				//this sounds probably wrong
				if (d&0x08)
				{
					opl2.chans[7].ops[1].reset_ADSR();
					//cout << "SNARE" << endl;
				}
				//this sounds friggin' awful. for now.
				if (d&0x04)
				{
					//opl2.chans[8].ops[0].reset_ADSR();
					//cout << "TOMTOM" << endl;
				}
				//these sound inadequate, until i come up with a better implementation
				if (d&0x02)
				{
					opl2.chans[8].ops[1].reset_ADSR();
					//cout << "CYMBAL" << endl;
				}
				if (d&0x01)
				{
					opl2.chans[7].ops[0].reset_ADSR();
					//cout << "HI-HAT" << endl;
				}
				//cout << "|||||||||||||||||||" << endl;
			}
			break;
		}
		if (r&0x10) //0xB?
		{
			opl2.chans[r&0x0F].hifnum = word(d&0x03);
			opl2.chans[r&0x0F].oct = uchar((d>>2)&0x07);
			if ((d>>5)&0x01)
				opl2.chans[r&0x0F].keyon(opl2);
			else
				opl2.chans[r&0x0F].keyoff(opl2);
		}
		else //0xA?
			opl2.chans[r&0x0F].lofnum = word(d);
		opl2.chans[r&0x0F].updfreq(opl2);
		break;
	case 0xC0:
		opl2.chans[r&0x0F].algo = d&0x01;
		opl2.chans[r&0x0F].feedback = uchar((d>>1)&0x07);
		break;
	case 0xE0:
		opl2.chans[chn[r&0x1F]].ops[opn[r&0x1F]].wavetype = uchar(d&0x03);
		break;
	}
}

void Opl2::update_ADSR()
{
	for(int c=0; c<OPL2_CHANNELS; c++)
	{
		for(int o=0; o<2; o++)
		{
		    auto& op = chans[c].ops[o];
			switch(op.adsr)
			{
			case A:
				if (op.A_state >= ADSR_A_SIZE)
				{
					op.DR_state = 0;
					op.DR_div = 0;
					op.adsr = D;
					op.ADSR_volume = 8192;
					break;
				}
				op.ADSR_volume = ADSR_A[op.A_state];
				if (op.A > 0)
					op.A_state += pow4rt2[op.A*4+op.rof];
				break;
			case D:
				if (op.ADSR_volume <= ADSR_S[op.S])
				{
					op.ADSR_volume = ADSR_S[op.S];
					op.adsr = (op.hold_instr?S:R);
					break;
				}
            [[fallthrough]]
			case R:
				if (op.DR_div <= 16)
                    op.ADSR_volume = ADSR_DR[op.DR_state]>>op.DR_div;
                else
                    op.adsr = R;
				if ((op.adsr==D?op.D:op.R) > 0)
					op.DR_state += pow4rt2[(op.adsr==D?op.D:op.R)*4+op.rof];
				while (op.DR_state >= ADSR_DR_SIZE)
				{
					op.DR_state -= ADSR_DR_SIZE;
					++op.DR_div;
				}
				if (op.ADSR_volume < 32 || op.DR_div > 16)
					op.adsr = N;
				break;
			case S:
				if (chans[c].note == false)
					op.adsr = R;
				break;
			default:
				break;
			}
		}
	}
}

int OP::update(int fm, Opl2& opl2)
{
	int sample=0;
	unsigned int tempstate = (sinestate+fm);
	tempstate &= (PERIOD_SIZE-1);

	sample = sine[wavetype][tempstate>>SINE_BLOCK]
	*TOTLVL[volume]/ADSR_MAX
		*ADSR_volume/ADSR_MAX
		*LKS[lks][freq]/ADSR_MAX;
	if (ampmod)
		sample = sample*AMPMOD[opl2.ampmod_depth][opl2.ampmod_state]/ADSR_MAX;
	sinestate += freq_harmonic;
	sinestate &= (PERIOD_SIZE-1);
	wanha2 = wanha1;
	wanha1 = sample;
	return sample;
}

//the meat of the program :>
//a single run on the opl2 chip.
//returns a 16bit mono sample
short Opl2::update()
{
    Opl2& opl2 = *this;

	vibrato_state++;
	if (vibrato_state >= VIBRATO_SIZE)
		vibrato_state=0;
	ampmod_state++;
	if (ampmod_state >= AMPMOD_SIZE)
		ampmod_state=0;
	int sini=0, fb=0;

	int maxc;
	if (opl2.rhythm)
		maxc = OPL2_CHANNELS-3;
	else
		maxc = OPL2_CHANNELS;
	update_ADSR();
	for(int c=0; c<maxc; c++)
	{
		fb = FB[chans[c].feedback]*(chans[c].ops[0].wanha1+chans[c].ops[0].wanha2)/(2*SAMPLERATE_DIV);
		int sample=0;

		if (chans[c].ops[0].adsr != N)
		{
			sample = chans[c].ops[0].update(fb, opl2);
		}
		if (chans[c].ops[1].adsr != N)
		{
			switch(chans[c].algo)
			{
			case 0://FM
				sample = chans[c].ops[1].update(FM_AMOUNT*sample, opl2);
				break;
			case 1://AM
				sample += chans[c].ops[1].update(0, opl2);
				sample /= 2;
				break;
			}
			sini += sample;
		}
	}
	if (opl2.rhythm)
	{
		//bass drum
		{
			int sample = 0;
			switch(chans[6].algo)
			{
			case 0://FM
				sample = chans[6].ops[1].update(chans[6].ops[0].update(0, opl2), opl2);
				break;
			case 1://only OP1!!
				sample = chans[6].ops[1].update(0, opl2);
				break;
			}
			sini += sample;
		}

		//the rest
		for(int c=OPL2_CHANNELS-2; c<OPL2_CHANNELS; c++)
		{
			for(int o=0; o<2; o++)
			{
				int sample=(((rand()&0xFF)|((rand()&0xFF)<<8))-32768)*2;
				sample = sample*TOTLVL[chans[c].ops[o].volume]/ADSR_MAX
					*chans[c].ops[o].ADSR_volume/ADSR_MAX
					*LKS[chans[c].ops[o].lks][chans[c].freq]/ADSR_MAX;
				sini += sample;
			}
		}
	}
	return short(sini/OPL2_CHANNELS);
}

void OP::reset_ADSR()
{
	sinestate = 0;
	adsr = ADSR_STATE::A;
	A_state = double_word(ADSR_volume)*ADSR_A_SIZE/ADSR_MAX;
	if (A_state >= ADSR_A_SIZE)
		A_state = ADSR_A_SIZE;
}

void CHANNEL::keyon(Opl2& opl2)
{
	updfreq(opl2);
	if (note == false)
	{
		ops[0].reset_ADSR();
		ops[1].reset_ADSR();
		note = true;
	}
}

void CHANNEL::keyoff(Opl2& opl2)
{
	updfreq(opl2);
	//if the note is turned off during attack phase,
	//we now enter sustain phase
	//at the same volume the attack phase was cut short
	//24.3.2008 FIXME?: Should we enter decay phase, if the volume is
	//higher than the sustain volume ADSR_S[ops[o].S]?
	if (ops[0].adsr == A)
	{
		ops[0].DR_div = VOL2DIV[ops[0].ADSR_volume];
		ops[0].DR_state = VOL2STATE[ops[0].ADSR_volume];
	}
	if (ops[1].adsr == A)
	{
		ops[1].DR_div = VOL2DIV[ops[1].ADSR_volume];
		ops[1].DR_state = VOL2STATE[ops[1].ADSR_volume];
	}

	ops[0].adsr = S;
	ops[1].adsr = S;
	note = false;
}

void CHANNEL::updfreq(Opl2& opl2)
{
	freq = ((hifnum<<8)+lofnum)*pow2[oct];
	//calculate ROF (almost) based on the information on the YMF262 datasheet.
	//if we did things exactly like on the sheet, the last bit would read:
	//(2-2*int(ops[n].ksr). but that makes some sounds too quick.
	for(int i=0; i<2; i++)
	{
		ops[i].freq = freq;
		ops[i].rof = word((((hifnum>>int(opl2.keysplit))&0x01)+2*oct)>>(3-3*int(ops[i].ksr)));
		if (ops[i].harmonic == 0)
			ops[i].freq_harmonic = freq/2;
		else
			ops[i].freq_harmonic = uint(freq)*HARM[ops[i].harmonic];
	}

}


void Opl2::writebyte(int port, int val)
{
	if (port == 0)
		writereg = val;
	else
		write(writereg, val);
}
