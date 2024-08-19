#ifndef SP_DEVICES_DATAREAD_H
#define SP_DEVICES_DATAREAD_H

#include <Common_sp_devices_defines.h>
#include <mainwindow.h>

struct SP_Devices_BlockDataInfo_Struct
{
	int data_position[MAX_SP_CHANNELS]; // Position of the channels in the block (-1 if channel not present)
	int headers_position[MAX_SP_CHANNELS]; // Position of the headers in the block (-1 if channel not present)
	int timestamps_position[MAX_SP_CHANNELS]; // Position of the timestamps in the Timestamps vector
	unsigned int nsegments[MAX_SP_CHANNELS]; // Number of segments per channel
	std::vector <int> segments_position[MAX_SP_CHANNELS]; // Position of the segments in the block (-1 if channel not present)
	std::vector <int> nsamples[MAX_SP_CHANNELS]; // Number of samples per segment (-1 if channel not present)

	void clear()
	{
		for (int ch = 0; ch < MAX_SP_CHANNELS; ++ch)
		{
			data_position[ch] = -1;
			headers_position[ch] = -1;
			timestamps_position[ch] = -1;
			nsegments[ch] = 0;
			segments_position[ch].clear();
			nsamples[ch].clear();
		}
	};
};


struct SP_Devices_BlockInfo_Struct_v2
{
	SP_Devices_BlockInfo_Struct_v2(const unsigned int b = 0)
	{
		niteration = -1;
		nblock = b;
		block_position = 0;
		block_size = 0;
		nsegments = 0;
	};

	unsigned int niteration; // Iteration number
	unsigned int nblock; // Block number at the iteration
	long long block_position; // Position in file of the block
	long long block_size; // Size of the iteration in bytes
	unsigned int nsegments; // Number of segments available at partial iteration
	long long nsetsegment[MAX_SP_CHANNELS]; // Start segment relative to the file set start
	SP_Devices_DataBlock_Information sp_devices; // Information of the card at partial iteration time
};

struct SP_Devices_FileInfo_Struct_v2
{
	SP_Devices_FileInfo_Struct_v2()
	{
		nfile = 0;
		nblocks = 0;
	};
	char filename[320]; // SP Devices data file name
	int cardid; // Card number (-1 if not valid)
	unsigned int nfile; // Number of file
	unsigned int nblocks; // Number of data blocks stored in the file
	bool isFWDAQ; // FWDAQ or FWPD
	std::vector<SP_Devices_BlockInfo_Struct_v2> info_block; // This array will contain the relevant information of the block
};

class SP_Devices_DataRead_Class_v2
{
public:
	SP_Devices_DataRead_Class_v2(char *fulliteration, long long *timestamps);
	~SP_Devices_DataRead_Class_v2();

	bool Initialise_Acquisition_File(const char *filename, const bool reset = true);
    bool Initialise_Acquisition_File_Plotter(const char *filename, const bool reset=true /**/);
	bool Initialise_Acquisition_FileSet(const char *dirname, const int card);

	unsigned int GetNIterations() const; // Get number of blocks NOT iterations in the fileset
	unsigned int GetNIterations(unsigned int &nblocks) const; // Get number of iterations in the fileset. The number of blocks is returned by reference
	unsigned int GetNBlocksInIteration(const unsigned int niteration) const; // Get number of blocks in iteration niteration

	bool GetIteration(const unsigned int nblock); // In this case, iteration refers to block
	bool GetIteration(const unsigned int niteration, const unsigned int nblock); // Since iterations usually don't fit in a single block, to traverse an iteration, use this function
	const char *GetPointerIteration(const unsigned int nblock); // In this case, iteration refers to block
	const short *GetChannel(const unsigned int channel, unsigned int &nsegments, unsigned int *segsize = NULL); // General function for getting a channel
	unsigned int GetNSegments(const unsigned int channel) const; // Gets the number of segments available in the loaded block for the requested channel
	const short *GetSegment(const unsigned int channel, const unsigned int nsegment, unsigned int &segsize);
	const void *GetSetSegment(const int channel, const unsigned int nsegment, unsigned int &nsamples, long long &timestamp);
	const long long *GetTimestamps(const unsigned int channel);

	// General information variables
	bool IsVariableSize(const unsigned int channel) const;
	double GetFullVerticalScale(const unsigned int channel) const;
	int GetNPreSamples(const unsigned int channel) const;
	double GetVerticalOffset(const unsigned int channel) const;
	double RawTomV(const unsigned int channel) const;
	double SampleTons() const;
	bool IsChannelActive(const unsigned int channel) const;
	int GetCardId() const;
	long long GetMaxSetSegments(const unsigned int channel) const;
    void Set_MainWindow(MainWindow *pMainWindow);
	void *GetHeader() const
	{
		if (!IsFileSetInitialised) return NULL;
		return (void *)&CurrentBlockInfo->sp_devices;
	};
	long long GetLiveTime(const unsigned int channel, long long &measurement_time);

private:
	char* const FullIteration; // Block of memory to store a whole iteration;
	long long* const Timestamps; // Block of memory to store the timestamps
    MainWindow* m_pMainWindow;

	bool IsFileSetInitialised;
	std::vector<SP_Devices_FileInfo_Struct_v2> FileSet;
	const SP_Devices_BlockInfo_Struct_v2 *CurrentBlockInfo;
	SP_Devices_BlockDataInfo_Struct BlockDataInfo;

	inline bool isSPLabelCorrect(const char *label, const char *filename) const;

	inline void fill_BlockDataInfo_Timestamps_FWDAQ();
	inline void fill_BlockDataInfo_Timestamps_FWPD();

	inline unsigned int get_nchannels();
};
#endif // SP_DEVICES_DATAREAD_H
