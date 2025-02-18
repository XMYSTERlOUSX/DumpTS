#include "StdAfx.h"
#include <stdio.h>
#include "PayloadBuf.h"
#include "AMRingBuffer.h"
#include <algorithm>
#include "Syncer.h"

extern unordered_map<std::string, std::string> g_params;

// option, 1: only PCR; 2: PCR + video; 3: PCR + audio; 4: PCR + all elementary streams
int ShowPCR(int option)
{
	int iRet = -1;
	uint8_t header_offset = 0;
	unsigned long ts_pack_idx = 0;
	unsigned char ts_pack_size = TS_PACKET_SIZE;
	unsigned char buf[TS_PACKET_SIZE];
	FILE *fp = NULL;
	size_t nRead = 0;
	uint8_t packet_start_code_prefix[3] = { 0 };
	uint8_t stream_id = 0;
	std::map<unsigned short, AMLinearRingBuffer> ring_buffers;
	std::map<unsigned short, std::tuple<uint32_t/*start ATC time*/, uint32_t/*AU size(how many TS packs)*/, uint32_t/*AU idx*/>> filter_stat;
	int64_t prev_PCR_byte_position = INT64_MAX;
	uint64_t prev_PCR = UINT64_MAX;

	errno_t errn = fopen_s(&fp, g_params["input"].c_str(), "rb");
	if (errn != 0 || fp == NULL)
	{
		printf("Failed to open the file: %s {errno: %d}.\n", g_params["input"].c_str(), errn);
		goto done;
	}

	nRead = fread(buf, 1, TS_PACKET_SIZE - 4, fp);
	if (nRead < TS_PACKET_SIZE - 4)
	{
		printf("The TS stream file is too small.\n");
		goto done;
	}

	if (buf[0] == 0x47)
	{
		header_offset = 0;
		ts_pack_size = TS_PACKET_SIZE - 4;
	}
	else if (buf[4] == 0x47)
	{
		header_offset = 4;
		ts_pack_size = TS_PACKET_SIZE;
	}
	else
	{
		printf("Unsupported transport stream.\n");
		goto done;
	}

	// reset the file read pointer
	FSEEK64(fp, 0, SEEK_SET);
	while (true)
	{
		size_t nRead = fread(buf, 1, ts_pack_size, fp);
		if (nRead < ts_pack_size)
			break;

		if (buf[header_offset] != 0x47)
		{
			printf("Seems to hit an wrong TS packet.\n");
			break;
		}

		uint16_t PID = ((buf[header_offset + 1] & 0x1F) << 8) | (buf[header_offset + 2]);
		uint16_t adaptation_field_control = (buf[header_offset + 3] >> 4) & 0x3;
		uint8_t  payload_unit_start_indicator = (buf[header_offset + 1] >> 6) & 0x1;
		uint8_t  continuity_counter = (buf[header_offset + 4] & 0xF);
		uint32_t atc_tm = UINT32_MAX;

		if (header_offset == 4)
			atc_tm = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

		if (PID >= 0x10 && PID <= 0x1FFE)
		{
			AMLinearRingBuffer lrb = NULL;
			auto iter = ring_buffers.find(PID);
			if (payload_unit_start_indicator == 1)
			{
				if (iter != ring_buffers.end())
				{
					int read_buf_len = 0;
					lrb = iter->second;
					uint8_t* pes_buf = AM_LRB_GetReadPtr(lrb, &read_buf_len);

					if (read_buf_len > 9 && pes_buf[0] == 0 && pes_buf[1] == 0 && pes_buf[2] == 0x01)
					{
						stream_id = pes_buf[3];
						uint16_t PES_packet_length = (pes_buf[4] << 8) | pes_buf[5];

						if (stream_id != SID_PROGRAM_STREAM_MAP
							&& stream_id != SID_PADDING_STREAM
							&& stream_id != SID_PRIVATE_STREAM_2
							&& stream_id != SID_ECM
							&& stream_id != SID_EMM
							&& stream_id != SID_PROGRAM_STREAM_DIRECTORY
							&& stream_id != SID_DSMCC_STREAM
							&& stream_id != SID_H222_1_TYPE_E)
						{
							uint8_t reserved_0 = (pes_buf[6] >> 6) & 0x3;
							uint8_t PES_scrambling_control = (pes_buf[6] >> 4) & 0x3;

							uint8_t PTS_DTS_flags = (pes_buf[7] >> 6) & 0x3;
							uint8_t PES_header_data_length = pes_buf[8];

							uint64_t pts = UINT64_MAX, dts = UINT64_MAX, start_atc_tm = UINT64_MAX;

							if ((PTS_DTS_flags == 0x2 && ((pes_buf[9] >> 4) & 0xF) == 0x2 && read_buf_len > 14 && PES_header_data_length >= 5) ||
								(PTS_DTS_flags == 0x3 && ((pes_buf[9] >> 4) & 0xF) == 0x3 && read_buf_len > 19 && PES_header_data_length >= 10))
							{
								pts = 0;
								pts = (pes_buf[9] >> 1) & 0x7;
								pts = pts << 15;
								pts |= (pes_buf[10] << 7) | (pes_buf[11] >> 1);
								pts = pts << 15;
								pts |= (pes_buf[12] << 7) | (pes_buf[13] >> 1);

								if (PTS_DTS_flags == 0x3)
								{
									dts = 0;
									dts = (pes_buf[14] >> 1) & 0x7;
									dts = dts << 15;
									dts |= (pes_buf[15] << 7) | (pes_buf[16] >> 1);
									dts = dts << 15;
									dts |= (pes_buf[17] << 7) | (pes_buf[18] >> 1);
								}

								start_atc_tm = std::get<0>(filter_stat[PID]);

								if (option == 4 || (option == 2 && (stream_id & 0xF0) == 0xE0) || (option == 3 && (stream_id & 0xC0) == 0xC0))
								{
									if (PTS_DTS_flags == 0x3)
									{
										printf("%sPES_PID: 0X%04X PTS(base: %10" PRIu64 "(90KHZ),           %13" PRIu64 "(27MHZ), %10" PRIu64 ".%03" PRIu64 "(ms)),\n"
											"                    DTS(base: %10" PRIu64 "(90KHZ)            %13" PRIu64 "(27MHZ), %10" PRIu64 ".%03" PRIu64 "(ms)), ATC interval %" PRIu32 ", %" PRIu64 ".%03" PRIu64 "(ms).\n",
											(stream_id & 0xF0) == 0xE0 ? "[<V]" : (
											(stream_id & 0xC0) == 0xC0 ? "[<A]" : "    "),
											PID, pts, pts * 300, pts / 90, pts * 1000 / 90 % 1000, dts, dts * 300, dts / 90, dts * 1000 / 90 % 1000,
											std::get<1>(filter_stat[PID]), (atc_tm - start_atc_tm) / 27000, (atc_tm - start_atc_tm) / 27 % 1000);
									}
									else
									{
										printf("%sPES_PID: 0X%04X PTS(base: %10" PRIu64 "(90KHZ),           %13" PRIu64 "(27MHZ), %10" PRIu64 ".%03" PRIu64 "(ms)), ATC interval %" PRIu32 ", %" PRIu64 ".%03" PRIu64 "(ms).\n",
											(stream_id & 0xF0) == 0xE0 ? "[<V]" : (
											(stream_id & 0xC0) == 0xC0 ? "[<A]" : "    "),
											PID, pts, pts * 300, pts / 90, pts * 1000 / 90 % 1000,
											std::get<1>(filter_stat[PID]), (atc_tm - start_atc_tm) / 27000, (atc_tm - start_atc_tm) / 27 % 1000);
									}
								}
							}
						}
					}

					AM_LRB_Reset(lrb);
				}

				uint8_t* p = &buf[header_offset + 4];
				if (adaptation_field_control == 0x2 || adaptation_field_control == 0x3)
				{
					uint8_t adaptation_field_length = *p++;
					p += adaptation_field_length;
				}


				if (ts_pack_size - (unsigned char)(p - &buf[0]) >= 4 && p[0] == 0 && p[1] == 0 && p[2] == 1)
					stream_id = p[3];
				else
					stream_id = 0;

				if (option == 4 || (option == 2 && (stream_id & 0xF0) == 0xE0) || (option == 3 && (stream_id & 0xC0) == 0xC0))
				{
					printf("%sPES_PID: 0X%04X, AU_idx: %10" PRIu32 ", ATC_ts: %" PRIu32 "\n",
						(stream_id & 0xF0) == 0xE0 ? "[>V]" : (
						(stream_id & 0xC0) == 0xC0 ? "[>A]" : "    "),
						PID, iter == ring_buffers.end() ? 0 : std::get<2>(filter_stat[PID]), atc_tm);
				}

				if (iter == ring_buffers.end())
				{
					lrb = ring_buffers[PID] = AM_LRB_Create(1024);
					std::get<2>(filter_stat[PID]) = 0;
				}
				else
				{
					std::get<2>(filter_stat[PID])++;
				}

				if (header_offset == 4)
					std::get<0>(filter_stat[PID]) = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
			}

			std::get<1>(filter_stat[PID])++;

			if (lrb != NULL)
			{
				uint8_t* p = &buf[header_offset + 4];
				if (adaptation_field_control == 0x2 || adaptation_field_control == 0x3)
				{
					uint8_t adaptation_field_length = *p++;
					p += adaptation_field_length;
				}

				// begin push the data into the linear buffer, only save the beginning of 1024 bytes
				int write_buf_len = 0;
				uint8_t* pWriteBuf = AM_LRB_GetWritePtr(lrb, &write_buf_len);
				if (pWriteBuf != NULL && write_buf_len > 0)
				{
					if (ts_pack_size - (unsigned char)(p - &buf[0]) < write_buf_len)
						write_buf_len = ts_pack_size - (unsigned char)(p - &buf[0]);

					memcpy(pWriteBuf, p, write_buf_len);
					AM_LRB_SkipWritePtr(lrb, write_buf_len);
				}
			}
		}

		if (!(adaptation_field_control & 0x2))
			continue;

		uint8_t* p = &buf[header_offset + 4];
		uint8_t adaptation_field_length = *p++;

		if (adaptation_field_length == 0)
			continue;

		uint8_t PCR_flag = ((*p) >> 4) & 0x1;
		uint8_t OPCR_flag = ((*p) >> 3) & 0x1;

		p++;

		if (PCR_flag == 0)
			continue;

		int64_t byte_position = FTELL64(fp) - ts_pack_size;

		uint64_t program_clock_reference_base = 0;
		for (int i = 0; i < 4; i++)
			program_clock_reference_base = (program_clock_reference_base << 8) | *(p++);

		program_clock_reference_base = (program_clock_reference_base << 1) | (((*p) >> 7) & 1);

		uint16_t program_clock_reference_extension = (*p) & 1;
		p++;
		program_clock_reference_extension = (program_clock_reference_extension << 8) | *(p++);

		uint64_t original_program_clock_reference_base = UINT64_MAX;
		uint16_t original_program_clock_reference_extension = UINT16_MAX;
		if (OPCR_flag)
		{
			original_program_clock_reference_base = 0;
			for (int i = 0; i < 4; i++)
				original_program_clock_reference_base = (original_program_clock_reference_base << 8) | *(p++);

			original_program_clock_reference_base = (original_program_clock_reference_base << 1) | (((*p) >> 7) & 1);

			original_program_clock_reference_extension = (*p) & 1;
			p++;
			original_program_clock_reference_extension = (original_program_clock_reference_extension << 8) | *(p++);

			printf(" -> PCR_PID: 0X%04X PCR(base: %10" PRIu64 "(90KHZ), ext: %3" PRIu16 ", %13" PRIu64 "(27MHZ), %10" PRIu64 ".%03" PRIu64 "(ms)), OPCR(base: %10" PRIu64 ", ext: %3" PRIu16 ", %" PRIu64 "(27MHZ), %" PRIu64 ".%03" PRIu64 "(ms)), "
				"transport_rate: %" PRId64 "bps\n",
				PID,
				program_clock_reference_base, program_clock_reference_extension,
				program_clock_reference_base * 300 + program_clock_reference_extension,
				(program_clock_reference_base * 300 + program_clock_reference_extension) / 27000,
				(program_clock_reference_base * 300 + program_clock_reference_extension) / 27 % 1000,
				original_program_clock_reference_base, original_program_clock_reference_extension,
				original_program_clock_reference_base * 300 + original_program_clock_reference_extension,
				(original_program_clock_reference_base * 300 + original_program_clock_reference_extension) / 27000,
				(original_program_clock_reference_base * 300 + original_program_clock_reference_extension) / 27 % 1000,
				prev_PCR_byte_position == INT64_MAX ? 0 : (byte_position - prev_PCR_byte_position) * 27000000 * 8 / (program_clock_reference_base * 300 + program_clock_reference_extension - prev_PCR)
			);
		}
		else
		{
			printf(" -> PCR_PID: 0X%04X PCR(base: %10" PRIu64 "(90KHZ), ext: %3" PRIu16 ", %13" PRIu64 "(27MHZ), %10" PRIu64 ".%03" PRIu64 "(ms)), "
				"transport_rate: %" PRId64 "bps\n",
				PID,
				program_clock_reference_base, program_clock_reference_extension,
				program_clock_reference_base * 300 + program_clock_reference_extension,
				(program_clock_reference_base * 300 + program_clock_reference_extension) / 27000,
				(program_clock_reference_base * 300 + program_clock_reference_extension) / 27 % 1000,
				prev_PCR_byte_position == INT64_MAX ? 0 : (byte_position - prev_PCR_byte_position) * 27000000 * 8 / (program_clock_reference_base * 300 + program_clock_reference_extension - prev_PCR)
			);
		}

		prev_PCR_byte_position = byte_position;
		prev_PCR = program_clock_reference_base * 300ULL + program_clock_reference_extension;
	}

done:
	for (auto lrb : ring_buffers)
	{
		if (lrb.second != NULL)
			AM_LRB_Destroy(lrb.second);
	}

	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
	return iRet;
}

int DiffTSATC()
{
	int iRet = -1;
	long long diff_threshold = -1;
	unsigned long ts_pack_idx = 0;
	unsigned char ts_pack_size = TS_PACKET_SIZE;
	unsigned char buf[TS_PACKET_SIZE];
	FILE *fp = NULL;
	long long start_pkt_idx = -1LL;
	long long end_pkt_idx = -1LL;
	long long pkt_idx = 0;
	uint32_t previous_arrive_time = UINT32_MAX;

	errno_t errn = fopen_s(&fp, g_params["input"].c_str(), "rb");
	if (errn != 0 || fp == NULL)
	{
		printf("Failed to open the file: %s {errno: %d}.\n", g_params["input"].c_str(), errn);
		goto done;
	}

	{
		auto iterDiffATC = g_params.find("diffATC");
		if (iterDiffATC != g_params.end())
		{
			diff_threshold = ConvertToLongLong(iterDiffATC->second);
		}

		auto iterStart = g_params.find("start");
		if (iterStart != g_params.end())
			start_pkt_idx = ConvertToLongLong(iterStart->second);

		auto iterEnd = g_params.find("end");
		if (iterEnd != g_params.end())
			end_pkt_idx = ConvertToLongLong(iterEnd->second);
	}

	while (true)
	{
		size_t nRead = fread(buf, 1, ts_pack_size, fp);
		if (nRead < ts_pack_size)
			break;

		if (buf[4] != 0x47)
		{
			printf("It seems not to be a supported m2ts or tts stream file.\n");
			break;
		}

		uint32_t arrive_time = ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]) & 0x3FFFFFFF;
		int32_t diff = INT32_MAX;

		uint16_t PID = ((buf[5] & 0x1F) << 8) | (buf[7]);

		if (previous_arrive_time != UINT32_MAX)
		{
			if (arrive_time > previous_arrive_time)
				diff = (int32_t)(arrive_time - previous_arrive_time);
			else
				diff = 0x40000000 + arrive_time - previous_arrive_time;

			if ((diff_threshold == -1LL || (diff_threshold > 0 && (long long)diff > diff_threshold)) &&
				(start_pkt_idx == -1LL || (start_pkt_idx >= 0 && pkt_idx >= start_pkt_idx)) &&
				(end_pkt_idx == -1LL || (end_pkt_idx >= 0 && pkt_idx < end_pkt_idx)))
			{
				printf("pkt_idx: %20lld [PID: 0X%04X][header 4bytes: %02X %02X %02X %02X] ATC: 0x%08" PRIX32 "(%10" PRIu32 "), diff: %" PRId32 "(%fms)\n",
					pkt_idx, PID, buf[0], buf[1], buf[2], buf[3], arrive_time, arrive_time, diff, diff*1000.0f / 27000000.f);
			}
		}
		else
		{
			printf("pkt_idx: %20lld [PID: 0X%04X][header 4bytes: %02X %02X %02X %02X] ATC: 0x%08" PRIX32 "(%10" PRIu32 "), diff: \n",
				pkt_idx, PID, buf[0], buf[1], buf[2], buf[3], arrive_time, arrive_time);
		}

		previous_arrive_time = arrive_time;
		pkt_idx++;
	}

	iRet = 0;

done:
	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
	return iRet;
}

int g_debug_sync = 0;

int BenchRead(int option)
{
	int iRet = -1;
	uint8_t header_offset = 0;
	uint32_t ts_pack_idx = 0;
	uint8_t ts_pack_size = TS_PACKET_SIZE;
	uint8_t buf[TS_PACKET_SIZE];
	FILE *fp = NULL;
	size_t nRead = 0;
	uint8_t packet_start_code_prefix[3] = { 0 };
	uint8_t stream_id = 0;
	std::map<unsigned short, AMLinearRingBuffer> ring_buffers;
	std::map<unsigned short, std::tuple<uint32_t/*start ATC time*/, uint32_t/*AU size(how many TS packs)*/, uint32_t/*AU idx*/>> filter_stat;
	int64_t prev_PCR_byte_position = INT64_MAX;
	uint64_t prev_PCR = UINT64_MAX;

	std::chrono::high_resolution_clock::time_point htp_start, htp_end;

	uint32_t arrive_timestamp;
	STDClockSyncer syncer(CLOCK_VALUE_30BIT | CLOCK_VALUE_27MHZ);

	uint64_t total_duration_27MHZ = 0ULL;

	errno_t errn = fopen_s(&fp, g_params["input"].c_str(), "rb");
	if (errn != 0 || fp == NULL)
	{
		printf("Failed to open the file: %s {errno: %d}.\n", g_params["input"].c_str(), errn);
		goto done;
	}

	nRead = fread(buf, 1, TS_PACKET_SIZE - 4, fp);
	if (nRead < TS_PACKET_SIZE - 4)
	{
		printf("The TS stream file is too small.\n");
		goto done;
	}

	if (buf[4] == 0x47)
	{
		header_offset = 4;
		ts_pack_size = TS_PACKET_SIZE;

		arrive_timestamp = ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]) & 0x3FFFFFFF;
		if (syncer.Start(arrive_timestamp) < 0)
		{
			printf("Failed to start the T-STD syncer.\n");
			goto done;
		}
	}
	else
	{
		printf("Unsupported transport stream, please specify the m2ts or tts file.\n");
		goto done;
	}

	// reset the file read pointer
	htp_start = std::chrono::high_resolution_clock::now();

	FSEEK64(fp, 0, SEEK_SET);
	while (!feof(fp))
	{
		size_t nRead = fread(buf, 1, ts_pack_size, fp);
		if (nRead < ts_pack_size)
			break;

		if (buf[header_offset] != 0x47)
		{
			printf("Seems to hit an wrong TTS/M2TS packet.\n");
			break;
		}

		uint32_t new_arrive_timestamp = ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]) & 0x3FFFFFFF;

		total_duration_27MHZ += new_arrive_timestamp >= arrive_timestamp ?
			(new_arrive_timestamp - arrive_timestamp) :
			(0x40000000UL + new_arrive_timestamp - arrive_timestamp);

		arrive_timestamp = new_arrive_timestamp;

		//printf("arrive_timestamp: %lu at TS-Pack#%" PRIu32 "\n", arrive_timestamp, ts_pack_idx);

		int iSyncRet = syncer.Sync(arrive_timestamp);
		if (iSyncRet == RET_CODE_TIME_OUT)
			printf("Hit ATC sync time-out at TS-Pack#%" PRIu32 ", arrive_timestamp: %" PRIu32 "!\n", ts_pack_idx, arrive_timestamp);
		else if (iSyncRet == RET_CODE_CLOCK_DISCONTINUITY)
			printf("Hit ATC discontinuity at TS-Pack#%" PRIu32 "!\n", ts_pack_idx);
		else if (iSyncRet < 0)
			printf("Hit ATC sync error (%d) at TS-Pack#%" PRIu32 ".\n", iSyncRet, ts_pack_idx);

		ts_pack_idx++;
	}

	htp_end = std::chrono::high_resolution_clock::now();

	{
		int64_t num_of_bits_processed = ts_pack_idx * 192LL * 8LL;
		int64_t elapse_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(htp_end - htp_start).count();
		int64_t read_bitrate = num_of_bits_processed * 1000 / elapse_time_ms;
		int64_t transport_rate = num_of_bits_processed * 27000000 / total_duration_27MHZ;
		printf("\n");
		printf("It took %" PRId64 " ms to process the data read.\n", elapse_time_ms);
		printf("The average read rate: %" PRId64 " (bps)/%" PRId64 ".%03d (Mbps), the target transport_rate: %" PRId64 " (bps)/%" PRId64 ".%03d (Mbps)\n",
			read_bitrate, read_bitrate / 1000 / 1000, (uint16_t)(read_bitrate / 1000 % 1000),
			transport_rate, transport_rate / 1000 / 1000, (uint16_t)(transport_rate / 1000 % 1000));
	}
done:
	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
	return iRet;
}
