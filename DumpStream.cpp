#include "StdAfx.h"
#include "PayloadBuf.h"
#include <memory.h>
#include <string.h>
#include "Bitstream.h"
#include "DumpTS.h"
#include "AudChMap.h"

using namespace std;

extern const char *dump_msg[];
extern unordered_map<std::string, std::string> g_params;
extern TS_FORMAT_INFO g_ts_fmtinfo;
extern int g_verbose_level;
extern DUMP_STATUS g_dump_status;

// For MLP audio
#define FBB_SYNC_CODE				0xF8726FBB
#define FBA_SYNC_CODE				0xF8726FBA
#define MLP_SIGNATURE				0xB752

#define AC3_SYNCWORD				0x0B77
#define EAC3_SYNCWORD				0x0B77

#define DTS_SYNCWORD_CORE			0x7ffe8001
#define DTS_SYNCWORD_XCH			0x5a5a5a5a
#define DTS_SYNCWORD_XXCH			0x47004a03
#define DTS_SYNCWORD_X96			0x1d95f262
#define DTS_SYNCWORD_XBR			0x655e315e
#define DTS_SYNCWORD_LBR			0x0a801921
#define DTS_SYNCWORD_XLL			0x41a29547
#define DTS_SYNCWORD_SUBSTREAM		0x64582025
#define DTS_SYNCWORD_SUBSTREAM_CORE 0x02b09261

struct AUDIO_INFO
{
	unsigned long	sample_frequency;
	unsigned long	channel_mapping;	// The channel assignment according to bit position defined in CHANNEL_MAP_LOC
	unsigned long	bits_per_sample;
	unsigned long	bitrate;			// bits/second
};

struct VIDEO_INFO
{
	unsigned char	EOTF;				// HDR10, SDR
	unsigned char	colorspace;			// REC.601, BT.709 and BT.2020
	unsigned char	colorfmt;			// YUV 4:2:0, 4:2:2 or 4:4:4
	unsigned char	reserved;
	unsigned long	video_height;
	unsigned long	video_width;
	unsigned short	framerate_numerator;
	unsigned short	framerate_denominator;
	unsigned short	aspect_ratio_numerator;
	unsigned short	aspect_ratio_denominator;
};

struct STREAM_INFO
{
	int	stream_coding_type;

	union
	{
		AUDIO_INFO	audio_info;
		VIDEO_INFO	video_info;
	};
};

using PID_StramInfos = unordered_map<unsigned short, std::vector<STREAM_INFO>>;
using DDP_Program_StreamInfos = unordered_map<unsigned char /* Program */, std::vector<STREAM_INFO>>;

PID_StramInfos g_stream_infos;
DDP_Program_StreamInfos g_ddp_program_stream_infos;
unsigned char g_cur_ddp_program_id = 0XFF;
std::vector<STREAM_INFO> g_cur_dtshd_stream_infos;

int ParseAC3Frame(unsigned short PID, int stream_type, unsigned char* pBuf, int cbSize, STREAM_INFO& audio_info)
{
	CBitstream bst(pBuf, cbSize << 3);
	bst.SkipBits(32);

	uint8_t fscod = (uint8_t)bst.GetBits(2);
	uint8_t frmsizecod = (uint8_t)bst.GetBits(6);

	uint8_t bsid = (uint8_t)bst.GetBits(5);

	if (bsid != 0x08)
		return -1;

	uint8_t bsmod = (uint8_t)bst.GetBits(3);
	uint8_t acmod = (uint8_t)bst.GetBits(3);

	uint8_t cmixlev = 0;
	uint8_t surmixlev = 0;
	uint8_t dsurmod = 0;
	if ((acmod & 0x1) && (acmod != 1))
		cmixlev = (uint8_t)bst.GetBits(2);

	if ((acmod & 0x4))
		surmixlev = (uint8_t)bst.GetBits(2);

	if (acmod == 0x2)
		dsurmod = (uint8_t)bst.GetBits(2);

	uint8_t lfeon = (uint8_t)bst.GetBits(1);

	audio_info.audio_info.bits_per_sample = 16;
	audio_info.audio_info.sample_frequency = fscod == 0 ? 48000 : (fscod == 1 ? 44100 : (fscod == 2 ? 32000 : 0));
	audio_info.audio_info.channel_mapping = acmod_ch_assignments[acmod];

	if (lfeon)
		audio_info.audio_info.channel_mapping |= CHANNEL_BITMASK(CH_LOC_LFE);

	audio_info.stream_coding_type = stream_type;

	return 0;
}

int ParseEAC3Frame(unsigned short PID, int stream_type, unsigned char* pBuf, int cbSize, STREAM_INFO& audio_info, unsigned char& ddp_progid)
{
	int iRet = -1;
	CBitstream bst(pBuf, cbSize << 3);
	bst.SkipBits(16);

	uint32_t byte2to5 = (uint32_t)bst.PeekBits(32);
	uint8_t bsid = (byte2to5 >> 3) & 0x1F;

	if (bsid != 0x8 && bsid != 0x10)
		return -1;

	if (bsid == 0x8 || bsid == 0x10 && ((byte2to5 >> 30) == 0 || (byte2to5 >> 30) == 2))
	{
		unsigned char program_id = 0;
		if (bsid == 0x08)
			program_id = 0;
		else
		{
			uint8_t substreamid = (byte2to5 >> 27) & 0x7;
			program_id = substreamid;
		}

		if (g_ddp_program_stream_infos.size() > 0 && 
			g_ddp_program_stream_infos.find(program_id) != g_ddp_program_stream_infos.end() &&
			g_ddp_program_stream_infos[program_id].size() > 0)
		{
			auto ddp_prog_stminfo = &g_ddp_program_stream_infos[program_id][0];

			// Current it is an AC3 program
			audio_info.stream_coding_type = ddp_prog_stminfo->stream_coding_type;
			audio_info.audio_info.bits_per_sample = 16;
			audio_info.audio_info.sample_frequency = ddp_prog_stminfo->audio_info.sample_frequency;

			unsigned long ddp_channel_mapping = 0;
			for (size_t i = 0; i < g_ddp_program_stream_infos[program_id].size(); i++)
			{
				ddp_channel_mapping |= g_ddp_program_stream_infos[program_id][i].audio_info.channel_mapping;
			}
				
			audio_info.audio_info.channel_mapping = ddp_prog_stminfo->audio_info.channel_mapping = ddp_channel_mapping;

			g_ddp_program_stream_infos[program_id].clear();
			ddp_progid = program_id;
			iRet = 0;
		}
	}

	if (bsid == 0x8)	// AC3
	{
		STREAM_INFO ac3_info;
		if (ParseAC3Frame(PID, stream_type, pBuf, cbSize, ac3_info) < 0)
		{
			// Clear the current information
			g_ddp_program_stream_infos[0].clear();
			goto done;
		}

		g_cur_ddp_program_id = 0;
		g_ddp_program_stream_infos[0].resize(1);
		g_ddp_program_stream_infos[0][0] = ac3_info;
	}
	else if (bsid == 0x10)	// EAC3
	{
		uint8_t strmtyp = (uint8_t)bst.GetBits(2);
		uint8_t substreamid = (uint8_t)bst.GetBits(3);

		uint16_t frmsiz = (uint16_t)bst.GetBits(11);

		uint8_t fscod = (uint8_t)bst.GetBits(2);
		uint8_t fscod2 = 0xFF;
		uint8_t numblkscod = 0x3;
		if (fscod == 0x3)
			fscod2 = (uint8_t)bst.GetBits(2);
		else
			numblkscod = (uint8_t)bst.GetBits(2);

		uint8_t acmod = (uint8_t)bst.GetBits(3);
		uint8_t lfeon = (uint8_t)bst.GetBits(1);

		uint8_t bsid = (uint8_t)bst.GetBits(5);
		uint8_t dialnorm = (uint8_t)bst.GetBits(5);
		uint8_t compre = (uint8_t)bst.GetBits(1);
		if (compre)
			bst.SkipBits(8);

		if (acmod == 0x0)
		{
			bst.SkipBits(5);
			uint8_t compr2e = (uint8_t)bst.GetBits(1);
			if (compr2e)
				bst.SkipBits(8);
		}

		uint16_t chanmap = 0;
		uint8_t chanmape = 0;
		if (strmtyp == 0x01)
		{
			chanmape = (uint8_t)bst.GetBits(1);
			if (chanmape)
				chanmap = (uint16_t)bst.GetBits(16);
		}

		auto getfs = [](uint8_t fscod, uint8_t fscod2) {
			return fscod == 0 ? 48000 : (fscod == 1 ? 44100 : (fscod == 2 ? 32000 : (fscod == 3 ? (
				fscod2 == 0 ? 24000 : (fscod2 == 1 ? 22050 : (fscod2 == 2 ? 16000 : 0))) : 0)));
		};

		if (strmtyp == 0 || strmtyp == 2)
		{
			g_cur_ddp_program_id = substreamid;
			g_ddp_program_stream_infos[substreamid].resize(1);

			// analyze the audio info.
			auto ptr_stm_info = &g_ddp_program_stream_infos[substreamid][0];
			ptr_stm_info->stream_coding_type = stream_type;
			ptr_stm_info->audio_info.bits_per_sample = 16;
			ptr_stm_info->audio_info.sample_frequency = getfs(fscod, fscod2);
			ptr_stm_info->audio_info.channel_mapping = acmod_ch_assignments[acmod] |
				(lfeon ? CHANNEL_BITMASK(CH_LOC_LFE) : 0);
		}
		else if (strmtyp == 1 && g_cur_ddp_program_id >= 0 && g_cur_ddp_program_id <= 7)
		{
			if (g_ddp_program_stream_infos[g_cur_ddp_program_id].size() != substreamid + 1)
			{
				g_ddp_program_stream_infos[g_cur_ddp_program_id].clear();
				goto done;
			}

			g_ddp_program_stream_infos[g_cur_ddp_program_id].resize(substreamid + 2);

			// analyze the audio info
			auto ptr_stm_info = &g_ddp_program_stream_infos[g_cur_ddp_program_id][substreamid + 1];
			ptr_stm_info->stream_coding_type = stream_type;
			ptr_stm_info->audio_info.bits_per_sample = 16;
			ptr_stm_info->audio_info.sample_frequency = getfs(fscod, fscod2);

			if (chanmape)
			{
				unsigned long ddp_channel_mapping = 0;
				for (int i = 0; i < _countof(ddp_ch_assignment); i++)
					if ((1 << (15-i))&chanmap)
						ddp_channel_mapping |= ddp_ch_assignment[i];
				ptr_stm_info->audio_info.channel_mapping = ddp_channel_mapping;
			}
			else
				ptr_stm_info->audio_info.channel_mapping = acmod_ch_assignments[acmod] |
					(lfeon? CHANNEL_BITMASK(CH_LOC_LFE):0);
		}
		else
		{
			// Nothing to do
			goto done;
		}
	}

done:
	return iRet;
}

int ParseMLPAU(unsigned short PID, int stream_type, unsigned long sync_code, unsigned char* pBuf, int cbSize, STREAM_INFO& audio_info)
{
	if (sync_code != FBA_SYNC_CODE && sync_code != FBB_SYNC_CODE)
		return -1;

	unsigned char check_nibble = (pBuf[0] & 0xF0) >> 4;
	unsigned short access_unit_length = ((pBuf[0] << 8) | pBuf[1]) & 0xFFF;

	audio_info.stream_coding_type = stream_type;

	unsigned char audio_sampling_frequency;
	if (sync_code == FBA_SYNC_CODE)
	{
		audio_info.audio_info.bits_per_sample = 24;

		audio_sampling_frequency = (pBuf[8] >> 4) & 0xF;

		unsigned char channel_assignment_6ch_presentation = ((pBuf[9] & 0x0F) << 1) | ((pBuf[10] >> 7) & 0x01);
		unsigned char channel_assignment_8ch_presentation = ((pBuf[10] & 0x1F) << 8) | pBuf[11];

		audio_info.audio_info.channel_mapping = 0;
		unsigned short flags = (pBuf[14] << 8) | pBuf[15];
		if ((flags & 0x8000))
		{
			for (int i = 0; i < _countof(FBA_Channel_Loc_mapping_1); i++)
				if (channel_assignment_8ch_presentation&&(1<<i))
					audio_info.audio_info.channel_mapping |= FBA_Channel_Loc_mapping_1[i];
		}
		else
		{
			for (int i = 0; i < _countof(FBA_Channel_Loc_mapping_0); i++)
				if (channel_assignment_8ch_presentation && (1 << i))
					audio_info.audio_info.channel_mapping |= FBA_Channel_Loc_mapping_0[i];
		}
	}
	else if (sync_code == FBB_SYNC_CODE)
	{
		unsigned char quantization_word_length_1 = (pBuf[8] >> 4);
		unsigned char audio_sampling_frequency_1 = (pBuf[9] >> 4) & 0xF;
		audio_sampling_frequency = audio_sampling_frequency_1;

		audio_info.audio_info.channel_mapping = 0;
		
		unsigned char channel_assignment = pBuf[11] & 0x1F;
		if (channel_assignment < sizeof(FBB_Channel_assignments) / sizeof(FBB_Channel_assignments[0]))
			audio_info.audio_info.channel_mapping = FBB_Channel_assignments[channel_assignment];
		else
			audio_info.audio_info.channel_mapping = 0;	// Not support
		
		audio_info.audio_info.bits_per_sample = quantization_word_length_1 == 0 ? 16 : (quantization_word_length_1 == 1 ? 20 : (quantization_word_length_1 == 2 ? 24 : 0));
	}

	switch (audio_sampling_frequency)
	{
	case 0x0: audio_info.audio_info.sample_frequency = 48000; break;
	case 0x1: audio_info.audio_info.sample_frequency = 96000; break;
	case 0x2: audio_info.audio_info.sample_frequency = 192000; break;
	case 0x8: audio_info.audio_info.sample_frequency = 44100; break;
	case 0x9: audio_info.audio_info.sample_frequency = 88200; break;
	case 0xA: audio_info.audio_info.sample_frequency = 176400; break;
	default:
		audio_info.audio_info.sample_frequency = (unsigned long)-1;
	}

	return 0;
}

int ParseCoreDTSAU(unsigned short PID, int stream_type, unsigned long sync_code, unsigned char* pBuf, int cbSize, STREAM_INFO& audio_info)
{
	int iRet = -1;
	CBitstream bst(pBuf, cbSize << 3);


	try
	{
		bst.SkipBits(32);
		uint8_t FTYPE = (uint8_t)bst.GetBits(1);

		if (FTYPE == 0)
			return -1;

		uint8_t SHORT = (uint8_t)bst.GetBits(5);
		uint8_t CPF = (uint8_t)bst.GetBits(1);
		uint8_t NBLKS = (uint8_t)bst.GetBits(7);
		if (NBLKS <= 4)
			return -1;

		uint16_t FSIZE = (uint16_t)bst.GetBits(14);
		if (FSIZE <= 94)
			return -1;

		uint8_t AMODE = (uint8_t)bst.GetBits(6);
		uint8_t SFREQ = (uint8_t)bst.GetBits(4);
		uint8_t RATE = (uint8_t)bst.GetBits(5);
		uint8_t FixedBit = (uint8_t)bst.GetBits(1);

		if (FixedBit != 0)
			return -1;

		uint8_t DYNF = (uint8_t)bst.GetBits(1);
		uint8_t TIMEF = (uint8_t)bst.GetBits(1);
		uint8_t AUXF = (uint8_t)bst.GetBits(1);
		uint8_t HDCD = (uint8_t)bst.GetBits(1);
		uint8_t EXT_AUDIO_ID = (uint8_t)bst.GetBits(3);
		uint8_t EXT_AUDIO = (uint8_t)bst.GetBits(1);
		uint8_t ASPF = (uint8_t)bst.GetBits(1);
		uint8_t LFF = (uint8_t)bst.GetBits(2);
		uint8_t HFLAG = (uint8_t)bst.GetBits(1);
		uint16_t HCRC;
		if (CPF == 1) // Present
			HCRC = (uint16_t)bst.GetBits(16);
		uint8_t FILTS = (uint8_t)bst.GetBits(1);
		uint8_t VERNUM = (uint8_t)bst.GetBits(4);
		uint8_t CHIST = (uint8_t)bst.GetBits(2);
		uint8_t PCMR = (uint8_t)bst.GetBits(3);
		uint8_t SUMF = (uint8_t)bst.GetBits(1);
		uint8_t SUMS = (uint8_t)bst.GetBits(1);

		static uint32_t SFREQs[] = {
			0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0 };

		static uint32_t RATEs[] = {
			32000, 56000, 64000, 96000, 112000, 128000, 192000, 224000, 256000, 320000, 384000, 448000, 512000, 576000, 640000,
			768000, 960000, 1024000, 1152000, 1280000, 1344000, 1408000, 1411200, 1472000, 1536000, 1920000, 2048000, 3072000, 3840000,
			(unsigned long)-3,	// Open
			(unsigned long)-2,	// Variant
			(unsigned long)-1	// Lossless
		};
		static uint8_t bpses[] = {
			16, 16, 20, 20, 24, 24, 0, 0
		};

		// Organize the information.
		audio_info.stream_coding_type = stream_type;
		audio_info.audio_info.channel_mapping = 0;
		for (size_t i = 0; AMODE < _countof(dts_audio_channel_arragements) && i < dts_audio_channel_arragements[AMODE].size(); i++)
			audio_info.audio_info.channel_mapping |= CHANNEL_BITMASK(dts_audio_channel_arragements[AMODE][i]);

		if (LFF)
			audio_info.audio_info.channel_mapping |= CHANNEL_BITMASK(CH_LOC_LFE);

		audio_info.audio_info.sample_frequency = SFREQs[SFREQ];
		audio_info.audio_info.bits_per_sample = bpses[PCMR];
		audio_info.audio_info.bitrate = RATEs[RATE];
	}
	catch (...)
	{
		return -1;
	}

	return 0;
}

enum DTS_EXTSUBSTREAM
{

	DTS_CORESUBSTREAM_CORE = 0x00000001,
	DTS_BCCORE_XXCH = 0x00000002,
	DTS_BCCORE_X96 = 0x00000004,
	DTS_BCCORE_XCH = 0x00000008,
	DTS_EXSUBSTREAM_CORE = 0x00000010,
	DTS_EXSUBSTREAM_XBR = 0x00000020,
	DTS_EXSUBSTREAM_XXCH = 0x00000040,
	DTS_EXSUBSTREAM_X96 = 0x00000080,
	DTS_EXSUBSTREAM_LBR = 0x00000100,
	DTS_EXSUBSTREAM_XLL = 0x00000200,
	DTS_EXSUBSTREAM_AUX1 = 0x00000400,
	DTS_EXSUBSTREAM_AUX2 = 0x00000800,
};

int ParseDTSExSSAU(unsigned short PID, int stream_type, unsigned long sync_code, unsigned char* pBuf, int cbSize, STREAM_INFO& audio_info)
{
	auto NumSpkrTableLookUp = [&](uint32_t nSpkrMask)
	{
		unsigned int i, nTotalChs;

		if (nSpkrMask == 0)
			return 0;

		nTotalChs = 0;
		for (i = 0; i < _countof(dtshd_speaker_bitmask_table); i++)
		{
			if ((1 << i)& nSpkrMask)
				nTotalChs += std::get<3>(dtshd_speaker_bitmask_table[i]);
		}
		return (int)nTotalChs;
	};

	auto CountBitsSet_to_1 = [](uint8_t bitcount, uint32_t val)
	{
		uint8_t nRet = 0;
		for (uint8_t i = 0; i < bitcount; i++)
		{
			if (val & (1 << i))
				nRet++;
		}
		return nRet;
	};

	CBitstream bst(pBuf, cbSize << 3);
	bst.SkipBits(32);

	uint8_t UserDefinedBits = (uint8_t)bst.GetBits(8);
	uint8_t nExtSSIndex = (uint8_t)bst.GetBits(2);

	uint8_t	bHeaderSizeType = (uint8_t)bst.GetBits(1);
	uint8_t nuBits4Header = bHeaderSizeType ? 12 : 8;
	uint8_t nuBits4ExSSFsize = bHeaderSizeType ? 20 : 16;

	uint32_t nuExtSSHeaderSize, nuExtSSFsize;
	bst.GetBits(nuBits4Header, nuExtSSHeaderSize);
	bst.GetBits(nuBits4ExSSFsize, nuExtSSFsize);
	nuExtSSHeaderSize++; nuExtSSFsize++;

	uint8_t bStaticFieldsPresent;
	bst.GetBits(1, bStaticFieldsPresent);

	STREAM_INFO stm_info;
	memset(&stm_info, 0, sizeof(stm_info));

	stm_info.stream_coding_type = stream_type;

	uint8_t nuNumAudioPresnt = 1, nuNumAssets = 1, bMixMetadataEnbl = 0;
	uint8_t nuNumMixOutConfigs = 0;
	uint8_t nuMixOutChMask[4];
	uint8_t nNumMixOutCh[4];
	uint16_t nuExSSFrameDurationCode;
	if (bStaticFieldsPresent)
	{
		uint8_t nuRefClockCode, bTimeStampFlag;

		bst.GetBits(2, nuRefClockCode);
		bst.GetBits(3, nuExSSFrameDurationCode);
		nuExSSFrameDurationCode = 512 * (nuExSSFrameDurationCode + 1);
		bst.GetBits(1, bTimeStampFlag);

		if (bTimeStampFlag)
			bst.SkipBits(36);

		bst.GetBits(3, nuNumAudioPresnt); nuNumAudioPresnt++;
		bst.GetBits(3, nuNumAssets); nuNumAssets++;

		uint8_t nuActiveExSSMask[8];
		uint8_t nuActiveAssetMask[8][4];
		for (uint8_t nAuPr = 0; nAuPr < nuNumAudioPresnt; nAuPr++)
			bst.GetBits(nExtSSIndex + 1, nuActiveExSSMask[nAuPr]);

		for (uint8_t nAuPr = 0; nAuPr < nuNumAudioPresnt; nAuPr++) {
			for (uint8_t nSS = 0; nSS < nExtSSIndex + 1; nSS++) {
				if (((nuActiveExSSMask[nAuPr] >> nSS) & 0x1) == 1)
					nuActiveAssetMask[nAuPr][nSS] = (uint8_t)bst.GetBits(8);
				else
					nuActiveAssetMask[nAuPr][nSS] = 0;
			}
		}

		bMixMetadataEnbl = (uint8_t)bst.GetBits(1);
		if (bMixMetadataEnbl)
		{
			uint8_t nuMixMetadataAdjLevel = (uint8_t)bst.GetBits(2);
			uint8_t nuBits4MixOutMask = (uint8_t)(bst.GetBits(2) + 1) << 2;
			nuNumMixOutConfigs = (uint8_t)bst.GetBits(2) + 1;
			// Output Mixing Configuration Loop
			for (uint8_t ns = 0; ns < nuNumMixOutConfigs; ns++) {
				nuMixOutChMask[ns] = (uint8_t)bst.GetBits(nuBits4MixOutMask);
				nNumMixOutCh[ns] = NumSpkrTableLookUp(nuMixOutChMask[ns]);
			}
		}
	}
	else
		return -1;	// Can't get audio information if bStaticFieldPresent is false;

	uint32_t nuAssetFsize[8];
	for (int nAst = 0; nAst < nuNumAssets; nAst++)
		nuAssetFsize[nAst] = (uint16_t)bst.GetBits(nuBits4ExSSFsize) + 1;

	for (int nAst = 0; nAst < nuNumAssets; nAst++)
	{
		uint8_t nuAssetIndex;
		uint16_t nuAssetDescriptFsize;
		uint8_t nuBitResolution = 0, nuMaxSampleRate = 0, nuTotalNumChs = 0, bOne2OneMapChannels2Speakers = 0;

		uint32_t asset_start_bitpos = bst.Tell();

		bst.GetBits(9, nuAssetDescriptFsize); nuAssetDescriptFsize++;
		bst.GetBits(3, nuAssetIndex);

		uint8_t bEmbeddedStereoFlag = 0, bEmbeddedSixChFlag = 0, nuRepresentationType = 0;
		uint8_t nuMainAudioScaleCode[4][32];

		if (bStaticFieldsPresent)
		{
			uint8_t bAssetTypeDescrPresent = (uint8_t)bst.GetBits(1);
			uint8_t nuAssetTypeDescriptor = 0;
			if (bAssetTypeDescrPresent)
				nuAssetTypeDescriptor = (uint8_t)bst.GetBits(4);
			uint8_t bLanguageDescrPresent = (uint8_t)bst.GetBits(1);
			uint32_t LanguageDescriptor;
			if (bLanguageDescrPresent)
				LanguageDescriptor = (uint32_t)bst.GetBits(24);
			uint8_t bInfoTextPresent = (uint8_t)bst.GetBits(1);
			uint16_t nuInfoTextByteSize;
			if (bInfoTextPresent)
				nuInfoTextByteSize = (uint16_t)bst.GetBits(10) + 1;

			if (bInfoTextPresent)
				bst.SkipBits(nuInfoTextByteSize * 8);
			nuBitResolution = (uint8_t)bst.GetBits(5) + 1;
			nuMaxSampleRate = (uint8_t)bst.GetBits(4);
			nuTotalNumChs = (uint8_t)bst.GetBits(8) + 1;
			bOne2OneMapChannels2Speakers = (uint8_t)bst.GetBits(1);

			uint32_t nuSpkrActivityMask = 0;
			if (bOne2OneMapChannels2Speakers)
			{
				uint8_t bSpkrMaskEnabled = 0, nuNumBits4SAMask = 0;
				if (nuTotalNumChs>2)
					bEmbeddedStereoFlag = (uint8_t)bst.GetBits(1);

				if (nuTotalNumChs>6)
					bEmbeddedSixChFlag = (uint8_t)bst.GetBits(1);

				bSpkrMaskEnabled = (uint8_t)bst.GetBits(1);
				if (bSpkrMaskEnabled)
					nuNumBits4SAMask = (uint8_t)(bst.GetBits(2) + 1) << 2;

				if (bSpkrMaskEnabled)
					nuSpkrActivityMask = (uint32_t)bst.GetBits(nuNumBits4SAMask);
				uint8_t nuNumSpkrRemapSets = (uint8_t)bst.GetBits(3);
				uint32_t nuStndrSpkrLayoutMask[8];
				uint8_t nuNumDecCh4Remap[8];
				uint32_t nuRemapDecChMask[8][32];
				uint8_t nuSpkrRemapCodes[8][32][32];
				for (uint8_t ns = 0; ns<nuNumSpkrRemapSets; ns++)
					nuStndrSpkrLayoutMask[ns] = (uint8_t)bst.GetBits(nuNumBits4SAMask);
				for (uint8_t ns = 0; ns<nuNumSpkrRemapSets; ns++) {
					uint8_t nuNumSpeakers = NumSpkrTableLookUp(nuStndrSpkrLayoutMask[ns]);
					nuNumDecCh4Remap[ns] = (uint8_t)bst.GetBits(5) + 1;
					for (int nCh = 0; nCh<nuNumSpeakers; nCh++) { // Output channel loop
						nuRemapDecChMask[ns][nCh] = (uint32_t)bst.GetBits(nuNumDecCh4Remap[ns]);
						uint8_t nCoefs = CountBitsSet_to_1(nuNumDecCh4Remap[ns], nuRemapDecChMask[ns][nCh]);
						for (uint8_t nc = 0; nc<nCoefs; nc++)
							nuSpkrRemapCodes[ns][nCh][nc] = (uint8_t)bst.GetBits(5);
					} // End output channel loop
				} // End nuNumSpkrRemapSets loop
			}
			else
			{
				// No speaker feed case
				bEmbeddedStereoFlag = 0;
				bEmbeddedSixChFlag = 0;
				nuRepresentationType = (uint8_t)bst.GetBits(3);
			}


			//
			// Update stm_info of current audio asset
			// It is not so accurate, later we may get the audio information order by audio present defined in each extension stream header
			//
			if (stm_info.audio_info.bits_per_sample < nuBitResolution)
				stm_info.audio_info.bits_per_sample = nuBitResolution;

			static uint32_t ExSS_Asset_Max_SampleRates[] = {
				8000,16000,32000,64000,128000,
				22050,44100,88200,176400,352800,
				12000,24000,48000,96000,192000,384000
			};

			if (stm_info.audio_info.sample_frequency < ExSS_Asset_Max_SampleRates[nuMaxSampleRate])
				stm_info.audio_info.sample_frequency = ExSS_Asset_Max_SampleRates[nuMaxSampleRate];

			// Update the channel information
			if (bOne2OneMapChannels2Speakers)
			{
				for (int i = 0; i < _countof(dtshd_speaker_bitmask_table); i++)
				{
					if ((1<<i)&nuSpkrActivityMask)
						stm_info.audio_info.channel_mapping |= std::get<2>(dtshd_speaker_bitmask_table[i]);
				}
			}
			else
			{
				// Can't process it at present.
				// TODO...
			}
		} // End of if (bStaticFieldsPresent)

		uint32_t asset_cur_bitpos = bst.Tell();

		uint8_t nuDRCCode, nuDialNormCode, nuDRC2ChDmixCode, bMixMetadataPresent;
		uint8_t bDRCCoefPresent = (uint8_t)bst.GetBits(1);
		if (bDRCCoefPresent)
			nuDRCCode = (uint8_t)bst.GetBits(8);
		uint8_t bDialNormPresent = (uint8_t)bst.GetBits(1);
		if (bDialNormPresent)
			nuDialNormCode = (uint8_t)bst.GetBits(5);
		if (bDRCCoefPresent && bEmbeddedStereoFlag)
			nuDRC2ChDmixCode = (uint8_t)bst.GetBits(8);
		if (bMixMetadataEnbl)
			bMixMetadataPresent = (uint8_t)bst.GetBits(1);
		else
			bMixMetadataPresent = 0;

		if (bMixMetadataPresent) {
			uint8_t bExternalMixFlag = (uint8_t)bst.GetBits(1);
			uint8_t nuPostMixGainAdjCode = (uint8_t)bst.GetBits(6);
			uint8_t nuControlMixerDRC = (uint8_t)bst.GetBits(2);
			if (nuControlMixerDRC <3)
				uint8_t nuLimit4EmbeddedDRC = (uint8_t)bst.GetBits(3);
			if (nuControlMixerDRC == 3)
				uint8_t nuCustomDRCCode = (uint8_t)bst.GetBits(8);
			uint8_t bEnblPerChMainAudioScale = (uint8_t)bst.GetBits(1);
			for (uint8_t ns = 0; ns<nuNumMixOutConfigs; ns++) {
				if (bEnblPerChMainAudioScale) {
					for (uint8_t nCh = 0; nCh<nNumMixOutCh[ns]; nCh++)
						nuMainAudioScaleCode[ns][nCh] = (uint8_t)bst.GetBits(6);
				}
				else
					nuMainAudioScaleCode[ns][0] = (uint8_t)bst.GetBits(6);
			}
			uint8_t nEmDM = 1;
			uint8_t nDecCh[2];
			nDecCh[0] = nuTotalNumChs;
			if (bEmbeddedSixChFlag) {
				nDecCh[nEmDM] = 6;
				nEmDM = nEmDM + 1;
			}
			if (bEmbeddedStereoFlag) {
				nDecCh[nEmDM] = 2;
				nEmDM = nEmDM + 1;
			}
			uint8_t nuMixMapMask[4][2][6];
			uint8_t nuNumMixCoefs[4][2][6];
			uint8_t nuMixCoeffs[4][2][6][32];
			for (uint8_t ns = 0; ns<nuNumMixOutConfigs; ns++) { //Configuration Loop
				for (uint8_t nE = 0; nE<nEmDM; nE++) { // Embedded downmix loop
					for (uint8_t nCh = 0; nCh<nDecCh[nE]; nCh++) { //Supplemental Channel Loop
						nuMixMapMask[ns][nE][nCh] = (uint8_t)bst.GetBits(nNumMixOutCh[ns]);
						nuNumMixCoefs[ns][nE][nCh] = CountBitsSet_to_1(nNumMixOutCh[ns], nuMixMapMask[ns][nE][nCh]);
						for (uint8_t nC = 0; nC<nuNumMixCoefs[ns][nE][nCh]; nC++)
							nuMixCoeffs[ns][nE][nCh][nC] = (uint8_t)bst.GetBits(6);
					} // End supplemental channel loop
				} // End of Embedded downmix loop
			} // End configuration loop
		} // End if (bMixMetadataPresent)

		uint16_t nuCoreExtensionMask = 0;
		uint8_t nuCodingMode = (uint8_t)bst.GetBits(2);
		switch (nuCodingMode) {
		case 0:
		{
			nuCoreExtensionMask = (uint16_t)bst.GetBits(12);
			if (nuCoreExtensionMask & DTS_EXSUBSTREAM_CORE) {
				uint16_t nuExSSCoreFsize = (uint16_t)bst.GetBits(14) + 1;
				uint8_t bExSSCoreSyncPresent = (uint8_t)bst.GetBits(1);
				uint8_t nuExSSCoreSyncDistInFrames = 0;
				if (bExSSCoreSyncPresent)
					nuExSSCoreSyncDistInFrames = (uint8_t)(1 << (bst.GetBits(2)));
			}
			uint16_t nuExSSXBRFsize, nuExSSXXCHFsize, nuExSSX96Fsize, nuExSSLBRFsize;
			uint8_t bExSSLBRSyncPresent, nuExSSLBRSyncDistInFrames;
			if (nuCoreExtensionMask & DTS_EXSUBSTREAM_XBR)
				nuExSSXBRFsize = (uint16_t)bst.GetBits(14) + 1;
			if (nuCoreExtensionMask & DTS_EXSUBSTREAM_XXCH)
				nuExSSXXCHFsize = (uint16_t)bst.GetBits(14) + 1;
			if (nuCoreExtensionMask & DTS_EXSUBSTREAM_X96)
				nuExSSX96Fsize = (uint16_t)bst.GetBits(12) + 1;
			if (nuCoreExtensionMask & DTS_EXSUBSTREAM_LBR) {
				nuExSSLBRFsize = (uint16_t)bst.GetBits(14) + 1;
				bExSSLBRSyncPresent = (uint8_t)bst.GetBits(1);
				if (bExSSLBRSyncPresent)
					nuExSSLBRSyncDistInFrames = (uint8_t)(1 << (bst.GetBits(2)));
			}
			if (nuCoreExtensionMask & DTS_EXSUBSTREAM_XLL) {
				uint32_t nuExSSXLLFsize = (uint32_t)bst.GetBits(nuBits4ExSSFsize) + 1;
				uint8_t bExSSXLLSyncPresent = (uint8_t)bst.GetBits(1);
				if (bExSSXLLSyncPresent) {
					uint16_t nuPeakBRCntrlBuffSzkB = (uint16_t)bst.GetBits(4) << 4;
					uint8_t nuBitsInitDecDly = (uint8_t)bst.GetBits(5) + 1;
					uint32_t nuInitLLDecDlyFrames = (uint32_t)bst.GetBits(nuBitsInitDecDly);
					uint32_t nuExSSXLLSyncOffset = (uint32_t)bst.GetBits(nuBits4ExSSFsize);
				}
			}
		}
		break;
		case 1:
		{
			uint32_t nuExSSXLLFsize = (uint32_t)bst.GetBits(nuBits4ExSSFsize) + 1;
			uint8_t bExSSXLLSyncPresent = (uint8_t)bst.GetBits(1);
			if (bExSSXLLSyncPresent) {
				uint16_t nuPeakBRCntrlBuffSzkB = (uint16_t)(bst.GetBits(4) << 4);
				uint8_t nuBitsInitDecDly = (uint8_t)bst.GetBits(5) + 1;
				uint32_t nuInitLLDecDlyFrames = (uint32_t)bst.GetBits(nuBitsInitDecDly);
				uint32_t nuExSSXLLSyncOffset = (uint32_t)bst.GetBits(nuBits4ExSSFsize);
			}
		}
		break;
		case 2:
		{
			uint16_t nuExSSLBRFsize = (uint16_t)bst.GetBits(14) + 1;
			uint8_t bExSSLBRSyncPresent = (uint8_t)bst.GetBits(1);
			uint8_t nuExSSLBRSyncDistInFrames = 0;
			if (bExSSLBRSyncPresent)
				nuExSSLBRSyncDistInFrames = (uint8_t)(1 << (bst.GetBits(2)));
		}
		break;
		case 3:
		{
			uint16_t nuExSSAuxFsize = (uint16_t)bst.GetBits(14) + 1;
			uint8_t nuAuxCodecID = (uint8_t)bst.GetBits(8);
			uint8_t bExSSAuxSyncPresent = (uint8_t)bst.GetBits(1);
			uint8_t nuExSSAuxSyncDistInFrames = 0;
			if (bExSSAuxSyncPresent)
				nuExSSAuxSyncDistInFrames = (uint8_t)bst.GetBits(3) + 1;
		}
		break;
		default:
			break;
		}

		asset_cur_bitpos = bst.Tell();

		uint8_t nuDTSHDStreamID;
		if (((nuCodingMode == 0) && (nuCoreExtensionMask & DTS_EXSUBSTREAM_XLL)) || (nuCodingMode == 1)) {
			nuDTSHDStreamID = (uint8_t)bst.GetBits(3);
		}
		uint8_t bOnetoOneMixingFlag = 0, bEnblPerChMainAudioScale = 0;
		if (bOne2OneMapChannels2Speakers == 1 && bMixMetadataEnbl == 1 && bMixMetadataPresent == 0)
			bOnetoOneMixingFlag = (uint8_t)bst.GetBits(1);
		if (bOnetoOneMixingFlag) {
			bEnblPerChMainAudioScale = (uint8_t)bst.GetBits(1);
			for (uint8_t ns = 0; ns<nuNumMixOutConfigs; ns++) {
				if (bEnblPerChMainAudioScale) {
					for (uint8_t nCh = 0; nCh<nNumMixOutCh[ns]; nCh++)
						nuMainAudioScaleCode[ns][nCh] = (uint8_t)bst.GetBits(6);
				}
				else
					nuMainAudioScaleCode[ns][0] = (uint8_t)bst.GetBits(6);
			}
		} // End of bOnetoOneMixingFlag==true condition
		uint8_t bDecodeAssetInSecondaryDecoder = (uint8_t)bst.GetBits(1);

		asset_cur_bitpos = bst.Tell();

		if (asset_cur_bitpos - asset_start_bitpos < (uint32_t)(nuAssetDescriptFsize << 3))
		{
			bool bDRCMetadataRev2Present = (bst.GetBits(1) == 1) ? true : false;
			if (bDRCMetadataRev2Present == true)
			{
				uint8_t DRCversion_Rev2 = (uint8_t)bst.GetBits(4);
				// one DRC value for each block of 256 samples
				uint8_t nRev2_DRCs = (uint8_t)(nuExSSFrameDurationCode / 256);
				// assumes DRCversion_Rev2 == 1:
				//uint8_t DRCCoeff_Rev2[16];
				for (uint8_t subSubFrame = 0; subSubFrame < nRev2_DRCs; subSubFrame++)
				{
					//DRCCoeff_Rev2[subSubFrame] = dts_dynrng_to_db(bst.GetBits(8));
					bst.SkipBits(8);
				}
			}

			asset_cur_bitpos = bst.Tell();
		}
	}	// End for (int nAst = 0; nAst < nuNumAssets; nAst++)

	if (stm_info.audio_info.bitrate == 0 &&
		stm_info.audio_info.bits_per_sample == 0 &&
		stm_info.audio_info.channel_mapping == 0 &&
		stm_info.audio_info.sample_frequency == 0)
		return -1;

	audio_info = stm_info;

	return 0;
}

int ParseCoreDTSHDAU(unsigned short PID, int stream_type, unsigned long sync_code, unsigned char* pBuf, int cbSize, STREAM_INFO& audio_info)
{
	int iRet = -1;

	if (sync_code == DTS_SYNCWORD_CORE)
	{
		// If Core stream is hit, generate the audio information
		if (g_cur_dtshd_stream_infos.size() > 0)
		{
			if (g_cur_dtshd_stream_infos.size() == 1)
				audio_info = g_cur_dtshd_stream_infos.front();
			else
				audio_info = g_cur_dtshd_stream_infos.back();

			g_cur_dtshd_stream_infos.clear();
			iRet = 0;
		}
	}


	try
	{
		int iRet2 = -1;
		if (sync_code == DTS_SYNCWORD_CORE)
		{
			g_cur_dtshd_stream_infos.emplace_back();
			iRet2 = ParseCoreDTSAU(PID, stream_type, sync_code, pBuf, cbSize, g_cur_dtshd_stream_infos.back());
		}
		else if (sync_code == DTS_SYNCWORD_SUBSTREAM)
		{
			g_cur_dtshd_stream_infos.emplace_back();
			iRet2 = ParseDTSExSSAU(PID, stream_type, sync_code, pBuf, cbSize, g_cur_dtshd_stream_infos.back());
		}

		// The DTS-HD full AU is not complete, reset the previous parse information
		// The first stm_info in g_cur_dtshd_stream_infos should be core DTS
		if (iRet2 < 0)
			g_cur_dtshd_stream_infos.clear();
	}
	catch (...)
	{
		return iRet;
	}

	return iRet;
}

int CheckRawBufferMediaInfo(unsigned short PID, int stream_type, unsigned char* pBuf, int cbSize)
{
	unsigned char* p = pBuf;
	int cbLeft = cbSize;
	int iParseRet = -1;
	unsigned char audio_program_id = 0;
	STREAM_INFO stm_info;

	// At first, check whether the stream type is already decided or not for the current dumped stream.
	if (stream_type == DOLBY_LOSSLESS_AUDIO_STREAM)
	{
		// Try to analyze the MLP audio information.
		// header size:
		// check_nibble/access_unit_length/input_timing: 4 bytes
		// major_sync_info: at least 28 bytes
		const int min_header_size = 32;
		if (cbSize < min_header_size)
			return -1;

		unsigned long sync_code = (p[4] << 16) | (p[5] << 8) | p[6];
		// For the first round, try to find FBA sync code
		while (cbLeft >= min_header_size)
		{
			sync_code = (sync_code << 8) | p[7];
			if (sync_code == FBA_SYNC_CODE || sync_code == FBB_SYNC_CODE)
			{
				int signature = (p[12] << 8) + p[13];

				if (signature == MLP_SIGNATURE)
				{
					/* found it */
					break;
				}
			}
			
			cbLeft--;
			p++;
		}

		if (cbLeft >= min_header_size && (sync_code == FBA_SYNC_CODE || sync_code == FBB_SYNC_CODE))
		{
			if (ParseMLPAU(PID, stream_type, sync_code, p, cbLeft, stm_info) == 0)
			{
				iParseRet = 0;
			}
		}
	}
	else if (DOLBY_AC3_AUDIO_STREAM == stream_type || DD_PLUS_AUDIO_STREAM == stream_type)
	{
		unsigned short sync_code = p[0];
		while (cbLeft >= 8)
		{
			sync_code = (sync_code << 8) | p[1];
			if (sync_code == AC3_SYNCWORD)
				break;

			cbLeft--;
			p++;
		}

		if (cbLeft >= 8)
		{
			if (DOLBY_AC3_AUDIO_STREAM == stream_type)
			{
				if (ParseAC3Frame(PID, stream_type, p, cbLeft, stm_info) == 0)
				{
					iParseRet = 0;
				}
			}
			else if (DD_PLUS_AUDIO_STREAM == stream_type)
			{
				if (ParseEAC3Frame(PID, stream_type, p, cbLeft, stm_info, audio_program_id) == 0)
				{
					iParseRet = 0;
				}
			}
		}
	}
	else if (HDMV_LPCM_AUDIO_STREAM == stream_type)
	{
		if (cbSize >= 4)
		{
			uint16_t audio_data_payload_size = (pBuf[0] << 8) | pBuf[1];
			uint8_t channel_assignment = (pBuf[2] >> 4) & 0x0F;
			uint8_t sample_frequency = pBuf[2] & 0x0F;
			uint8_t bits_per_sample = (pBuf[3] >> 6) & 0x03;

			stm_info.stream_coding_type = stream_type;
			stm_info.audio_info.sample_frequency = sample_frequency == 1 ? 48000 : (sample_frequency == 4 ? 96000 : (sample_frequency == 5 ? 192000 : 0));
			stm_info.audio_info.bits_per_sample = bits_per_sample == 1 ? 16 : (bits_per_sample == 2 ? 20 : (bits_per_sample == 3 ? 24 : 0));
			stm_info.audio_info.channel_mapping = std::get<0>(hdmv_lpcm_ch_assignments[channel_assignment]);
			iParseRet = 0;
		}
	}
	else if (DTS_AUDIO_STREAM == stream_type)
	{
		uint32_t sync_code = p[0];
		while (cbLeft >= 13)
		{
			sync_code = (sync_code << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			if (sync_code == DTS_SYNCWORD_CORE)	// Only Support Big-Endian 32-bit sync code at present.
				break;

			cbLeft--;
			p++;
		}

		if (ParseCoreDTSAU(PID, stream_type, sync_code, p, cbLeft, stm_info) == 0)
		{
			iParseRet = 0;
		}
	}
	else if (DTS_HD_EXCEPT_XLL_AUDIO_STREAM == stream_type || DTS_HD_XLL_AUDIO_STREAM == stream_type)
	{
		uint32_t sync_code = p[0];
		while (cbLeft >= 4)
		{
			sync_code = (sync_code << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			if (sync_code == DTS_SYNCWORD_CORE || sync_code == DTS_SYNCWORD_SUBSTREAM)	// Only Support Big-Endian 32-bit sync code at present.
				break;

			cbLeft--;
			p++;
		}

		if (ParseCoreDTSHDAU(PID, stream_type, sync_code, p, cbLeft, stm_info) == 0)
		{
			iParseRet = 0;
		}
	}

	if (iParseRet == 0)
	{
		bool bChanged = false;
		// Compare with the previous audio information.
		if (g_stream_infos.find(PID) == g_stream_infos.end())
			bChanged = true;
		else
		{
			if (g_stream_infos[PID].size() > audio_program_id)
			{
				STREAM_INFO& cur_stm_info = g_stream_infos[PID][audio_program_id];
				if (memcmp(&cur_stm_info, &stm_info, sizeof(stm_info)) != 0)
					bChanged = true;
			}
			else
				bChanged = true;
		}

		if (bChanged)
		{
			if (g_stream_infos[PID].size() <= audio_program_id)
				g_stream_infos[PID].resize(audio_program_id + 1);
			g_stream_infos[PID][audio_program_id] = stm_info;
			if (g_stream_infos[PID].size() > 1)
				printf("%s Stream information#%d:\r\n", STREAM_TYPE_NAMEA(stm_info.stream_coding_type), audio_program_id);
			else
				printf("%s Stream information:\r\n", STREAM_TYPE_NAMEA(stm_info.stream_coding_type));
			printf("\tPID: 0X%X.\r\n", PID);
			printf("\tStream Type: %d(0X%02X).\r\n", stm_info.stream_coding_type, stm_info.stream_coding_type);
			printf("\tSample Frequency: %d (HZ).\r\n", stm_info.audio_info.sample_frequency);
			printf("\tBits Per Sample: %d.\r\n", stm_info.audio_info.bits_per_sample);
			printf("\tChannel Layout: %s.\r\n", GetChannelMappingDesc(stm_info.audio_info.channel_mapping).c_str());

			if (stm_info.audio_info.bitrate != 0)
			{
				int k = 1000;
				int m = k * k;
				if (stm_info.audio_info.bitrate >= 1024 * 1024)
					printf("\tBitrate: %d.%03d mbps.\r\n", stm_info.audio_info.bitrate / m, stm_info.audio_info.bitrate * 1000 / m % 1000);
				else if(stm_info.audio_info.bitrate >= 1024)
					printf("\tBitrate: %d.%03d kbps.\r\n", stm_info.audio_info.bitrate / k, stm_info.audio_info.bitrate * 1000 / k % 1000);
				else
					printf("\tBitrate: %d bps.\r\n", stm_info.audio_info.bitrate);
			}
		}
	}

	return iParseRet;
}

int WriteWaveFileBuffer(FILE* fw, unsigned PID, int stream_type, unsigned char* es_buffer, int es_buffer_len)
{
	if (fw == NULL)
		return -1;

	// The first LPCM packet, need write the wave header
	uint16_t audio_data_payload_size = (es_buffer[0] << 8) | es_buffer[1];
	uint8_t channel_assignment = (es_buffer[2] >> 4) & 0x0F;
	uint8_t sample_frequency = es_buffer[2] & 0x0F;
	uint8_t bits_per_sample = (es_buffer[3] >> 6) & 0x03;

	if (es_buffer_len < audio_data_payload_size + 4)
		return -1;

	uint32_t numch[] = { 0, 1, 0, 2, 4, 4,4, 4, 6, 6, 8, 8, 0, 0, 0, 0 };
	uint32_t fs[] = { 0, 48000, 0, 0, 96000, 192000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t bps[] = { 0, 16, 20, 24 };

	if (numch[channel_assignment] == 0)
	{
		if (g_verbose_level > 0)
			printf("Channel assignment value (%d) in PCM header is invalid.\r\n", channel_assignment);
		return -1;
	}

	if (fs[sample_frequency] == 0)
	{
		if (g_verbose_level > 0)
			printf("Sample frequency value (%d) in PCM header is invalid.\r\n", sample_frequency);
		return -1;
	}

	if (bps[bits_per_sample] == 0)
	{
		if (g_verbose_level > 0)
			printf("Bits_per_sample(%d) in PCM header is invalid.\r\n", bits_per_sample);
		return -1;
	}

	uint8_t* hdmv_lpcm_payload_buf = new uint8_t[audio_data_payload_size];
	memcpy(hdmv_lpcm_payload_buf, es_buffer + 4, audio_data_payload_size);

	bool bWaveFormatExtent = (numch[channel_assignment] > 2 || bps[bits_per_sample] > 16) ? true : false;
	uint32_t dwChannelMask = 0;
	for (int i = 0; i < _countof(Channel_Speaker_Mapping); i++)
		if (CHANNLE_PRESENT(std::get<0>(hdmv_lpcm_ch_assignments[channel_assignment]), i))
			dwChannelMask |= CHANNEL_BITMASK(Channel_Speaker_Mapping[i]);

	if (g_dump_status.num_of_dumped_payloads == 0)
	{
		std::vector<uint8_t> riff_hdr_bytes;
		if (!bWaveFormatExtent)
		{
			// mono or stereo, old wave format file can support it.
			tuple<
				uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
				uint16_t, uint16_t, uint32_t, uint32_t, uint16_t, uint16_t,
				uint32_t, uint32_t> RIFFHdr = 
			{
				ENDIANULONG('RIFF'), 0, ENDIANULONG('WAVE'), ENDIANULONG('fmt '), 16,
				1, numch[channel_assignment], fs[sample_frequency], numch[channel_assignment] * fs[sample_frequency] * (bps[bits_per_sample] + 7) / 8, 
				numch[channel_assignment] * ((bps[bits_per_sample] + 7) / 8), (bps[bits_per_sample] + 7) & 0xFFF8,
				ENDIANULONG('data'), 0
			};
	
			
			fill_bytes_from_tuple(RIFFHdr, riff_hdr_bytes);
		}
		else
		{
			// multiple channels, or 20-bit LPCM
			tuple<
				uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
				uint16_t, uint16_t, uint32_t, uint32_t, uint16_t, uint16_t, uint16_t, uint16_t, uint32_t,
				uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t,
				uint32_t, uint32_t, uint32_t,
				uint32_t, uint32_t> RIFFHdr = 
			{
				ENDIANULONG('RIFF'), 0, ENDIANULONG('WAVE'), ENDIANULONG('fmt '), 40,
				0xFFFE, numch[channel_assignment], fs[sample_frequency], numch[channel_assignment] * fs[sample_frequency] * (bps[bits_per_sample] + 7) / 8,
				numch[channel_assignment] * ((bps[bits_per_sample] + 7) / 8), (bps[bits_per_sample] + 7) & 0xFFF8,
				22, bps[bits_per_sample], dwChannelMask, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71,
				ENDIANULONG('fact'), 4, 0,
				ENDIANULONG('data'), 0
			};

			fill_bytes_from_tuple(RIFFHdr, riff_hdr_bytes);
		}

		if (fw != NULL)
			fwrite(riff_hdr_bytes.data(), 1, riff_hdr_bytes.size(), fw);
	}

	uint8_t* raw_data = es_buffer + 4;
	int raw_data_len = es_buffer_len - 4;

	uint8_t* pDstSample = raw_data;
	uint8_t* pSrcSample = hdmv_lpcm_payload_buf;
	int BPS = (bps[bits_per_sample] + 7) / 8;
	int group_size = numch[channel_assignment] * BPS;
	std::vector<uint8_t>& chmaps = std::get<1>(hdmv_lpcm_ch_assignments[channel_assignment]);

	// CHANNLE_LOC -> SPEAKER_LOC -> Channel Number
	int speaker_ch_num = 0;
	int Speaker_Channel_Numbers[32] = { -1 };
	for (int i = SPEAKER_POS_FRONT_LEFT; i < SPEAKER_POS_MAX; i++)
		if (dwChannelMask&CHANNEL_BITMASK(i))
			Speaker_Channel_Numbers[i] = speaker_ch_num++;

	for (int i = 0; i < audio_data_payload_size / group_size; i++)
	{
		for (size_t src_ch = 0; src_ch < chmaps.size(); src_ch++)
		{
			size_t dst_ch = Speaker_Channel_Numbers[Channel_Speaker_Mapping[chmaps[src_ch]]];
			for (int B = 0; B < BPS; B++)
			{
				pDstSample[dst_ch*BPS + B] = pSrcSample[src_ch*BPS + BPS - B - 1];
			}
		}

		pDstSample += group_size;
		pSrcSample += group_size;
	}

	raw_data_len = audio_data_payload_size / group_size*group_size;

	// Write the data body
	if (fw != NULL && raw_data_len > 0)
		fwrite(raw_data, 1, raw_data_len, fw);

	delete[] hdmv_lpcm_payload_buf;

	return 0;
}

int FinalizeWaveFileBuffer(FILE* fw, unsigned PID, int stream_type)
{
	long long real_file_size = _ftelli64(fw);
	if (real_file_size == 0)
		return -1;

	uint32_t file_size = real_file_size > UINT32_MAX ? UINT32_MAX : (uint32_t)real_file_size;

	fseek(fw, 0, SEEK_SET);
	unsigned char wave_header1[36];
	if (fread(wave_header1, 1, sizeof(wave_header1), fw) <= 0)
		return -1;

	uint16_t audio_format = wave_header1[20] | (wave_header1[21] << 8);
	uint16_t num_of_chs = wave_header1[22] | (wave_header1[23] << 8);
	*((uint32_t*)(&wave_header1[4])) = file_size - 8;

	// write-back the header.
	fseek(fw, 0, SEEK_SET);
	fwrite(wave_header1, 1, sizeof(wave_header1), fw);

	uint32_t data_chunk_size = 0;
	if (audio_format == 1)
	{
		data_chunk_size = file_size - 36 - 8;
	}
	else if (audio_format == 0xFFFE)
	{
		fseek(fw, 24, SEEK_CUR);

		uint8_t fact_chunk_buf[12];
		if (fread(fact_chunk_buf, 1, sizeof(fact_chunk_buf), fw) <= 0)
			return -1;

		data_chunk_size = file_size - 36 - 24 - 12 - 4;
		uint32_t each_channel_size = data_chunk_size / num_of_chs;
		*((uint32_t*)(&fact_chunk_buf[8])) = (each_channel_size) & 0xFF;

		fseek(fw, -1 * (long)(sizeof(fact_chunk_buf)), SEEK_CUR);
		fwrite(fact_chunk_buf, 1, sizeof(fact_chunk_buf), fw);
	}

	if (data_chunk_size > 0)
	{
		uint8_t data_chunk_hdr[8];
		if (fread(data_chunk_hdr, 1, sizeof(data_chunk_hdr), fw) <= 0)
			return -1;

		*((uint32_t*)(&data_chunk_hdr[4])) = data_chunk_size;

		fseek(fw, -1 * (long)(sizeof(data_chunk_hdr)), SEEK_CUR);
		fwrite(data_chunk_hdr, 1, sizeof(data_chunk_hdr), fw);
	}

	return 0;
}

int FlushPSIBuffer(FILE* fw, unsigned char* psi_buffer, int psi_buffer_len, int dumpopt, int &raw_data_len)
{
	int iret = 0;
	// For PSI, store whole payload with pointer field
	raw_data_len = psi_buffer_len;
	if (fw != NULL)
		fwrite(psi_buffer, 1, raw_data_len, fw);

	return iret;
}

int FlushPESBuffer(FILE* fw, unsigned short PID, int stream_type, unsigned char* pes_buffer, int pes_buffer_len, int dumpopt, int &raw_data_len, int stream_id = -1, int stream_id_extension = -1)
{
	int iret = 0;
	raw_data_len = 0;
	if (pes_buffer_len >= 9)
	{
		if (pes_buffer[0] == 0 &&
			pes_buffer[1] == 0 &&
			pes_buffer[2] == 1)
		{
			int off = 9;
			int pes_stream_id = pes_buffer[3];
			int pes_stream_id_extension = -1;
			int pes_len = pes_buffer[4] << 8 | pes_buffer[5];
			int pes_hdr_len = pes_buffer[8];
			unsigned char PTS_DTS_flags = (pes_buffer[7] >> 6) & 0x3;
			unsigned char ESCR_flag = (pes_buffer[7] >> 5) & 0x1;
			unsigned char ES_rate_flag = (pes_buffer[7] >> 4) & 0x1;
			unsigned char DSM_trick_mode_flag = (pes_buffer[7] >> 3) & 0x1;
			unsigned char additional_copy_info_flag = (pes_buffer[7] >> 2) & 0x1;
			unsigned char PES_CRC_flag = (pes_buffer[7] >> 1) & 0x1;
			unsigned char pes_hdr_extension_flag = pes_buffer[7] & 0x1;

			if (PTS_DTS_flags == 2)
				off += 5;
			else if (PTS_DTS_flags == 3)
				off += 10;

			if (ESCR_flag)
				off += 6;

			if (ES_rate_flag)
				off += 3;

			if (DSM_trick_mode_flag)
				off += 1;

			if (additional_copy_info_flag)
				off += 1;

			if (PES_CRC_flag)
				off += 2;

			if (off >= pes_buffer_len)
				return -1;

			if (pes_hdr_extension_flag)
			{
				unsigned char PES_private_data_flag = (pes_buffer[off] >> 7) & 0x1;
				unsigned char pack_header_field_flag = (pes_buffer[off] >> 6) & 0x1;
				unsigned char program_packet_sequence_counter_flag = (pes_buffer[off] >> 5) & 0x1;
				unsigned char PSTD_buffer_flag = (pes_buffer[off] >> 4) & 0x1;
				unsigned char PES_extension_flag_2 = pes_buffer[off] & 0x1;
				off += 1;

				if (PES_private_data_flag)
					off += 16;

				if (pack_header_field_flag)
				{
					off++;

					if (off >= pes_buffer_len)
						return -1;

					off += pes_buffer[off];
				}

				if (program_packet_sequence_counter_flag)
					off += 2;

				if (PSTD_buffer_flag)
					off += 2;

				if (off >= pes_buffer_len)
					return -1;

				if (PES_extension_flag_2)
				{
					unsigned char PES_extension_field_length = pes_buffer[off] & 0x7F;

					off++;

					if (off >= pes_buffer_len)
						return -1;

					if (PES_extension_field_length > 0)
					{
						unsigned char stream_id_extension_flag = (pes_buffer[off] >> 7) & 0x1;
						if (stream_id_extension_flag == 0)
						{
							pes_stream_id_extension = pes_buffer[off];
						}
					}
				}
			}

			// filter it by stream_id and stream_id_extension
			if (stream_id != -1 && stream_id != pes_stream_id)
				return 1;	// mis-match with stream_id filter

			if (stream_id_extension != -1 && pes_stream_id_extension != -1 && stream_id_extension != pes_stream_id_extension)
				return 1;	// mis-match with stream_id filter

			if (pes_len == 0)
				raw_data_len = pes_buffer_len - pes_hdr_len - 9;
			else
				raw_data_len = pes_len - 3 - pes_hdr_len;

			unsigned char* raw_data = pes_buffer + pes_hdr_len + 9;

			// Check the media information of current elementary stream
			if (DUMP_MEDIA_INFO_VIEW&dumpopt)
			{
				int raw_buf_len = 0;
				if (pes_len == 0)
					raw_buf_len = pes_buffer_len - pes_hdr_len - 9;
				else
					raw_buf_len = pes_len - 3 - pes_hdr_len;

				if (raw_buf_len > 0)
					CheckRawBufferMediaInfo(PID, stream_type, pes_buffer + pes_hdr_len + 9, raw_data_len);
			}

			if ((dumpopt&DUMP_RAW_OUTPUT) || (dumpopt&DUMP_PCM) || (dumpopt&DUMP_WAV))
			{
				if (pes_buffer_len < pes_len + 6 || pes_buffer_len < pes_hdr_len + 9)
				{
					if (g_verbose_level >= 1)
						printf("The PES buffer length(%d) is too short, it is expected to be greater than %d.\r\n", pes_buffer_len, std::max(pes_len + 6, pes_hdr_len + 9));
					iret = -1;
					goto done;
				}

				if ((dumpopt&DUMP_PCM) || (dumpopt&DUMP_WAV))
				{
					// Check whether stream_type is the supported type or not
					if (stream_type != HDMV_LPCM_AUDIO_STREAM)
					{
						if (g_verbose_level >= 1)
							printf("At present, not sure ES is LPCM stream or not, drop this ES packet.\r\n");
						goto done;
					}

					if (dumpopt&DUMP_WAV)
					{
						iret = WriteWaveFileBuffer(fw, PID, stream_type, raw_data, raw_data_len);
					}

					// strip the LPCM header (4 bytes)
					raw_data += 4;
					raw_data_len -= 4;
				}

				if (fw != NULL && raw_data_len > 0 && !(dumpopt&DUMP_WAV))
					fwrite(raw_data, 1, raw_data_len, fw);
			}
			else if (dumpopt&DUMP_PES_OUTPUT)
			{
				raw_data_len = pes_len == 0 ? pes_buffer_len : pes_len;
				if (fw != NULL)
					fwrite(pes_buffer, 1, raw_data_len, fw);
			}

			if (dumpopt&DUMP_PTS_VIEW)
			{
				if (pes_buffer_len < pes_len + 6 || pes_buffer_len < pes_hdr_len + 9)
				{
					iret = -1;
				}
				else
				{
					static int PktIndex = 0;
					__int64 pts = GetPTSValue(pes_buffer + 9);
					printf("PktIndex:%d, PTS value is %lld(0X%llX).\r\n", PktIndex++, pts, pts);
				}
			}
		}
		else
		{
			iret = -2;	// invalid ES
		}
	}
	else if (pes_buffer_len > 0)
	{
		iret = -3;	// too short PES raw data buffer
	}

done:
	return iret;
}

// Dump a stream with specified PID to a pes/es stream
int DumpOneStream()
{
	int iRet = -1;
	FILE *fp = NULL, *fw = NULL;
	int sPID = FILTER_PID;
	int dumpopt = 0;
	unsigned char* pes_buffer = new (std::nothrow) unsigned char[20 * 1024 * 1024];

	unordered_map<unsigned short, CPayloadBuf*> pPSIBufs;
	unordered_map<unsigned short, unsigned char> stream_types;

	pPSIBufs[0] = new CPayloadBuf(PID_PROGRAM_ASSOCIATION_TABLE);

	unsigned char buf[1024];

	if (g_params.find("pid") == g_params.end())
	{
		printf("No PID is specified, nothing to do.\r\n");
		goto done;
	}

	sPID = (int)ConvertToLongLong(g_params["pid"]);

	errno_t errn = fopen_s(&fp, g_params["input"].c_str(), "rb");
	if (errn != 0 || fp == NULL)
	{
		printf("Failed to open the file: %s {errno: %d}.\r\n", g_params["input"].c_str(), errn);
		goto done;
	}

	if (g_params.find("output") != g_params.end())
	{
		errn = fopen_s(&fw, g_params["output"].c_str(), "wb+");
		if (errn != 0 || fw == NULL)
		{
			printf("Failed to open the file: %s {errno: %d}.\r\n", g_params["output"].c_str(), errn);
			goto done;
		}
	}

	size_t pes_hdr_location;
	int raw_data_len = 0, dump_ret;
	unsigned long pes_buffer_len = 0;
	unsigned long pat_buffer_len = 0;
	int buf_head_offset = g_ts_fmtinfo.num_of_prefix_bytes;
	int ts_pack_size = g_ts_fmtinfo.packet_size;

	if (g_params.find("srcfmt") != g_params.end() && g_params["srcfmt"].compare("m2ts") == 0)
		dumpopt |= DUMP_BD_M2TS;

	int stream_id_extension = -1;
	if (g_params.find("stream_id_extension") != g_params.end())
	{
		stream_id_extension = (int)ConvertToLongLong(g_params["stream_id_extension"]);
	}

	int stream_id = -1;
	if (g_params.find("stream_id") != g_params.end())
	{
		stream_id = (int)ConvertToLongLong(g_params["stream_id"]);
	}

	// Make sure the dump option
	if (g_params.find("outputfmt") != g_params.end())
	{
		std::string& strOutputFmt = g_params["outputfmt"];
		if (strOutputFmt.compare("es") == 0)
			dumpopt |= DUMP_RAW_OUTPUT;
		else if (strOutputFmt.compare("pes") == 0)
			dumpopt |= DUMP_PES_OUTPUT;
		else if (strOutputFmt.compare("pcm") == 0)
			dumpopt |= DUMP_PCM;
		else if (strOutputFmt.compare("wav") == 0)
			dumpopt |= DUMP_WAV;
	}

	if (g_params.find("showpts") != g_params.end())
	{
		dumpopt |= DUMP_PTS_VIEW;
	}

	if (g_params.find("showinfo") != g_params.end())
	{
		dumpopt |= DUMP_MEDIA_INFO_VIEW;
	}

	g_dump_status.state = DUMP_STATE_RUNNING;

	unsigned long ts_pack_idx = 0;
	while (true)
	{
		int nRead = fread(buf, 1, ts_pack_size, fp);
		if (nRead < ts_pack_size)
			break;

		unsigned short PID = (buf[buf_head_offset + 1] & 0x1f) << 8 | buf[buf_head_offset + 2];

		bool bPSI = pPSIBufs.find(PID) != pPSIBufs.end() ? true : false;

		// Continue the parsing when PAT/PMT is hit
		if (PID != sPID && !bPSI)
		{
			g_dump_status.cur_pack_idx++;
			ts_pack_idx++;
			continue;
		}

		int index = buf_head_offset + 4;
		unsigned char payload_unit_start = buf[buf_head_offset + 1] & 0x40;
		unsigned char adaptation_field_control = (buf[buf_head_offset + 3] >> 4) & 0x03;
		unsigned char discontinuity_counter = buf[buf_head_offset + 3] & 0x0f;

		if (adaptation_field_control & 0x02)
			index += buf[buf_head_offset + 4] + 1;

		if (payload_unit_start)
		{
			if (bPSI)
			{
				if (pPSIBufs.find(PID) != pPSIBufs.end())
				{
					if (pPSIBufs[PID]->ProcessPMT(pPSIBufs) == 0)
					{
						// Update each PID stream type
						unsigned char stream_type = 0xFF;
						if (pPSIBufs[PID]->GetPMTInfo(sPID, stream_type) == 0)
							stream_types[sPID] = stream_type;
					}

					pPSIBufs[PID]->Reset();
				}
			}

			if (PID == sPID && pes_buffer_len > 0)
			{
				if (bPSI)
				{
					if ((dump_ret = FlushPSIBuffer(fw, pes_buffer, pes_buffer_len, dumpopt, raw_data_len)) < 0)
						printf(dump_msg[-dump_ret - 1], ftell(fp), ftell(fw));
				}
				else
				{
					if ((dump_ret = FlushPESBuffer(fw, 
						PID, stream_types.find(PID) == stream_types.end()?-1: stream_types[PID], 
						pes_buffer, pes_buffer_len, dumpopt, raw_data_len, stream_id, stream_id_extension)) < 0)
						printf(dump_msg[-dump_ret - 1], ftell(fp), ftell(fw));
				}

				g_dump_status.num_of_processed_payloads++;
				if (dump_ret == 0)
					g_dump_status.num_of_dumped_payloads++;

				pes_buffer_len = 0;
				pes_hdr_location = ftell(fp) - nRead;
			}
		}

		if (bPSI)
		{
			if (pPSIBufs[PID]->PushTSBuf(ts_pack_idx, buf, index, (unsigned char)g_ts_fmtinfo.packet_size) >= 0)
			{
				// Try to process PSI buffer
				int nProcessPMTRet = pPSIBufs[PID]->ProcessPMT(pPSIBufs);
				// If process PMT result is -1, it means the buffer is too small, don't reset the buffer.
				if (nProcessPMTRet != -1)
					pPSIBufs[PID]->Reset();

				if (nProcessPMTRet == 0)
				{
					// Update each PID stream type
					unsigned char stream_type = 0xFF;
					if (pPSIBufs[PID]->GetPMTInfo(sPID, stream_type) == 0)
					{
						stream_types[sPID] = stream_type;

						// Check whether it is a supported stream type or not
						if ((dumpopt&DUMP_PCM) || (dumpopt&DUMP_WAV))
						{
							if (stream_type != HDMV_LPCM_AUDIO_STREAM)
							{
								printf("At present only support dumping pcm/wav data from LPCM audio stream.\r\n");
								iRet = -1;
								goto done;
							}
						}
					}
				}
			}
		}

		if (PID == sPID && (payload_unit_start || !payload_unit_start && pes_buffer_len > 0))
		{
			memcpy(pes_buffer + pes_buffer_len, buf + index, g_ts_fmtinfo.packet_size - index);
			pes_buffer_len += g_ts_fmtinfo.packet_size - index;
		}

		ts_pack_idx++;
		g_dump_status.cur_pack_idx++;
	}

	if (pes_buffer_len > 0)
	{
		if (sPID == PID_PROGRAM_ASSOCIATION_TABLE || pPSIBufs.find(sPID) != pPSIBufs.end())
		{
			if ((dump_ret = FlushPSIBuffer(fw, pes_buffer, pes_buffer_len, dumpopt, raw_data_len)) < 0)
				printf(dump_msg[-dump_ret - 1], ftell(fp), ftell(fw));
		}
		else
		{
			if ((dump_ret = FlushPESBuffer(fw,
				sPID, stream_types.find(sPID) == stream_types.end() ? -1 : stream_types[sPID],
				pes_buffer, pes_buffer_len, dumpopt, raw_data_len, stream_id, stream_id_extension)) < 0)
				printf(dump_msg[-dump_ret - 1], ftell(fp), ftell(fw));
		}

		g_dump_status.num_of_processed_payloads++;
		if (dump_ret == 0)
			g_dump_status.num_of_dumped_payloads++;
	}

	if (dumpopt&DUMP_WAV)
	{
		// Fill the correct value in WAVE file header
		FinalizeWaveFileBuffer(fw, sPID, stream_types.find(sPID) == stream_types.end() ? -1 : stream_types[sPID]);
	}

	// Finish the dumping w/o any error
	g_dump_status.completed = 1;

	iRet = 0;

done:
	if (fp)
		fclose(fp);
	if (fw)
		fclose(fw);

	if (pes_buffer != NULL)
	{
		delete[] pes_buffer;
		pes_buffer = NULL;
	}

	for (std::unordered_map<unsigned short, CPayloadBuf*>::iterator iter = pPSIBufs.begin(); iter != pPSIBufs.end(); iter++)
	{
		delete iter->second;
		iter->second = NULL;
	}

	return iRet;
}