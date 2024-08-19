#ifndef SP_CARD_COMMON_DEFINES_H
#define SP_CARD_COMMON_DEFINES_H

#ifdef WIN32
#ifndef __INTEL_OFFLOAD
#include  <io.h>
#endif
#else
typedef long long __int64;
#include <unistd.h>
#endif

#ifdef QT_WIDGETS_LIB
#include <QString>
#include <QFile>
#include <QMessageBox>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifndef ADQAPI
#define ADQ_LEVEL_TRIGGER_MODE 3
#define ADQ_EXT_TRIGGER_MODE 2
#define ADQ_CLOCK_INT_INTREF 0
#define ADQ_CLOCK_EXT 2
#endif

#define MAX_SP_CARDS 8
#define MAX_SP_CHANNELS 4
#define SP_BYTES_PER_SAMPLE 2

struct SP_Devices_Header_Status
{
	unsigned char overRange : 1;	// Record status		byte		Over/under range
	unsigned char fifoFill : 3;		// Record status		byte		FIFO fill factor
	unsigned char lostData : 4;		// Record status		byte		lost data
};
struct SP_Devices_Header
{
	unsigned char status;			// Record Status		Byte		Over/under range, FIFO fill factor and lost data
	unsigned char uid;				// User ID				Byte		A user-configurable value to identify the ADQ14 unit
	unsigned char channel;			// Channel				Byte		The channel number
	unsigned char format;			// Data format			Byte		Information about data
	unsigned int sn;				// Serial number		uint32		Serial number of the ADQ14 digitizer
	unsigned int record;			// Record Number		uint32		The current record number
	unsigned int period;			// SAMPLE_PERIOD		uint32		Time between two samples in 125 ps steps
	long long unsigned timestamp;	// TIME_STAMP			uint64		Time-stamp of trigger event in 125 ps steps
	long long recordstart;			// RECORD_START			int64		Time between trigger event and record start in 125 ps steps
	unsigned int nbytes;			// Record length		uint32		Length of data in record
	unsigned short movingAverage;	// MovingAverage		uint16		Value of the Moving average at trigger point (FWPD)
	unsigned short gateCounter;		// GateCounter			uint16		Gate Counter
};

#define MONSTER_SUITE
#ifdef MONSTER_SUITE

// List of SP Devices serial numbers owned by CIEMAT
#define MONSTER_SP_DEVICES_CARD_00 4574
#define MONSTER_SP_DEVICES_CARD_01 4575
#define MONSTER_SP_DEVICES_CARD_02 5036
#define MONSTER_SP_DEVICES_CARD_03 5037

#define MONSTER_SP_DEVICES_CARD_04 5819
#define MONSTER_SP_DEVICES_CARD_05 5820
#define MONSTER_SP_DEVICES_CARD_06 5821
#define MONSTER_SP_DEVICES_CARD_07 5822
#define MONSTER_SP_DEVICES_CARD_08 5823
#define MONSTER_SP_DEVICES_CARD_09 5824
#define MONSTER_SP_DEVICES_CARD_10 5825
#define MONSTER_SP_DEVICES_CARD_11 5826
#define MONSTER_SP_DEVICES_CARD_12 5827
#define MONSTER_SP_DEVICES_CARD_13 5895

#define MAX_ENERGY_CALCULATIONS 4 // Number of different energy (areas and amplitude) calculations allowed
#define MONSTER_RAWDATA_BLOCKSIZE 40000000
#define MONSTER_ANALYSIS_BLOCKSIZE 6000000

// Data structures
struct SP_Devices_Monster_Data_Header
{
	// The size of this structure is 32 bytes (and the 64-bit variables are well aligned)
	unsigned int nrecord;
	unsigned int nsamples;
	long long timestamp;
	long long recordstart;
	unsigned int position;
	unsigned short moving_average;
	char status;
	char channel;
};
struct SP_Devices_DataBlock_Information // Analysis version of the MONSTER DAQ SP_Devices_Block_Information
{
	// The structure as it is defined now has a size of 512 bytes (and the 64-bit variables are well aligned)
	SP_Devices_DataBlock_Information()
	{
		// [ID]
		b_card = false;
		card_name[0] = 0;
		serial_number[0] = 0;
		firmware[0] = 0;
		crate = 0;
		slot = 0;

		// [General]
		i64Frequency = 1000000000;
		clk_source = ADQ_CLOCK_INT_INTREF;
		clk_impedance = 50;
		clk_output = false;
		trigger_blocking = false;

		// [Acquisition]
		iNumberOfIteration = 0;
		iStartSegment = 0;
		iStartSample = 0;

		for (int i = 0; i < 7; ++i) temperature[i] = 0;
		BlockSize = 0;
		DataSize = 0;
		iIniTimestamp = 0;
		iEndTimestamp = 0;

		// [Channel]
		for (int i = 0; i < MAX_SP_CHANNELS; i++)
		{
			b_channel[i] = false;
			iFullVerticalScale[i] = 0;
			b_UserVerticalScale[i] = false;
			iUserVerticalScale[i] = 0;
			iOffset[i] = 0;
			b_DBS[i] = false;
			iDBS[i] = 0;
		}

		// [Hardware Trigger]
		iHw_TriggerSource = -1;
		iHw_TriggerSlope = 1;
		iHw_TriggerCoupling = 50;
		iHw_TriggerLevel = 0;

		// [Trigger Blocking]
		iBlockingSource = 0;
		iBlockingMode = 0;
		iBlockingWindow = 0;

		// [FWDAQ]
		iFWDAQ_NSegments = 0;
		iFWDAQ_SegmentSize = 0;
		iFWDAQ_Delay = 0;

		// [FWPD]
		for (int i = 0; i < MAX_SP_CHANNELS; i++)
		{
			bFWPD_VariableLength[i] = false;
			iFWPD_NSegments[i] = 0;
			iFWPD_LEW[i] = 0;
			iFWPD_TEW[i] = 0;
			iFWPD_TriggerLevel[i] = 0;
			iFWPD_TriggerSlope[i] = 0;

			bFWPD_ResetLevel[i] = false;
			iFWPD_ResetLevel[i] = 0;
			bFWPD_TriggerArmLevel[i] = false;
			iFWPD_TriggerArmLevel[i] = 0;
			bFWPD_ResetArmLevel[i] = false;
			iFWPD_ResetArmLevel[i] = 0;
			bFWPD_SMA[i] = false;
			iFWPD_SMA[i] = 0;

			b_CoincidenceTrigger[i] = false;
			iCoincidenceSource[i] = 0;
			iCoincidenceWindow[i] = 0;
			iCoincidenceDelay[i] = 0;
		}
	};
	SP_Devices_DataBlock_Information(const SP_Devices_DataBlock_Information *sp_info)
	{
		memcpy(this, sp_info, sizeof(SP_Devices_DataBlock_Information));
	};

	SP_Devices_DataBlock_Information& operator= (const SP_Devices_DataBlock_Information &sp_info)
	{
		if (this == &sp_info) return *this;
		memcpy(this, &sp_info, sizeof(SP_Devices_DataBlock_Information));
		return *this;
	};
	SP_Devices_DataBlock_Information& operator= (const SP_Devices_DataBlock_Information *sp_info)
	{
		if (this == sp_info) return *this;
		memcpy(this, sp_info, sizeof(SP_Devices_DataBlock_Information));
		return *this;
	};

	// [ID]
	char card_name[32];		// 32-bytes null terminated Card Model
	char serial_number[16];	// 16-bytes null terminated serial number
	char firmware[16];		// 16-bytes null terminated Firmware FWDAQ (standard) or FWPD (advanced for triggerless mode)
	unsigned int crate;		// Crate Number
	unsigned int slot;		// Slot Position

	// [General]
	long long i64Frequency;		// Frequency in Hz
	int clk_source;				// 0: Internal clock source, internal 10 MHz reference. 1: Internal clock source, external 10 MHz reference. 2 : External clock source
	unsigned int clk_impedance;	// 50 Ohm / 1 MOhm

	// [Acquisition]
	long long    iStartSegment;		// Starting segment
	long long    iStartSample;		// Starting sample of the segment (the only case it is not 0 is when the user asks very large segments)
	unsigned int iNumberOfIteration;// Iteration number either in multi segment or continuous
	unsigned int temperature[7];	// The temperature in Celsius times 256. There are 7 positions where it can be measured.
	unsigned int BlockSize;			// If RAW data, size of the Tag + Block Header + NFrames + NSamples + Headers + Data. If Processed data, size of the Tag + Block Header + NFrames + NSignals + Analysis Header + Processed Data
	unsigned int DataSize;			// Size of the RAW Data or 0 if Processed Data
	long long    iIniTimestamp;		// First timestamp of the block
	long long    iEndTimestamp;		// Last timestamp of the block

	// [Channel]
	float			FullVerticalScale[MAX_SP_CHANNELS];		// Actual vertical scale set in the channel
	unsigned int	iFullVerticalScale[MAX_SP_CHANNELS];	// Full vertical range in mV of channel (i)
	unsigned int	iUserVerticalScale[MAX_SP_CHANNELS];	// Advanced Full vertical range in mV of channel (i)
	int				iOffset[MAX_SP_CHANNELS];				// DC Offset in mV of channel (i)
	int				iDBS[MAX_SP_CHANNELS];					// Digital Offset (DBS) in mV of channel (i)

	// [Hardware Trigger]
	int iHw_TriggerSource;		// Tigger source. [1-4] for channels 1-4, -1 if external trigger, -2 if SYNC connector
	int	iHw_TriggerCoupling;	// External trigger coupling
	int iHw_TriggerSlope;		// 1 if rising slope / 0 if falling slope
	int iHw_TriggerLevel;		// Trigger level of the hardware trigger in mV (the SYNC connector is a logic signal LVECL or simething like that -> check)

	// [Trigger Blocking]
	int iBlockingSource;			// Channel/ExtTrigger/Sync producing the trigger block
	int iBlockingMode;				// Type of blocking (Once, Window, Gate)
	unsigned int iBlockingWindow;	// For Window mode, the size of the window

	// [FWDAQ]
	unsigned int iFWDAQ_NSegments;   // Number of segments stored in the block
	unsigned int iFWDAQ_SegmentSize; // Number of samples per segment
	int          iFWDAQ_Delay;		 // Number of presamples (if positive, is the number of holdoff samples)

	// [FWPD]
	unsigned int iFWPD_NSegments[MAX_SP_CHANNELS];		// Number of segments stored in the block
	unsigned int iFWPD_LEW[MAX_SP_CHANNELS];			// Pre-Trigger samples
	unsigned int iFWPD_TEW[MAX_SP_CHANNELS];			// Post-Trigger Reset samples
	int iFWPD_TriggerLevel[MAX_SP_CHANNELS];			// Trigger level in mV
	unsigned int iFWPD_TriggerSlope[MAX_SP_CHANNELS];	// 1 if rising slope / 0 if falling slope

	int iFWPD_ResetLevel[MAX_SP_CHANNELS];		// Reset level mV
	int iFWPD_TriggerArmLevel[MAX_SP_CHANNELS];   // Trigger Arm level in mV
	int iFWPD_ResetArmLevel[MAX_SP_CHANNELS];     // Reset Arm level mV
	int iFWPD_SMA[MAX_SP_CHANNELS];        // Moving Average period

	int iCoincidenceSource[MAX_SP_CHANNELS];	// Bitmask channels
	int iCoincidenceWindow[MAX_SP_CHANNELS];	// Window Length after coincidence trigger where triggers are accepted
	int iCoincidenceDelay[MAX_SP_CHANNELS];		// Not implemented yet

	// Booleans (I have converted them on char to ensure a good alignment)
	char b_card;						// Boolean to mark the card as defined or not

	char clk_output;			// Clock output is enabled/disabled
	char trigger_blocking;		// Trigger Blocking feature enabled/disabled
	char timestamp_sync;		// Multi-card synchronization of the timestamp performed

	char b_channel[MAX_SP_CHANNELS];	// Boolean to indicate if channels are active
	char b_UserVerticalScale[MAX_SP_CHANNELS];	// Boolean to indicate user vertical range is active
	char b_DBS[MAX_SP_CHANNELS];				// Boolean to indicate if DBS is active

	char bFWPD_VariableLength[MAX_SP_CHANNELS];	// Boolean to indicate if record is variable length
	char bFWPD_ResetLevel[MAX_SP_CHANNELS];		// Boolean to indicate if Reset level is active
	char bFWPD_TriggerArmLevel[MAX_SP_CHANNELS];// Boolean to indicate if Trigger Arm level is active
	char bFWPD_ResetArmLevel[MAX_SP_CHANNELS];  // Boolean to indicate if Reset Arm level is active
	char bFWPD_SMA[MAX_SP_CHANNELS];			// Boolean to indicate if Moving Average period is active
	char b_CoincidenceTrigger[MAX_SP_CHANNELS];	// Enable/Disable the trigger coincidence
};

// Analysis Parameters

struct MONSTER_Signal_Detection_Struct
{
	double threshold;

	int signal_from; // Usado solo para detección de pileup
	int signal_to;   // Usado solo para detección de pileup. 
	//Podrían suprimirse y marcar como pileup siempre que se detecten 2 o + señales por frame.

	bool slope;
	// true - positive
	//false - negative
};

struct MONSTER_Signal_Energy_Struct
{
	int range_from; // Usado para determinar rango donde calcular maximo, y área
	int range_to; // Usado para determinar rango donde calcular maximo, y área
	bool active;
};

struct MONSTER_Channel_Analysis_Struct
{
	MONSTER_Signal_Detection_Struct detection;
	MONSTER_Signal_Energy_Struct energy[MAX_ENERGY_CALCULATIONS];
	bool active;
};


///// Analysis Results /////////////

struct MONSTER_Signal_Results
{

	short pileup;
	short saturated;
	unsigned int nsignal;

	double time;

	// Energy
	double energy[MAX_ENERGY_CALCULATIONS];
	// [0]- Amplitud
	// [1]- Area 1
	// [2]- Area 2
};

struct MONSTER_Record_Results
{
	// Estructura completa sin cambios, por el momento
	unsigned int nrecord;
	unsigned int nsignals;
	float baseline;
	float stdbaseline;
	long long timestamp;

	unsigned int position;
	short int recordstart;
	unsigned short int channel;
};
#endif

#endif // SP_CARD_COMMON_DEFINES_H
