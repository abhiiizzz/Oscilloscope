#include <Common_sp_devices_read.h>


#define SP_LABEL_SIZE 23
#define MAX_SP_NFILES 1000
#ifndef forever
#define forever while(true)
#endif

SP_Devices_DataRead_Class_v2::SP_Devices_DataRead_Class_v2(char *fulliteration, long long *timestamps) : FullIteration(fulliteration), Timestamps(timestamps)
{
}
SP_Devices_DataRead_Class_v2::~SP_Devices_DataRead_Class_v2()
{
}

void SP_Devices_DataRead_Class_v2::Set_MainWindow(MainWindow *pMainWindow) {
    m_pMainWindow = pMainWindow;
}

bool SP_Devices_DataRead_Class_v2::Initialise_Acquisition_File_Plotter(const char *filename, const bool reset /**/) {
    SP_Devices_FileInfo_Struct_v2 _FileInfo;

    if (reset)
    {
        FileSet.clear(); // Reset the fileset
        IsFileSetInitialised = false;
    }

    sprintf(_FileInfo.filename, "%s", filename);

    FILE *file = fopen(_FileInfo.filename, "rb");
    if (file == NULL) return false;

    char label[SP_LABEL_SIZE] = { 0 };
    fread(label, sizeof(char), SP_LABEL_SIZE - 1, file);
    if (!isSPLabelCorrect(label, filename)) return false;

    SP_Devices_BlockInfo_Struct_v2 _BlockInfo;

    // Read Card Information
    fread(&_BlockInfo.sp_devices, sizeof(SP_Devices_DataBlock_Information), 1, file);
    _BlockInfo.niteration = _BlockInfo.sp_devices.iNumberOfIteration;
    fread(&_BlockInfo.nsegments, sizeof(unsigned int), 1, file);
//	_BlockInfo.nsegments = _BlockInfo.sp_devices.iNumberOfSegments;
    _BlockInfo.nblock = 0;

    // Get the card number
    {
        int card_number;
        sscanf(_BlockInfo.sp_devices.serial_number, "SPD-%d", &card_number);
        switch (card_number)
        {
        case MONSTER_SP_DEVICES_CARD_00: _FileInfo.cardid = MONSTER_SP_DEVICES_CARD_00; break;
        case MONSTER_SP_DEVICES_CARD_01: _FileInfo.cardid = MONSTER_SP_DEVICES_CARD_01; break;
        case MONSTER_SP_DEVICES_CARD_02: _FileInfo.cardid = MONSTER_SP_DEVICES_CARD_02; break;
        case MONSTER_SP_DEVICES_CARD_03: _FileInfo.cardid = MONSTER_SP_DEVICES_CARD_03; break;
        default: _FileInfo.cardid = -1;
        }
    }

    // Get the number of channels in the file
    unsigned int nchannels = 0;
    for (int i = 0; i<MAX_SP_CHANNELS; i++) if (_BlockInfo.sp_devices.b_channel[i]) ++nchannels;

    // There are two option: FWDAQ and FWPD
    _FileInfo.isFWDAQ = _BlockInfo.sp_devices.iFWDAQ_NSegments == 0 ? false : true;

    // I check if previous file exists and if the last iteration is the same than for this one
    if (FileSet.size())
    {
        const unsigned int lastfs = FileSet.size() - 1;
        const SP_Devices_BlockInfo_Struct_v2 &last_block = FileSet.at(lastfs).info_block.at(FileSet.at(lastfs).info_block.size() - 1);
        for (int i = 0; i<MAX_SP_CHANNELS; i++) if (_BlockInfo.sp_devices.b_channel[i]) _BlockInfo.nsetsegment[i] = last_block.nsetsegment[i];
        if (last_block.niteration == _BlockInfo.niteration)
        {
            // I have to check if the (sp_devices.iStartSegment+sp_devices.iNumberOfSegments) of last file equals (sp_devices.iStartSegment-1) of current file
            const unsigned int ls = _FileInfo.isFWDAQ ? last_block.sp_devices.iStartSegment + last_block.nsegments / nchannels : last_block.sp_devices.iStartSegment + last_block.nsegments;
            if (ls == _BlockInfo.sp_devices.iStartSegment - 1) _BlockInfo.nblock = last_block.nblock + 1;
            else _BlockInfo.nblock = last_block.nblock + 2; // So, there will be a gap!
        }
    }
    else
    {
        // Initialize the start set segment number. (No FWPD done yet)
        for (int i = 0; i < MAX_SP_CHANNELS; i++) if (_BlockInfo.sp_devices.b_channel[i])
        {
            if (_FileInfo.isFWDAQ) _BlockInfo.nsetsegment[i] = -(long long)_BlockInfo.sp_devices.iFWDAQ_NSegments;
            else _BlockInfo.nsetsegment[i] = -(long long)_BlockInfo.sp_devices.iFWPD_NSegments[i];
        }
    }



    long long block_position = 0; // Position in file of the last block

    forever
    {
        if(m_pMainWindow->isPaused()) { QApplication::processEvents(); continue; }
        QApplication::processEvents();
        // Update the start set segment number. (No FWPD done yet)
        for (int i = 0; i < MAX_SP_CHANNELS; i++) if (_BlockInfo.sp_devices.b_channel[i])
        {
            if (_FileInfo.isFWDAQ) _BlockInfo.nsetsegment[i] += _BlockInfo.sp_devices.iFWDAQ_NSegments;
            else _BlockInfo.nsetsegment[i] += _BlockInfo.sp_devices.iFWPD_NSegments[i];
        }

        // The jump to read the next sp_devices header is done at the end, so I assume that the update of variables has to be done here

        if (_FileInfo.nblocks)
        {
            const SP_Devices_BlockInfo_Struct_v2 &last_block = _FileInfo.info_block.at(_FileInfo.nblocks - 1);
            if (last_block.niteration == _BlockInfo.niteration)
            {
                // I have to check if the (sp_devices.iStartSegment+sp_devices.iNumberOfSegments) of last block equals (sp_devices.iStartSegment-1) of current block
                const unsigned int ls = _FileInfo.isFWDAQ ? last_block.sp_devices.iStartSegment + last_block.nsegments / nchannels : last_block.sp_devices.iStartSegment + last_block.nsegments;
                if (ls == _BlockInfo.sp_devices.iStartSegment - 1) _BlockInfo.nblock = last_block.nblock + 1;
                else _BlockInfo.nblock = last_block.nblock + 2; // So, there will be a gap!
            }
            else _BlockInfo.nblock = 0;
        }
        //
        // Iteration size
        //
        // 1. Get the header block size
        const long long header_block_size = sizeof(SP_Devices_DataBlock_Information);

        // 2. Get the data block size
        long long data_block_size = _BlockInfo.sp_devices.DataSize;

        // 3. Get the segment headers block size
        long long data_headers_block_size = (long long)_BlockInfo.nsegments * sizeof(SP_Devices_Monster_Data_Header);

        // I can calculate now the iteration size
        const long long iteration_size = _BlockInfo.sp_devices.BlockSize;

        // Store the block position in file and calculate the new position
        _BlockInfo.block_position = block_position;
        _BlockInfo.block_size = iteration_size;
        block_position += iteration_size;

        // Add block information to the local _FileInfo
        _FileInfo.info_block.push_back(_BlockInfo);
        ++_FileInfo.nblocks;

        fseek(file, _BlockInfo.block_position, SEEK_SET);
        const long long read_bytes = fread(FullIteration, sizeof(char), _BlockInfo.block_size, file);
        if (read_bytes != _BlockInfo.block_size) break;


        // AYAN : Here we will use FullIteration and _BlockInfo for extracting the data required.
            // fill_BlockDataInfo_Timestamps_FWDAQ
        // 4.FWDAQ.1 Constants
            CurrentBlockInfo = &_BlockInfo;
            const unsigned int channel_size = CurrentBlockInfo->sp_devices.iFWDAQ_NSegments * CurrentBlockInfo->sp_devices.iFWDAQ_SegmentSize * sizeof(short);
            const unsigned int nchannels = get_nchannels();
            const unsigned int start_data = SP_LABEL_SIZE - 1 + sizeof(SP_Devices_DataBlock_Information) + 2 *sizeof(unsigned int);

            const SP_Devices_Monster_Data_Header *headers = (SP_Devices_Monster_Data_Header *)(&FullIteration[start_data + nchannels * channel_size]);

            // 4.FWDAQ.2 Fill the timestamps and BlockDataInfo
            unsigned int ichannel = 0;
            for (int channel = 0; channel < MAX_SP_CHANNELS; ++channel) if (CurrentBlockInfo->sp_devices.b_channel[channel])
            {
                BlockDataInfo.nsegments[channel] = CurrentBlockInfo->sp_devices.iFWDAQ_NSegments;
                BlockDataInfo.segments_position[channel].resize(CurrentBlockInfo->sp_devices.iFWDAQ_NSegments);
                BlockDataInfo.nsamples[channel].resize(CurrentBlockInfo->sp_devices.iFWDAQ_NSegments);
                BlockDataInfo.timestamps_position[channel] = 0;
                BlockDataInfo.data_position[channel] = start_data + ichannel * channel_size;
                ++ichannel;
            }
            const unsigned int start_record = headers[0].nrecord;
            unsigned int records_accum[MAX_SP_CHANNELS] = { 0 };
            for (int s = 0; s < CurrentBlockInfo->nsegments; ++s)
            {
                const int channel = headers[s].channel;
                const unsigned int nrecord = records_accum[channel];
                ++records_accum[channel];

                BlockDataInfo.segments_position[channel][nrecord] = start_data + sizeof(short) * headers[s].position;
                BlockDataInfo.nsamples[channel][nrecord] = headers[s].nsamples;
                Timestamps[nrecord] = headers[s].timestamp;
            }
            // fill_BlockDataInfo_Timestamps_FWDAQ : END
            std::vector<QVector<double>> x(ichannel),y(ichannel);
            m_pMainWindow->setNumberOfChannels(ichannel);
            const SP_Devices_DataBlock_Information *_sp_devices = (SP_Devices_DataBlock_Information *)(&FullIteration[22]);
            for(int channel=0; channel < ichannel; channel++)
            {
                const int offset = _sp_devices->iOffset[channel]; // Offset in mV
                const int delay = _sp_devices->firmware[2]=='D'? _sp_devices->iFWDAQ_Delay : _sp_devices->iFWPD_LEW[channel]; // Delay (ns)

                unsigned int buffer_npoints;
                const unsigned int nsegment = 0;
                if (BlockDataInfo.nsegments[channel] <= nsegment) return NULL;
                buffer_npoints = BlockDataInfo.nsamples[channel].at(nsegment);
                const short *stream_of_data = (short *)&FullIteration[BlockDataInfo.segments_position[channel].at(nsegment)];
                const double tomV = CurrentBlockInfo->sp_devices.FullVerticalScale[channel] / 65536;
                for(int i=0; i<buffer_npoints; i++) {
                    // i + delay, stream_of_data[i] * tomV - offset

                    x[channel].push_back(i+delay);
                    y[channel].push_back(stream_of_data[i]*tomV-offset);
                }
            }
            m_pMainWindow->setx(x);
            m_pMainWindow->sety(y);
            for( int channel = 0; channel < ichannel; channel++ )
            {
                emit m_pMainWindow->plotting(channel);
                QApplication::processEvents();
            }
            emit m_pMainWindow->eventReceived();
            QApplication::processEvents();
            QThread::msleep(m_pMainWindow->getSleepDelay());
        // AYAN END

        // Jump to the next iteration (remember that I have already read the tag, sp_devices header, nsegments)
//#ifdef WIN32
//        _fseeki64(file, sizeof(unsigned int)+data_block_size + data_headers_block_size, SEEK_CUR);
//#else
//        fseeko(file, sizeof(unsigned int)+data_block_size + data_headers_block_size, SEEK_CUR);
//#endif

        // Read the label and header
        if (fread(label, sizeof(char), SP_LABEL_SIZE - 1, file) != SP_LABEL_SIZE - 1) break;
        if (!isSPLabelCorrect(label, filename)) break;
        if (fread(&_BlockInfo.sp_devices, sizeof(SP_Devices_DataBlock_Information), 1, file) != 1) break;

        // Update npartialiterations and total_segments
        _BlockInfo.niteration = _BlockInfo.sp_devices.iNumberOfIteration;
        if (fread(&_BlockInfo.nsegments, sizeof(unsigned int), 1, file) != 1) break;
//		_BlockInfo.nsegments = _BlockInfo.sp_devices.iNumberOfSegments;
    }

    fclose(file);

    IsFileSetInitialised = true; // The fileset is now initialised

    FileSet.push_back(_FileInfo);

    CurrentBlockInfo = FileSet.at(0).info_block.data();

    return true;
}

bool SP_Devices_DataRead_Class_v2::Initialise_Acquisition_File(const char *filename, const bool reset)
{
	SP_Devices_FileInfo_Struct_v2 _FileInfo;

	if (reset)
	{
		FileSet.clear(); // Reset the fileset
		IsFileSetInitialised = false;
	}

	sprintf(_FileInfo.filename, "%s", filename);

	FILE *file = fopen(_FileInfo.filename, "rb");
	if (file == NULL) return false;

	char label[SP_LABEL_SIZE] = { 0 };
	fread(label, sizeof(char), SP_LABEL_SIZE - 1, file);
	if (!isSPLabelCorrect(label, filename)) return false;

	SP_Devices_BlockInfo_Struct_v2 _BlockInfo;

	// Read Card Information
	fread(&_BlockInfo.sp_devices, sizeof(SP_Devices_DataBlock_Information), 1, file);
	_BlockInfo.niteration = _BlockInfo.sp_devices.iNumberOfIteration;
	fread(&_BlockInfo.nsegments, sizeof(unsigned int), 1, file);
//	_BlockInfo.nsegments = _BlockInfo.sp_devices.iNumberOfSegments;
	_BlockInfo.nblock = 0;

	// Get the card number
	{
		int card_number;
		sscanf(_BlockInfo.sp_devices.serial_number, "SPD-%d", &card_number);
		switch (card_number)
		{
		case MONSTER_SP_DEVICES_CARD_00: _FileInfo.cardid = MONSTER_SP_DEVICES_CARD_00; break;
		case MONSTER_SP_DEVICES_CARD_01: _FileInfo.cardid = MONSTER_SP_DEVICES_CARD_01; break;
		case MONSTER_SP_DEVICES_CARD_02: _FileInfo.cardid = MONSTER_SP_DEVICES_CARD_02; break;
		case MONSTER_SP_DEVICES_CARD_03: _FileInfo.cardid = MONSTER_SP_DEVICES_CARD_03; break;
		default: _FileInfo.cardid = -1;
		}
	}

	// Get the number of channels in the file
	unsigned int nchannels = 0;
	for (int i = 0; i<MAX_SP_CHANNELS; i++) if (_BlockInfo.sp_devices.b_channel[i]) ++nchannels;

	// There are two option: FWDAQ and FWPD
	_FileInfo.isFWDAQ = _BlockInfo.sp_devices.iFWDAQ_NSegments == 0 ? false : true;

	// I check if previous file exists and if the last iteration is the same than for this one
	if (FileSet.size())
	{
		const unsigned int lastfs = FileSet.size() - 1;
		const SP_Devices_BlockInfo_Struct_v2 &last_block = FileSet.at(lastfs).info_block.at(FileSet.at(lastfs).info_block.size() - 1);
		for (int i = 0; i<MAX_SP_CHANNELS; i++) if (_BlockInfo.sp_devices.b_channel[i]) _BlockInfo.nsetsegment[i] = last_block.nsetsegment[i];
		if (last_block.niteration == _BlockInfo.niteration)
		{
			// I have to check if the (sp_devices.iStartSegment+sp_devices.iNumberOfSegments) of last file equals (sp_devices.iStartSegment-1) of current file
			const unsigned int ls = _FileInfo.isFWDAQ ? last_block.sp_devices.iStartSegment + last_block.nsegments / nchannels : last_block.sp_devices.iStartSegment + last_block.nsegments;
			if (ls == _BlockInfo.sp_devices.iStartSegment - 1) _BlockInfo.nblock = last_block.nblock + 1;
			else _BlockInfo.nblock = last_block.nblock + 2; // So, there will be a gap!
		}
	}
	else
	{
		// Initialize the start set segment number. (No FWPD done yet)
		for (int i = 0; i < MAX_SP_CHANNELS; i++) if (_BlockInfo.sp_devices.b_channel[i])
		{
			if (_FileInfo.isFWDAQ) _BlockInfo.nsetsegment[i] = -(long long)_BlockInfo.sp_devices.iFWDAQ_NSegments;
			else _BlockInfo.nsetsegment[i] = -(long long)_BlockInfo.sp_devices.iFWPD_NSegments[i];
		}
	}



	long long block_position = 0; // Position in file of the last block

	forever
	{
		// Update the start set segment number. (No FWPD done yet)
		for (int i = 0; i < MAX_SP_CHANNELS; i++) if (_BlockInfo.sp_devices.b_channel[i])
		{
			if (_FileInfo.isFWDAQ) _BlockInfo.nsetsegment[i] += _BlockInfo.sp_devices.iFWDAQ_NSegments;
			else _BlockInfo.nsetsegment[i] += _BlockInfo.sp_devices.iFWPD_NSegments[i];
		}

		// The jump to read the next sp_devices header is done at the end, so I assume that the update of variables has to be done here

		if (_FileInfo.nblocks)
		{
			const SP_Devices_BlockInfo_Struct_v2 &last_block = _FileInfo.info_block.at(_FileInfo.nblocks - 1);
			if (last_block.niteration == _BlockInfo.niteration)
			{
				// I have to check if the (sp_devices.iStartSegment+sp_devices.iNumberOfSegments) of last block equals (sp_devices.iStartSegment-1) of current block
				const unsigned int ls = _FileInfo.isFWDAQ ? last_block.sp_devices.iStartSegment + last_block.nsegments / nchannels : last_block.sp_devices.iStartSegment + last_block.nsegments;
				if (ls == _BlockInfo.sp_devices.iStartSegment - 1) _BlockInfo.nblock = last_block.nblock + 1;
				else _BlockInfo.nblock = last_block.nblock + 2; // So, there will be a gap!
			}
			else _BlockInfo.nblock = 0;
		}

		//
		// Iteration size
		//
		// 1. Get the header block size
		const long long header_block_size = sizeof(SP_Devices_DataBlock_Information);

		// 2. Get the data block size
		long long data_block_size = _BlockInfo.sp_devices.DataSize;

		// 3. Get the segment headers block size
		long long data_headers_block_size = (long long)_BlockInfo.nsegments * sizeof(SP_Devices_Monster_Data_Header);

		// I can calculate now the iteration size
		const long long iteration_size = _BlockInfo.sp_devices.BlockSize;

		// Store the block position in file and calculate the new position
		_BlockInfo.block_position = block_position;
		_BlockInfo.block_size = iteration_size;
		block_position += iteration_size;

		// Add block information to the local _FileInfo
		_FileInfo.info_block.push_back(_BlockInfo);
		++_FileInfo.nblocks;

		// Jump to the next iteration (remember that I have already read the tag, sp_devices header, nsegments)
#ifdef WIN32
		_fseeki64(file, sizeof(unsigned int)+data_block_size + data_headers_block_size, SEEK_CUR);
#else
		fseeko(file, sizeof(unsigned int)+data_block_size + data_headers_block_size, SEEK_CUR);
#endif

		// Read the label and header
		if (fread(label, sizeof(char), SP_LABEL_SIZE - 1, file) != SP_LABEL_SIZE - 1) break;
		if (!isSPLabelCorrect(label, filename)) break;
		if (fread(&_BlockInfo.sp_devices, sizeof(SP_Devices_DataBlock_Information), 1, file) != 1) break;

		// Update npartialiterations and total_segments
		_BlockInfo.niteration = _BlockInfo.sp_devices.iNumberOfIteration;
		if (fread(&_BlockInfo.nsegments, sizeof(unsigned int), 1, file) != 1) break;
//		_BlockInfo.nsegments = _BlockInfo.sp_devices.iNumberOfSegments;
	}

	fclose(file);

	IsFileSetInitialised = true; // The fileset is now initialised

	FileSet.push_back(_FileInfo);

	CurrentBlockInfo = FileSet.at(0).info_block.data();

	return true;
}
bool SP_Devices_DataRead_Class_v2::Initialise_Acquisition_FileSet(const char *dirname, const int card)
{
	char filename[320];

	IsFileSetInitialised = false;

	for (int i = 0; i < MAX_SP_NFILES; i++)
	{
#ifdef WIN32
		sprintf(filename, "%s\\sp_file_%d_%03d.bin", dirname, card, i);
		if ((_access(filename, 0)) != -1) // Check for existence
#else
		sprintf(filename, "%s/sp_file_%d_%03d.bin", dirname, card, i);
		if ((access(filename, 0)) != -1) // Check for existence (Linux version)
#endif
		{
            Initialise_Acquisition_File_Plotter(filename, !IsFileSetInitialised);
		}
	}
	return IsFileSetInitialised;
}

unsigned int SP_Devices_DataRead_Class_v2::GetNIterations() const
{
	unsigned int nblocks = 0;
	for (int nfile = 0; nfile < FileSet.size(); ++nfile) nblocks += FileSet.at(nfile).nblocks;

	return nblocks;
}
unsigned int SP_Devices_DataRead_Class_v2::GetNIterations(unsigned int &nblocks) const
{
	unsigned int niterations = 0;
	int last_iteration = -1;
	for (int nfile = 0; nfile < FileSet.size(); ++nfile)
	{
		nblocks += FileSet.at(nfile).nblocks;
		for (int nblock = 0; nblock < FileSet.at(nfile).nblocks; ++nblock)
		{
			if (FileSet.at(nfile).info_block.at(nblock).niteration != last_iteration)
			{
				++niterations;
				last_iteration = FileSet.at(nfile).info_block.at(nblock).niteration;
			}
		}
	}
	return niterations;
}
unsigned int SP_Devices_DataRead_Class_v2::GetNBlocksInIteration(const unsigned int niteration) const
{
	unsigned int nblocks = 0;
	for (int nfile = 0; nfile < FileSet.size(); ++nfile)
	for (int nblock = 0; nblock < FileSet.at(nfile).nblocks; ++nblock)
	if (FileSet.at(nfile).info_block.at(nblock).niteration == niteration) ++nblocks;

	return nblocks;
}

bool SP_Devices_DataRead_Class_v2::GetIteration(const unsigned int nblock)
{
	// Remember that we are reading a block, not a whole iteration

	// 1. Check the file set is defined
	if (!IsFileSetInitialised) return false;

	// 2. Get the file storing nblock
	const unsigned int nfiles = FileSet.size();
	unsigned int nfile = -1; // File containing the requested block
	unsigned int ntblocks = 0;
	unsigned int rblock; // Real block number in file of the requested block
	for (int nf = 0; nf < nfiles; ++nf) if (nblock < (ntblocks += FileSet.at(nf).nblocks))
	{
		nfile = nf;
		rblock = nblock - (ntblocks - FileSet.at(nf).nblocks);
		break;
	}
	if (nfile == -1) return false; // The requested block does not exist

	// 3. Clear the BlockDataInfo
	BlockDataInfo.clear();

	// 4. Read the whole iteration
	const SP_Devices_FileInfo_Struct_v2 &_CurrentFileInfo = FileSet.at(nfile);
	CurrentBlockInfo = &_CurrentFileInfo.info_block.at(rblock);
	FILE *file = fopen(_CurrentFileInfo.filename, "rb");
	if (file == NULL) return false;
#ifdef WIN32
	if (CurrentBlockInfo->block_position) _fseeki64(file, CurrentBlockInfo->block_position, SEEK_SET);
#else
	if (CurrentBlockInfo->block_position) fseeko(file, CurrentBlockInfo->block_position, SEEK_SET);
#endif
	const long long read_bytes = fread(FullIteration, sizeof(char), CurrentBlockInfo->block_size, file);
	if (read_bytes != CurrentBlockInfo->block_size) { fclose(file); return false; }
	fclose(file);

	// 5. Fill the BlockDataInfo and Timestamps
	if (_CurrentFileInfo.isFWDAQ) fill_BlockDataInfo_Timestamps_FWDAQ();
	else fill_BlockDataInfo_Timestamps_FWPD();

	return true;
}

const char *SP_Devices_DataRead_Class_v2::GetPointerIteration(const unsigned int nblock)
{
	// Remember that we are reading a block, not a whole iteration

	// 1. Check the file set is defined
	if (!IsFileSetInitialised) return NULL;

	// 2. Get the file storing nblock
	const unsigned int nfiles = FileSet.size();
	unsigned int nfile = -1; // File containing the requested block
	unsigned int ntblocks = 0;
	unsigned int rblock; // Real block number in file of the requested block
    for (int nf = 0; nf < nfiles; ++nf) if (nblock < (ntblocks += FileSet.at(nf).nblocks))
    {
        nfile = nf;
        rblock = nblock - (ntblocks - FileSet.at(nf).nblocks);
        break;
    }

	if (nfile == -1) return NULL; // The requested block does not exist

	// 3. Clear the BlockDataInfo
	BlockDataInfo.clear();

	// 4. Read the whole iteration
	const SP_Devices_FileInfo_Struct_v2 &_CurrentFileInfo = FileSet.at(nfile);
	CurrentBlockInfo = &_CurrentFileInfo.info_block.at(rblock);
	FILE *file = fopen(_CurrentFileInfo.filename, "rb");
	if (file == NULL) return NULL;
#ifdef WIN32
	if (CurrentBlockInfo->block_position) _fseeki64(file, CurrentBlockInfo->block_position, SEEK_SET);
#else
	if (CurrentBlockInfo->block_position) fseeko(file, CurrentBlockInfo->block_position, SEEK_SET);
#endif
	const long long read_bytes = fread(FullIteration, sizeof(char), CurrentBlockInfo->block_size, file);
	if (read_bytes != CurrentBlockInfo->block_size) { fclose(file); return NULL; }
	fclose(file);

	// 5. Fill the BlockDataInfo and Timestamps
    if (_CurrentFileInfo.isFWDAQ) fill_BlockDataInfo_Timestamps_FWDAQ();
    else fill_BlockDataInfo_Timestamps_FWPD();

	return FullIteration;
}


void SP_Devices_DataRead_Class_v2::fill_BlockDataInfo_Timestamps_FWDAQ() // Need to check this function well
{
	// 4.FWDAQ.1 Constants
	const unsigned int channel_size = CurrentBlockInfo->sp_devices.iFWDAQ_NSegments * CurrentBlockInfo->sp_devices.iFWDAQ_SegmentSize * sizeof(short);
	const unsigned int nchannels = get_nchannels();
	const unsigned int start_data = SP_LABEL_SIZE - 1 + sizeof(SP_Devices_DataBlock_Information) + 2 *sizeof(unsigned int);

	const SP_Devices_Monster_Data_Header *headers = (SP_Devices_Monster_Data_Header *)(&FullIteration[start_data + nchannels * channel_size]);

	// 4.FWDAQ.2 Fill the timestamps and BlockDataInfo
	unsigned int ichannel = 0;
	for (int channel = 0; channel < MAX_SP_CHANNELS; ++channel) if (CurrentBlockInfo->sp_devices.b_channel[channel])
	{
		BlockDataInfo.nsegments[channel] = CurrentBlockInfo->sp_devices.iFWDAQ_NSegments;
		BlockDataInfo.segments_position[channel].resize(CurrentBlockInfo->sp_devices.iFWDAQ_NSegments);
		BlockDataInfo.nsamples[channel].resize(CurrentBlockInfo->sp_devices.iFWDAQ_NSegments);
		BlockDataInfo.timestamps_position[channel] = 0;
		BlockDataInfo.data_position[channel] = start_data + ichannel * channel_size;
		++ichannel;
	}
	const unsigned int start_record = headers[0].nrecord;
	unsigned int records_accum[MAX_SP_CHANNELS] = { 0 };
	for (int s = 0; s < CurrentBlockInfo->nsegments; ++s)
	{
		const int channel = headers[s].channel;
		const unsigned int nrecord = records_accum[channel];
		++records_accum[channel];

		BlockDataInfo.segments_position[channel][nrecord] = start_data + sizeof(short) * headers[s].position;
		BlockDataInfo.nsamples[channel][nrecord] = headers[s].nsamples;
		Timestamps[nrecord] = headers[s].timestamp;
	}
}
void SP_Devices_DataRead_Class_v2::fill_BlockDataInfo_Timestamps_FWPD()
{
	// 4.FWPD.1 Constants
	const unsigned int start_data = SP_LABEL_SIZE - 1 + sizeof(SP_Devices_DataBlock_Information)+2 * sizeof(unsigned int);
	const unsigned int start_headers = start_data + CurrentBlockInfo->sp_devices.DataSize;

	const SP_Devices_Monster_Data_Header *headers = (SP_Devices_Monster_Data_Header *)(&FullIteration[start_headers]);

	// 4.FWPD.3 Fill BlockDataInfo and Timestamps
	int timestamp_start = 0;
	for (int channel = 0; channel < MAX_SP_CHANNELS; ++channel) if (CurrentBlockInfo->sp_devices.b_channel[channel])
	{
		BlockDataInfo.nsegments[channel] = CurrentBlockInfo->sp_devices.iFWPD_NSegments[channel];
		BlockDataInfo.segments_position[channel].resize(CurrentBlockInfo->sp_devices.iFWPD_NSegments[channel]);
		BlockDataInfo.nsamples[channel].resize(CurrentBlockInfo->sp_devices.iFWPD_NSegments[channel]);
		BlockDataInfo.timestamps_position[channel] = timestamp_start;
		BlockDataInfo.data_position[channel] = start_data;
		timestamp_start += CurrentBlockInfo->sp_devices.iFWPD_NSegments[channel];
	}
	else BlockDataInfo.nsegments[channel] = 0;

	unsigned int records_accum[MAX_SP_CHANNELS] = { 0 };
	for (int s = 0; s < CurrentBlockInfo->nsegments; ++s)
	{
		const int channel = headers[s].channel;
		const unsigned int nrecord = records_accum[channel];
		++records_accum[channel];
		const unsigned int segsize = headers[s].nsamples * sizeof(short);

		BlockDataInfo.segments_position[channel][nrecord] = start_data + sizeof(short)* headers[s].position;
		BlockDataInfo.nsamples[channel][nrecord] = headers[s].nsamples;
		Timestamps[BlockDataInfo.timestamps_position[channel] + nrecord] = headers[s].timestamp + headers[s].recordstart;
	}
}

bool SP_Devices_DataRead_Class_v2::GetIteration(const unsigned int niteration, const unsigned int nblock)
{
	// 1. Check the file set is defined
	if (!IsFileSetInitialised) return false;

	// 2. Get the file storing iteration and block
	const unsigned int nfiles = FileSet.size();
	unsigned int nfile = -1; // File containing the requested block
	unsigned int ntblocks = 0;
	unsigned int rblock = 0; // Real block number in file of the requested block
	for (int nf = 0; nf < nfiles; ++nf) for (int nb = 0; nb < FileSet.at(nf).info_block.size(); ++nb)
	{
		if (FileSet.at(nf).info_block.at(nb).niteration == niteration && FileSet.at(nf).info_block.at(nb).nblock == nblock)
		{
			nfile = nf;
			break;
		}
		++rblock;
	}
	if (nfile == -1) return false; // The requested block does not exist

	return GetIteration(rblock);
}
const short *SP_Devices_DataRead_Class_v2::GetChannel(const unsigned int channel, unsigned int &nsegments, unsigned int *segsize)
{
	// This is a general function that can be used only in the FWDAQ case
	if (CurrentBlockInfo->sp_devices.b_channel[channel] == false) return NULL;
	if (CurrentBlockInfo->sp_devices.iFWDAQ_NSegments == 0) return NULL;

	nsegments = BlockDataInfo.nsegments[channel];
	if (segsize != NULL) memcpy(segsize, BlockDataInfo.nsamples[channel].data(), nsegments * sizeof(unsigned int));

	return (const short *)&FullIteration[BlockDataInfo.data_position[channel]];
}
unsigned int SP_Devices_DataRead_Class_v2::GetNSegments(const unsigned int channel) const
{
	return BlockDataInfo.nsegments[channel];
}
const short *SP_Devices_DataRead_Class_v2::GetSegment(const unsigned int channel, const unsigned int nsegment, unsigned int &segsize)
{
	if (BlockDataInfo.nsegments[channel] <= nsegment) return NULL;
	segsize = BlockDataInfo.nsamples[channel].at(nsegment);
	return (short *)&FullIteration[BlockDataInfo.segments_position[channel].at(nsegment)];
}
const void *SP_Devices_DataRead_Class_v2::GetSetSegment(const int channel, const unsigned int nsegment, unsigned int &nsamples, long long &timestamp)
{
	// Check and load iteration if required
	if (CurrentBlockInfo->nsetsegment[channel] < nsegment && CurrentBlockInfo->nsetsegment[channel] + BlockDataInfo.nsegments[channel] <= nsegment)
	{
		// Look iteration and load
		// 2. Get the file storing iteration and block
		const unsigned int nfiles = FileSet.size();
		unsigned int ntblocks = 0;
		unsigned int rblock = -1; // Real block number in file of the requested block
		for (int nf = 0; nf < nfiles; ++nf) for (int nb = 0; nb < FileSet.at(nf).info_block.size(); ++nb)
		{
			SP_Devices_BlockInfo_Struct_v2 &_block = FileSet.at(nf).info_block.at(nb);
			if (_block.nsetsegment[channel] <= nsegment)	++rblock;
			else break;
		}
		if (rblock == -1) return NULL; // The requested block does not exist

		if (!GetIteration(rblock)) return NULL;
	}
	unsigned int _segment = nsegment - CurrentBlockInfo->nsetsegment[channel];
	nsamples = BlockDataInfo.nsamples[channel].at(_segment);
	timestamp = Timestamps[_segment];
	return (short *)&FullIteration[BlockDataInfo.segments_position[channel].at(_segment)];
}
const long long *SP_Devices_DataRead_Class_v2::GetTimestamps(const unsigned int channel)
{
	if (CurrentBlockInfo->sp_devices.b_channel[channel] == false) return NULL;
	return &Timestamps[BlockDataInfo.timestamps_position[channel]];
}

bool SP_Devices_DataRead_Class_v2::IsVariableSize(const unsigned int channel) const
{
	return CurrentBlockInfo->sp_devices.bFWPD_VariableLength;
}
double SP_Devices_DataRead_Class_v2::GetFullVerticalScale(const unsigned int channel) const
{
	return CurrentBlockInfo->sp_devices.FullVerticalScale[channel];
}
int SP_Devices_DataRead_Class_v2::GetNPreSamples(const unsigned int channel) const
{
	const double ns_to_samples = CurrentBlockInfo->sp_devices.i64Frequency / 1000000000.;
	return CurrentBlockInfo->sp_devices.iFWDAQ_NSegments == 0? -(CurrentBlockInfo->sp_devices.iFWPD_LEW[channel]*ns_to_samples) : (CurrentBlockInfo->sp_devices.iFWDAQ_Delay*ns_to_samples);
}
double SP_Devices_DataRead_Class_v2::GetVerticalOffset(const unsigned int channel) const
{
	return  CurrentBlockInfo->sp_devices.iOffset[channel];
}
double SP_Devices_DataRead_Class_v2::RawTomV(const unsigned int channel) const
{
	return CurrentBlockInfo->sp_devices.FullVerticalScale[channel] / 65536;
}
double SP_Devices_DataRead_Class_v2::SampleTons() const
{
	return 1000000000. / CurrentBlockInfo->sp_devices.i64Frequency;
}

bool SP_Devices_DataRead_Class_v2::isSPLabelCorrect(const char *label, const char *filename) const
{
	if (strcmp(label, "SP_DEVICES_HEADER_v2.0"))
	{
#ifdef QT_WIDGETS_LIB
		QWidget *messagewarning = 0;
		QMessageBox::warning(messagewarning, QString("DAISY"), QString("File %1 has not SP_DEVICES_HEADER_v2.0 format\n").arg(filename));
#else
		printf("File %s has not SP_DEVICES_HEADER_v2.0 format\n", filename);
#endif
		return false;
	}
	return true;
};
bool SP_Devices_DataRead_Class_v2::IsChannelActive(const unsigned int channel) const
{
	return CurrentBlockInfo->sp_devices.b_channel[channel];
}
int SP_Devices_DataRead_Class_v2::GetCardId() const
{
	if (IsFileSetInitialised) return FileSet.at(0).cardid;

	return -1;
}

unsigned int SP_Devices_DataRead_Class_v2::get_nchannels()
{
	unsigned int nch = 0;
	for (int ch = 0; ch < MAX_SP_CHANNELS; ++ch) if (CurrentBlockInfo->sp_devices.b_channel[ch]) ++nch;
	return nch;
}
long long SP_Devices_DataRead_Class_v2::GetMaxSetSegments(const unsigned int channel) const
{
	const unsigned int nfile = FileSet.size() - 1;
	const unsigned int nblock = FileSet.at(nfile).info_block.size() - 1;
	const SP_Devices_BlockInfo_Struct_v2 &_block = FileSet.at(nfile).info_block.at(nblock);

	return _block.nsetsegment[channel] + _block.nsegments;
}

long long SP_Devices_DataRead_Class_v2::GetLiveTime(const unsigned int channel, long long &measurement_time)
{
	// Get number of iterations
	const unsigned int nblocks = GetNIterations();

	GetIteration(0);
	const long long initial_time = Timestamps[0];
	long long time_last_iteration = initial_time;
	long long time_current_iteration_ini = initial_time;
	long long time_current_iteration_last = Timestamps[CurrentBlockInfo->nsegments - 1];
	unsigned int last_iteration = CurrentBlockInfo->niteration;
	long long live_time = 0;

	if (channel == 0) printf("Number of blocks to read: %d", (int)nblocks);
	// Loop over all blocks
	for (unsigned int block = 1; block < nblocks; ++block)
	{
		GetIteration(block);
		// Check if we have changed iteration
		if (last_iteration == CurrentBlockInfo->niteration)
		{
			// Only update time_current_iteration_last
			time_current_iteration_last = Timestamps[CurrentBlockInfo->nsegments - 1];
		}
		else
		{
			// Update the new iteration
			last_iteration = CurrentBlockInfo->niteration;
			// Add the live time
			live_time += time_current_iteration_last - time_current_iteration_ini;
			time_current_iteration_ini = Timestamps[0];
			time_current_iteration_last = Timestamps[CurrentBlockInfo->nsegments - 1];
		}
	}
	live_time += time_current_iteration_last - time_current_iteration_ini;
	const long long final_time = time_current_iteration_last;

	measurement_time = final_time - initial_time;

	return live_time;
}
