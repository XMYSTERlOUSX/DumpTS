#include "StdAfx.h"
#include <ctype.h>
#include "AMRingBuffer.h"
#include "MMT.h"
#include "ESRepacker.h"
#include "PayloadBuf.h"
#include <chrono>

extern std::unordered_map<std::string, std::string> g_params;
extern int g_verbose_level;

#define DEFAULT_TLV_PACKETS_PER_DISPLAY 20
int g_TLV_packets_per_display = DEFAULT_TLV_PACKETS_PER_DISPLAY;

enum SHOW_TLV_PACK_OPTION
{
	SHOW_TLV_ALL = 0,
	SHOW_TLV_IPv4,
	SHOW_TLV_IPv6,
	SHOW_TLV_HCIP,
	SHOW_TLV_TCS,
};

bool NeedShowMMTTable()
{
	return (g_params.find("showinfo") != g_params.end() ||
			g_params.find("showPLT") != g_params.end() ||
			g_params.find("showMPT") != g_params.end());
}

bool NeedShowMMTTLVPacket()
{
	return (g_params.find("showIPv4pack") != g_params.end() ||
			g_params.find("showIPv6pack") != g_params.end() ||
			g_params.find("showHCIPpack") != g_params.end() ||
			g_params.find("showTCSpack") != g_params.end());
}

int ShowMMTTLVPacks(SHOW_TLV_PACK_OPTION option)
{
	int nRet = RET_CODE_SUCCESS;
	if (g_params.find("input") == g_params.end())
		return -1;

	std::string szOutputFile;
	std::string szOutputFmt;
	std::string& szInputFile = g_params["input"];

	if (g_params.find("outputfmt") != g_params.end())
		szOutputFmt = g_params["outputfmt"];

	if (g_params.find("output") != g_params.end())
		szOutputFile = g_params["output"];

	CFileBitstream bs(szInputFile.c_str(), 4096, &nRet);

	if (nRet < 0)
	{
		printf("Failed to open the file: %s.\n", szInputFile.c_str());
		return nRet;
	}

	int nParsedTLVPackets = 0;
	int nIPv4Packets = 0, nIPv6Packets = 0, nHdrCompressedIPPackets = 0, nTransmissionControlSignalPackets = 0, nNullPackets = 0, nOtherPackets = 0;
	int nFilterPackets = 0;

	int nLocateSyncTry = 0;
	bool bFindSync = false;
	try
	{
		uint32_t tlv_hdr = 0;
		uint64_t left_bits = 0;
		bs.Tell(&left_bits);

		while (left_bits >= (4ULL << 3))
		{
			tlv_hdr = (uint32_t)bs.PeekBits(32);
			if (((tlv_hdr >> 24) & 0xFF) != 0x7F)
			{
				bFindSync = false;
				if (nLocateSyncTry > 10)
				{
					printf("[MMT/TLV] TLV header should start with 0x7F at file position: %" PRIu64 ".\n", bs.Tell() >> 3);
					nRet = RET_CODE_BUFFER_NOT_COMPATIBLE;
					break;
				}
				else
				{
					bs.SkipBits(8);
					continue;
				}
			}
			else
			{
				if (bFindSync == false)
				{
					nLocateSyncTry++;
					bFindSync = true;
				}
			}

			MMT::TLVPacket* pTLVPacket = nullptr;
			switch ((tlv_hdr >> 16) & 0xFF)
			{
			case MMT::TLV_IPv4_packet:
				pTLVPacket = new MMT::Undefined_TLVPacket();
				nIPv4Packets++;
				break;
			case MMT::TLV_IPv6_packet:
				pTLVPacket = new MMT::IPv6Packet();
				nIPv6Packets++;
				break;
			case MMT::TLV_Header_compressed_IP_packet:
				pTLVPacket = new MMT::HeaderCompressedIPPacket();
				nHdrCompressedIPPackets++;
				break;
			case MMT::TLV_Transmission_control_signal_packet:
				pTLVPacket = new MMT::TransmissionControlSignalPacket();
				nTransmissionControlSignalPackets++;
				break;
			case MMT::TLV_Null_packet:
				pTLVPacket = new MMT::TLVNullPacket();
				nNullPackets++;
				break;
			default:
				pTLVPacket = new MMT::Undefined_TLVPacket();
				nOtherPackets++;
			}

			if ((nRet = pTLVPacket->Unpack(bs)) < 0)
			{
				printf("Failed to unpack a TLV packet at file position: %" PRIu64 ".\n", pTLVPacket->start_bitpos >> 3);
				delete pTLVPacket;
				break;
			}

			bool bFiltered = false;
			if ((option) == SHOW_TLV_ALL ||
				(option == SHOW_TLV_IPv4 && ((tlv_hdr >> 16) & 0xFF) == MMT::TLV_IPv4_packet) ||
				(option == SHOW_TLV_IPv6 && ((tlv_hdr >> 16) & 0xFF) == MMT::TLV_IPv6_packet) ||
				(option == SHOW_TLV_HCIP && ((tlv_hdr >> 16) & 0xFF) == MMT::TLV_Header_compressed_IP_packet) ||
				(option == SHOW_TLV_TCS  && ((tlv_hdr >> 16) & 0xFF) == MMT::TLV_Transmission_control_signal_packet))
			{
				pTLVPacket->Print();
				bFiltered = true;
			}

			if (left_bits < (((uint64_t)pTLVPacket->Data_length + 4ULL) << 3))
			{
				delete pTLVPacket;
				break;
			}

			left_bits -= (((uint64_t)pTLVPacket->Data_length + 4ULL) << 3);
			delete pTLVPacket;

			nParsedTLVPackets++;

			if (bFiltered)
			{
				nFilterPackets++;
				if ((nFilterPackets%g_TLV_packets_per_display) == 0 && g_TLV_packets_per_display != std::numeric_limits<decltype(g_TLV_packets_per_display)>::max())
				{
					printf("Press any key to continue('q': quit)...\n");
					char chk = _getch();
					if (chk == 0x3 || chk == 0x1A || chk == 'q' || chk == 'Q')	// Ctrl + C/Z, quit the loop
						break;
				}
			}
		}
	}
	catch (...)
	{
		nRet = RET_CODE_ERROR;
	}

	printf("The total number of TLV packets: %d.\n", nParsedTLVPackets);
	printf("The number of IPv4 TLV packets: %d.\n", nIPv4Packets);
	printf("The number of IPv6 TLV packets: %d.\n", nIPv6Packets);
	printf("The number of Header Compressed IP packets: %d.\n", nHdrCompressedIPPackets);
	printf("The number of Transmission Control Signal TLV packets: %d.\n", nTransmissionControlSignalPackets);
	printf("The number of Null TLV packets: %d.\n", nNullPackets);
	printf("The number of other TLV packets: %d.\n", nOtherPackets);

	return nRet;
}

// define a PLT/MPT tree for the result
using TreePLTMPT = std::vector<std::tuple<MMT::PackageListTable*, std::vector<MMT::MMTPackageTable*>>>;
using TreeCIDPAMsgs = std::map<uint16_t, TreePLTMPT>;

using TreePkgBuf = std::map<uint16_t, AMLinearRingBuffer>;
using TreeCIDPkgBuf = std::map<uint16_t, TreePkgBuf>;

uint32_t FindAssetType(const TreeCIDPAMsgs& CIDPAMsgs, uint16_t CID, uint16_t packet_id)
{
	auto iterCID = CIDPAMsgs.find(CID);
	if (iterCID == CIDPAMsgs.cend())
		return 0;

	auto& iterM = iterCID->second.back();
	auto& mpts = std::get<1>(iterM);

	// check whether packet lies at the MPT or not
	if (mpts.size() > 0)
	{
		auto& mpt = mpts.back();
		for (auto& a : mpt->assets)
		{
			for (auto& loc : a->MMT_general_location_infos)
			{
				if (loc.UsePacketID(packet_id))
				{
					return a->asset_type;
				}
			}
		}
	}

	return 0;
}

int ProcessPAMessage(TreeCIDPAMsgs& CIDPAMsgs, uint16_t CID, uint64_t packet_id, uint8_t* pMsgBuf, int cbMsgBuf, bool* pbChange=nullptr, int dumpOptions = 0, std::vector<uint16_t>& filter_packets_ids = std::vector<uint16_t>())
{
	if (cbMsgBuf <= 0)
		return RET_CODE_INVALID_PARAMETER;

	CBitstream bs(pMsgBuf, ((size_t)cbMsgBuf<<3));
	uint16_t msg_id = (uint16_t)bs.PeekBits(16);
	
	// If the current message is not a PA message, ignore it.
	if (msg_id != 0)
		return RET_CODE_SUCCESS;

	// Make sure the tree PA message is already there for the specified CID
	auto iter = CIDPAMsgs.find(CID);
	if (iter == CIDPAMsgs.end())
		return RET_CODE_INVALID_PARAMETER;

	int iRet = RET_CODE_SUCCESS;
	MMT::PAMessage PAMsg(cbMsgBuf);
	if ((iRet = PAMsg.Unpack(bs)) < 0)
		return iRet;

	for (auto& t : PAMsg.tables)
	{
		if (t == nullptr)
			continue;

		if (t->table_id == 0x80)	// PLT
		{
			// Check whether there is already this PLT
			MMT::PackageListTable* pPLT = (MMT::PackageListTable*)t;
			if (iter->second.size() > 0)
			{
				auto& tuple_tree = iter->second.back();
				if (pPLT->Compare(std::get<0>(tuple_tree)) == 0)
					continue;
			}

			printf("Found a new PLT with packet_id: %" PRIu64 "(0X%" PRIX64 ") in header compressed IP packet with CID: %d(0X%X)...\n", packet_id, packet_id, CID, CID);

			std::string szLocInfo;
			// Get the packet_id of current MPT
			for (auto& plt_pkg : pPLT->package_infos)
			{
				uint64_t pkg_id = std::get<1>(plt_pkg);
				auto& info = std::get<2>(plt_pkg);
				szLocInfo = info.GetLocDesc();

				printf("\tFound a package with package_id: %" PRIu64 "(0X%" PRIX64 "), %s.\n", pkg_id, pkg_id, szLocInfo.c_str());
			}

			if (g_verbose_level > 0 || (dumpOptions&DUMP_PLT))
				pPLT->Print(stdout, 8);
			
			// A new PLT is found
			iter->second.push_back(std::make_tuple(pPLT, std::vector<MMT::MMTPackageTable*>()));
			// reset the PLT to NULL to avoid destructing it during PAMessage
			t = nullptr;
			if (pbChange)
				*pbChange = true;
		}
		else if (t->table_id == 0x20) // MPT
		{
			bool bFoundExistedMPT = false, bInPLTMPTs = false, bFoundNewMPT = false;;
			MMT::MMTPackageTable* pMPT = (MMT::MMTPackageTable*)t;

			//printf("\n=============MPT version: %d=============\n", t->version);

			// Check whether the current MPT lies at the last PLT or not
			if (iter->second.size() > 0)
			{
				auto& tuple_tree = iter->second.back();
				auto& vMPTs = std::get<1>(tuple_tree);
				for (auto& m : vMPTs)
				{
					if (m->MMT_package_id == pMPT->MMT_package_id)
					{
						bFoundExistedMPT = true;
						break;
					}
				}

				// Already found the existed MPT, continue the next table processing
				if (bFoundExistedMPT == true && !(dumpOptions&DUMP_MPT))
					continue;

				auto& pPLT = std::get<0>(tuple_tree);
				if (pPLT != nullptr)
				{
					for (auto& p : pPLT->package_infos)
					{
						if (pMPT->MMT_package_id == std::get<1>(p))
						{
							bInPLTMPTs = true;
							break;
						}
					}
				}

				if (bInPLTMPTs == true || pPLT == nullptr)
				{
					vMPTs.push_back(pMPT);
					// reset the table to NULL to avoid destructing it during PAMessage
					t = nullptr;
					bFoundNewMPT = true;
					if (pbChange)
						*pbChange = true;
					if (dumpOptions&(DUMP_MPT | DUMP_MEDIA_INFO_VIEW))
						printf("Found a new MPT with packet_id: %" PRIu64 "(0X%" PRIX64 ") in header compressed IP packet with CID: %d(0X%X)...\n", packet_id, packet_id, CID, CID);
				}
				else
				{
					// It is a new MPT which does NOT belong to any PLT, assume there is a null PLT own it
					iter->second.push_back(std::make_tuple(nullptr, std::vector<MMT::MMTPackageTable*>({ pMPT })));
					t = nullptr;
					bFoundNewMPT = true;
					if (pbChange)
						*pbChange = true;
					if (dumpOptions&(DUMP_MPT | DUMP_MEDIA_INFO_VIEW))
						printf("Found a new MPT with packet_id: %" PRIu64 "(0X%" PRIX64 ") in header compressed IP packet with CID: %d(0X%X)...\n", packet_id, packet_id, CID, CID);
				}
			}
			else
			{
				// It is a new MPT which does NOT belong to any PLT, assume there is a null PLT own it
				iter->second.push_back(std::make_tuple(nullptr, std::vector<MMT::MMTPackageTable*>({ pMPT })));
				t = nullptr;
				bFoundNewMPT = true;
				if (pbChange)
					*pbChange = true;
				if (dumpOptions&(DUMP_MPT | DUMP_MEDIA_INFO_VIEW))
					printf("Found a new MPT with packet_id: %" PRIu64 "(0X%" PRIX64 ") in header compressed IP packet with CID: 0X%X(%u)...\n",
						packet_id, packet_id, CID, CID);
			}

			if ((dumpOptions&DUMP_MPT))
			{
				printf("%21s: 0x%X\n", "table_id", pMPT->table_id);
				printf("%21s: 0x%X\n", "version", pMPT->version);
				printf("%21s: %d(%s)\n", "MPT_mode", pMPT->MPT_mode, 
					pMPT->MPT_mode == 0?"MPT is processed according to the order of subset":(
					pMPT->MPT_mode == 1?"After MPT of subset 0 is received, any subset which has the same version number can be processed":(
					pMPT->MPT_mode == 2?"MPT of subset can be processed arbitrarily":"Unknown")));
				printf("%21s: 0x%llX\n", "MMT_package_id", pMPT->MMT_package_id);

				size_t left_descs_bytes = pMPT->MPT_descriptors_bytes.size();
				CBitstream descs_bs(&pMPT->MPT_descriptors_bytes[0], pMPT->MPT_descriptors_bytes.size() << 3);
				while (left_descs_bytes > 0)
				{
					uint16_t peek_desc_tag = (uint16_t)descs_bs.PeekBits(16);
					MMT::MMTSIDescriptor* pDescr = nullptr;
					switch (peek_desc_tag)
					{
					case 0x0001:	// MPUTimestampDescriptor
						pDescr = new MMT::MPUTimestampDescriptor();
						break;
					case 0x8010:	// Video component Descriptor
						pDescr = new MMT::VideoComponentDescriptor();
						break;
					case 0x8014:
						pDescr = new MMT::MHAudioComponentDescriptor();
						break;
					case 0x8011:	// MH-stream identification descriptor:
						pDescr = new MMT::MHStreamIdentifierDescriptor();
						break;
					case 0x8020:
						pDescr = new MMT::MHDataComponentDescriptor();
						break;
					case 0x8026:	// MPUExtendedTimestampDescriptor
						pDescr = new MMT::MPUExtendedTimestampDescriptor();
						break;
					case 0x8038:	// Content copy control Descriptor
						pDescr = new MMT::ContentCopyControlDescriptor();
						break;
					case 0x8039:	// Content usage control Descriptor
						pDescr = new MMT::ContentUsageControlDescriptor();
						break;
					default:
						pDescr = new MMT::UnimplMMTSIDescriptor();
					}

					if (pDescr->Unpack(descs_bs) >= 0)
					{
						pDescr->Print(stdout, 16);

						if (left_descs_bytes < pDescr->descriptor_length + 3UL)
							break;

						left_descs_bytes -= pDescr->descriptor_length + 3UL;

						delete pDescr;
					}
					else
					{
						delete pDescr;
						break;
					}
				}
			}

			if (bFoundNewMPT && pMPT != nullptr)
			{
				for (size_t k = 0; k < pMPT->assets.size(); k++)
				{
					// Check whether the current packet_id should be selected or not
					bool bPacketIDFiltered = false;
					if (filter_packets_ids.size() == 0)
						bPacketIDFiltered = true;

					auto& a = pMPT->assets[k];
					for (auto& info : a->MMT_general_location_infos)
					{
						if (bPacketIDFiltered == false &&
							std::find(filter_packets_ids.cbegin(), filter_packets_ids.cend(), info.packet_id) != filter_packets_ids.cend())
							bPacketIDFiltered = true;
					}

					if (dumpOptions&(DUMP_MPT | DUMP_MEDIA_INFO_VIEW) && bPacketIDFiltered)
					{
						printf("\t#%05" PRIu32 " Asset, asset_id: 0X%" PRIX64 "(%" PRIu64 "), asset_type: %c%c%c%c(0X%08X):\n", (uint32_t)k, a->asset_id, a->asset_id,
							isprint((a->asset_type >> 24) & 0xFF) ? ((a->asset_type >> 24) & 0xFF) : '.',
							isprint((a->asset_type >> 16) & 0xFF) ? ((a->asset_type >> 16) & 0xFF) : '.',
							isprint((a->asset_type >> 8) & 0xFF) ? ((a->asset_type >> 8) & 0xFF) : '.',
							isprint((a->asset_type) & 0xFF) ? ((a->asset_type) & 0xFF) : '.',
							a->asset_type);
					}

					for (auto& info : a->MMT_general_location_infos)
					{
						if (dumpOptions&(DUMP_MPT | DUMP_MEDIA_INFO_VIEW) && bPacketIDFiltered)
							printf("\t\t%s\n", info.GetLocDesc().c_str());
					}

					size_t left_descs_bytes = a->asset_descriptors_bytes.size();
					CBitstream descs_bs(&a->asset_descriptors_bytes[0], a->asset_descriptors_bytes.size() << 3);
					while (left_descs_bytes > 0)
					{
						uint16_t peek_desc_tag = (uint16_t)descs_bs.PeekBits(16);
						MMT::MMTSIDescriptor* pDescr = nullptr;
						switch (peek_desc_tag)
						{
						case 0x0001:	// MPUTimestampDescriptor
							pDescr = new MMT::MPUTimestampDescriptor();
							break;
						case 0x8010:	// Video component Descriptor
							pDescr = new MMT::VideoComponentDescriptor();
							break;
						case 0x8014:
							pDescr = new MMT::MHAudioComponentDescriptor();
							break;
						case 0x8011:	// MH-stream identification descriptor:
							pDescr = new MMT::MHStreamIdentifierDescriptor();
							break;
						case 0x8020:
							pDescr = new MMT::MHDataComponentDescriptor();
							break;
						case 0x8026:	// MPUExtendedTimestampDescriptor
							pDescr = new MMT::MPUExtendedTimestampDescriptor();
							break;
						default:
							pDescr = new MMT::UnimplMMTSIDescriptor();
						}

						if (pDescr->Unpack(descs_bs) >= 0)
						{
							if ((dumpOptions&DUMP_MPT) && bPacketIDFiltered)
							{
								pDescr->Print(stdout, 28);
							}
							else if (peek_desc_tag == 0x8010 || peek_desc_tag == 0x8014)
							{
								if (dumpOptions&(DUMP_MPT | DUMP_MEDIA_INFO_VIEW) && bPacketIDFiltered)
									pDescr->Print(stdout, 28);
							}

							if (left_descs_bytes < pDescr->descriptor_length + 3UL)
								break;

							left_descs_bytes -= pDescr->descriptor_length + 3UL;

							delete pDescr;
						}
						else
						{
							delete pDescr;
							break;
						}
					}
				}
			}
		}
	}
	
	return RET_CODE_SUCCESS;
}

int ProcessMFU(uint16_t CID, uint64_t packet_id, uint8_t payload_type, uint32_t asset_type, uint8_t fragmentation_indicator, uint8_t* pMFUData, int cbMFUData, CESRepacker* pESRepacker=nullptr)
{
	if (payload_type == 0)
	{
		if (pESRepacker != nullptr)
			return pESRepacker->Process(pMFUData, cbMFUData, (FRAGMENTATION_INDICATOR)fragmentation_indicator);
	}

	return RET_CODE_ERROR_NOTIMPL;
}

int ShowMMTPackageInfo()
{
	int dumpopt = 0;
	int nRet = RET_CODE_SUCCESS;
	if (g_params.find("input") == g_params.end())
		return -1;

	std::string& szInputFile = g_params["input"];

	CFileBitstream bs(szInputFile.c_str(), 4096, &nRet);

	if (nRet < 0)
	{
		printf("Failed to open the file: %s.\n", szInputFile.c_str());
		return nRet;
	}

	if (g_params.find("showinfo") != g_params.end())
	{
		dumpopt |= DUMP_MEDIA_INFO_VIEW;
	}

	if (g_params.find("showMPT") != g_params.end())
	{
		dumpopt |= DUMP_MPT;
	}

	if (g_params.find("showPLT") != g_params.end())
	{
		dumpopt |= DUMP_PLT;
	}

	const char* sp;
	const char* ep;
	int64_t i64Val = -1LL;
	std::vector<uint16_t> fitler_packet_ids;
	auto pidIter = g_params.find("pid");
	if (pidIter != g_params.end())
	{
		int iterPID = (int)pidIter->second.find("&");
		if (iterPID == 0 || iterPID == std::string::npos)//Only one pid/packet_id this case
		{
			sp = pidIter->second.c_str();
			ep = sp + pidIter->second.length();
			if (iterPID == 0)
				sp++;
			if (ConvertToInt((char*)sp, (char*)ep, i64Val) == false || i64Val < 0 || i64Val > UINT16_MAX)
			{
				printf("Please specify a valid packet_id.\n");
				return RET_CODE_ERROR;
			}
			fitler_packet_ids.push_back((uint16_t)i64Val);
		}
		else//Two pids/packet_ids
		{
			std::string PIDs;
			PIDs = pidIter->second.substr(0, iterPID);
			sp = PIDs.c_str();
			ep = sp + PIDs.length();
			if (ConvertToInt((char*)sp, (char*)ep, i64Val) == false || i64Val < 0 || i64Val > UINT16_MAX)
			{
				printf("Please specify a valid packet_id.\n");
				return RET_CODE_ERROR;
			}
			fitler_packet_ids.push_back((uint16_t)i64Val);

			PIDs = pidIter->second.substr(iterPID + 1, pidIter->second.length());
			sp = PIDs.c_str();
			ep = sp + PIDs.length();
			if (ConvertToInt((char*)sp, (char*)ep, i64Val) == false || i64Val < 0 || i64Val > UINT16_MAX)
			{
				printf("Please specify a valid packet_id for the second pid.\n");
				return RET_CODE_ERROR;
			}
			fitler_packet_ids.push_back((uint16_t)i64Val);
		}
	}

	int filter_CID = -1;
	auto iterCID = g_params.find("CID");
	if (iterCID != g_params.end())
	{
		sp = iterCID->second.c_str();
		ep = sp + iterCID->second.length();
		if (ConvertToInt((char*)sp, (char*)ep, i64Val) == true && i64Val >= 0 && i64Val <= UINT16_MAX)
			filter_CID = (int)i64Val;
	}

	int nParsedTLVPackets = 0;
	int nIPv4Packets = 0, nIPv6Packets = 0, nHdrCompressedIPPackets = 0, nTransmissionControlSignalPackets = 0, nNullPackets = 0, nOtherPackets = 0;
	TreeCIDPAMsgs CIDPAMsgs;
	std::vector<uint8_t> fullPAMessage;

	int nLocateSyncTry = 0;
	bool bFindSync = false;
	try
	{
		uint32_t tlv_hdr = 0;
		uint64_t left_bits = 0;
		bs.Tell(&left_bits);

		while (left_bits >= (4ULL << 3))
		{
			tlv_hdr = (uint32_t)bs.PeekBits(32);
			if (((tlv_hdr >> 24) & 0xFF) != 0x7F)
			{
				bFindSync = false;
				if (nLocateSyncTry > 10)
				{
					printf("[MMT/TLV] TLV header should start with 0x7F at file position: %" PRIu64 ".\n", bs.Tell() >> 3);
					nRet = RET_CODE_BUFFER_NOT_COMPATIBLE;
					break;
				}
				else
				{
					bs.SkipBits(8);
					continue;
				}
			}
			else
			{
				if (bFindSync == false)
				{
					nLocateSyncTry++;
					bFindSync = true;
				}
			}

			MMT::TLVPacket* pTLVPacket = nullptr;
			switch ((tlv_hdr >> 16) & 0xFF)
			{
			case MMT::TLV_IPv4_packet:
				pTLVPacket = new MMT::Undefined_TLVPacket();
				nIPv4Packets++;
				break;
			case MMT::TLV_IPv6_packet:
				pTLVPacket = new MMT::IPv6Packet();
				nIPv6Packets++;
				break;
			case MMT::TLV_Header_compressed_IP_packet:
				pTLVPacket = new MMT::HeaderCompressedIPPacket();
				nHdrCompressedIPPackets++;
				break;
			case MMT::TLV_Transmission_control_signal_packet:
				pTLVPacket = new MMT::TransmissionControlSignalPacket();
				nTransmissionControlSignalPackets++;
				break;
			case MMT::TLV_Null_packet:
				pTLVPacket = new MMT::TLVNullPacket();
				nNullPackets++;
				break;
			default:
				pTLVPacket = new MMT::Undefined_TLVPacket();
				nOtherPackets++;
			}

			if ((nRet = pTLVPacket->Unpack(bs)) < 0)
			{
				printf("Failed to unpack a TLV packet at file position: %" PRIu64 ".\n", pTLVPacket->start_bitpos >> 3);
				delete pTLVPacket;
				break;
			}

			if (left_bits < (((uint64_t)pTLVPacket->Data_length + 4ULL) << 3))
			{
				printf("The buffer is not enough(left-bits: 0x%llX, required bits: 0x%llX)!\n",
					left_bits, (((uint64_t)pTLVPacket->Data_length + 4ULL) << 3));
				delete pTLVPacket;
				break;
			}

			// Detect the PLT and MPT, and show them
			if (((tlv_hdr >> 16) & 0xFF) == MMT::TLV_Header_compressed_IP_packet)
			{
				MMT::HeaderCompressedIPPacket* pHeaderCompressedIPPacket = (MMT::HeaderCompressedIPPacket*)pTLVPacket;

				// Check whether it is a control message MMT packet
				if (pHeaderCompressedIPPacket->MMTP_Packet == nullptr ||
					pHeaderCompressedIPPacket->MMTP_Packet->Payload_type != 2 ||
					pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages == nullptr)
					goto Skip;

				uint16_t CID = pHeaderCompressedIPPacket->Context_id;
				if (filter_CID >= 0 && filter_CID != CID)
					goto Skip;

				// Check there is an existed CID there or not, if it does NOT exist, add one
				if (CIDPAMsgs.find(CID) == CIDPAMsgs.end())
				{
					TreePLTMPT emptyTreePLTMAP;
					CIDPAMsgs[CID] = emptyTreePLTMAP;
				}

				// Check whether the current PA message contains a complete message
				if (pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->fragmentation_indicator == 0 ||
					pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->fragmentation_indicator == 3 ||
					pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->Aggregate_flag == 1)
				{
					if (fullPAMessage.size() > 0)
					{
						for (auto& m : pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->messages)
						{
							auto& v = std::get<1>(m);
							if (v.size() > 0)
							{
								size_t orig_size = fullPAMessage.size();
								fullPAMessage.resize(orig_size + v.size());
								memcpy(&fullPAMessage[orig_size], &v[0], v.size());
							}
						}
						
						ProcessPAMessage(CIDPAMsgs, CID, pHeaderCompressedIPPacket->MMTP_Packet->Packet_id, &fullPAMessage[0], (int)fullPAMessage.size(), nullptr, dumpopt, fitler_packet_ids);

						fullPAMessage.clear();
					}
					else
					{
						for (auto& m : pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->messages)
						{
							auto& v = std::get<1>(m);
							ProcessPAMessage(CIDPAMsgs, CID, pHeaderCompressedIPPacket->MMTP_Packet->Packet_id, &v[0], (int)v.size(), nullptr, dumpopt, fitler_packet_ids);
						}
					}
				}
				else
				{
					assert(pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->messages.size() <= 1);
					// Need add the data into the ring buffer, and parse it later
					for (auto& m : pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->messages)
					{
						auto& v = std::get<1>(m);
						if (v.size() > 0)
						{
							size_t orig_size = fullPAMessage.size();
							fullPAMessage.resize(orig_size + v.size());
							memcpy(&fullPAMessage[orig_size], &v[0], v.size());
						}
					}
				}
			}

		Skip:
			left_bits -= (((uint64_t)pTLVPacket->Data_length + 4ULL) << 3);
			delete pTLVPacket;

			nParsedTLVPackets++;
		}
	}
	catch (...)
	{
		nRet = RET_CODE_ERROR;
	}

	if (dumpopt&DUMP_MEDIA_INFO_VIEW)
	{
		printf("\nLayout of MMT:\n");

		// Print PLT, MPT and asset tree
		for (auto &m : CIDPAMsgs)
		{
			printf("\tCID: 0X%X(%d):\n", m.first, m.first);
			for (size_t i = 0; i < m.second.size(); i++)
			{
				auto& plt = std::get<0>(m.second[i]);
				auto& vmpt = std::get<1>(m.second[i]);
				printf("\t\t#%04" PRIu32 " PLT (Package List Table):\n", (uint32_t)i);
				for (size_t j = 0; j < vmpt.size(); j++)
				{
					auto& mpt = vmpt[j];

					std::string szLocInfo;
					// Get the packet_id of current MPT
					if (plt != nullptr)
					{
						for (auto& plt_pkg : plt->package_infos)
						{
							if (std::get<1>(plt_pkg) == mpt->MMT_package_id)
							{
								auto& info = std::get<2>(plt_pkg);
								szLocInfo = info.GetLocDesc();
							}
						}
					}

					printf("\t\t\t#%05" PRIu32 " MPT (MMT Package Table), Package_id: 0X%" PRIX64 "(%" PRIu64 "), %s:\n", (uint32_t)j, mpt->MMT_package_id, mpt->MMT_package_id, szLocInfo.c_str());
					for (size_t k = 0; k < mpt->assets.size(); k++)
					{
						auto& a = mpt->assets[k];
						printf("\t\t\t\t#%05" PRIu32 " Asset, asset_id: 0X%" PRIX64 "(%" PRIu64 "), asset_type: %c%c%c%c(0X%08" PRIX32 "):\n", (uint32_t)k, a->asset_id, a->asset_id,
							isprint((a->asset_type >> 24) & 0xFF) ? ((a->asset_type >> 24) & 0xFF) : '.',
							isprint((a->asset_type >> 16) & 0xFF) ? ((a->asset_type >> 16) & 0xFF) : '.',
							isprint((a->asset_type >> 8) & 0xFF) ? ((a->asset_type >> 8) & 0xFF) : '.',
							isprint((a->asset_type) & 0xFF) ? ((a->asset_type) & 0xFF) : '.',
							a->asset_type);

						for (auto& info : a->MMT_general_location_infos)
						{
							printf("\t\t\t\t\t%s\n", info.GetLocDesc().c_str());
						}

						size_t left_descs_bytes = a->asset_descriptors_bytes.size();
						CBitstream descs_bs(&a->asset_descriptors_bytes[0], a->asset_descriptors_bytes.size() << 3);
						while (left_descs_bytes > 0)
						{
							uint16_t peek_desc_tag = (uint16_t)descs_bs.PeekBits(16);
							MMT::MMTSIDescriptor* pDescr = nullptr;
							switch (peek_desc_tag)
							{
							case 0x8010:	// Video component Descriptor
								pDescr = new MMT::VideoComponentDescriptor();
								break;
							case 0x8014:
								pDescr = new MMT::MHAudioComponentDescriptor();
								break;
							default:
								pDescr = new MMT::UnimplMMTSIDescriptor();
							}

							if (pDescr->Unpack(descs_bs) >= 0)
							{
								if (peek_desc_tag == 0x8010 || peek_desc_tag == 0x8014)
									pDescr->Print(stdout, 40);

								if (left_descs_bytes < pDescr->descriptor_length + 3UL)
									break;

								left_descs_bytes -= pDescr->descriptor_length + 3UL;

								delete pDescr;
							}
							else
							{
								delete pDescr;
								break;
							}
						}
					}
				}
			}
		}
	}

	// Release all PLT and MPT table in CIDPAMsgs
	for (auto& v : CIDPAMsgs)
	{
		for (auto& t : v.second)
		{
			delete std::get<0>(t);
			auto& vmpt = std::get<1>(t);
			for (auto& m : vmpt)
				delete m;
		}
	}
	
	printf("\n");

	printf("The total number of TLV packets: %d.\n", nParsedTLVPackets);
	printf("The number of IPv4 TLV packets: %d.\n", nIPv4Packets);
	printf("The number of IPv6 TLV packets: %d.\n", nIPv6Packets);
	printf("The number of Header Compressed IP packets: %d.\n", nHdrCompressedIPPackets);
	printf("The number of Transmission Control Signal TLV packets: %d.\n", nTransmissionControlSignalPackets);
	printf("The number of Null TLV packets: %d.\n", nNullPackets);
	printf("The number of other TLV packets: %d.\n", nOtherPackets);

	return nRet;
}

int DumpMMTOneStream()
{
	int nRet = RET_CODE_SUCCESS;
	int nFilterTLVPackets = 0, nFilterMFUs = 0, nParsedTLVPackets = 0;
	TreeCIDPAMsgs CIDPAMsgs;
	std::vector<uint8_t> fullPAMessage;
	uint32_t asset_type = 0;
	char szLog[512] = { 0 };
	CESRepacker* pESRepacker[2] = { nullptr };

	auto start_time = std::chrono::high_resolution_clock::now();

	if (g_params.find("input") == g_params.end())
		return -1;

	const char* sp;
	const char* ep;
	int64_t i64Val = -1LL;
	int PIDN = 0;
	uint16_t src_packet_id[2];
	std::string Outputfiles[2];
	auto pidIter = g_params.find("pid");
	auto iterParam = g_params.find("output");
	if (pidIter != g_params.end())
	{
		int iterPID = (int)pidIter->second.find("&");
		if (iterPID == 0 || iterPID == std::string::npos)//Only one pid this case
		{
			sp = pidIter->second.c_str();
			ep = sp + pidIter->second.length();
			if (ConvertToInt((char*)sp, (char*)ep, i64Val) == false || i64Val < 0 || i64Val > UINT16_MAX)
			{
				printf("Please specify a valid packet_id.\n");
				return RET_CODE_ERROR;
			}
			src_packet_id[0] = (uint16_t)i64Val;
			PIDN = 0;
		}
		else//Two pids
		{
			std::string PIDs;
			PIDs = pidIter->second.substr(0, iterPID);
			sp = PIDs.c_str();
			ep = sp + PIDs.length();
			if (ConvertToInt((char*)sp, (char*)ep, i64Val) == false || i64Val < 0 || i64Val > UINT16_MAX)
			{
				printf("Please specify a valid packet_id.\n");
				return RET_CODE_ERROR;
			}
			src_packet_id[0] = (uint16_t)i64Val;
			if (iterParam != g_params.end())
			{
				Outputfiles[0] = iterParam->second;
				if (Outputfiles[0].rfind(".") == std::string::npos)// No dot in output file name
					Outputfiles[0] += ".";
				Outputfiles[1] = Outputfiles[0];
				Outputfiles[0].insert(Outputfiles[0].rfind("."), "_" + PIDs);
			}

			PIDs = pidIter->second.substr(iterPID + 1, pidIter->second.length());
			sp = PIDs.c_str();
			ep = sp + PIDs.length();
			if (ConvertToInt((char*)sp, (char*)ep, i64Val) == false || i64Val < 0 || i64Val > UINT16_MAX)
			{
				printf("Please specify a valid packet_id for the second pid.\n");
				return RET_CODE_ERROR;
			}
			src_packet_id[1] = (uint16_t)i64Val;
			if (iterParam != g_params.end())
				Outputfiles[1].insert(Outputfiles[1].rfind("."), "_" + PIDs);
			PIDN = 1;
		}
	}

	int filter_CID = -1;
	auto iterCID = g_params.find("CID");
	if (iterCID != g_params.end())
	{
		sp = iterCID->second.c_str();
		ep = sp + iterCID->second.length();
		if (ConvertToInt((char*)sp, (char*)ep, i64Val) == true && i64Val >= 0 && i64Val <= UINT16_MAX)
			filter_CID = (int)i64Val;
	}

	std::string& szInputFile = g_params["input"];
	bool bExplicitVideoStreamDump = g_params.find("video") == g_params.end() ? false : true;

	bool bListMMTPpacket = g_params.find("listMMTPpacket") != g_params.end();
	bool bListMMTPpayload = g_params.find("listMMTPpayload") != g_params.end();

	CFileBitstream bs(szInputFile.c_str(), 4096, &nRet);

	if (nRet < 0)
	{
		printf("Failed to open the file: %s.\n", szInputFile.c_str());
		return nRet;
	}

	int nLocateSyncTry = 0;
	bool bFindSync = false;
	try
	{
		uint32_t tlv_hdr = 0;
		uint64_t left_bits = 0;
		bs.Tell(&left_bits);

		while (left_bits >= (4ULL << 3))
		{
			tlv_hdr = (uint32_t)bs.PeekBits(32);
			if (((tlv_hdr >> 24) & 0xFF) != 0x7F)
			{
				bFindSync = false;
				if (nLocateSyncTry > 10)
				{
					printf("[MMT/TLV] TLV header should start with 0x7F at file position: %" PRIu64 ".\n", bs.Tell() >> 3);
					nRet = RET_CODE_BUFFER_NOT_COMPATIBLE;
					break;
				}
				else
				{
					bs.SkipBits(8);
					continue;
				}
			}
			else
			{
				if (bFindSync == false)
				{
					nLocateSyncTry++;
					bFindSync = true;
				}
			}

			MMT::TLVPacket* pTLVPacket = nullptr;
			switch ((tlv_hdr >> 16) & 0xFF)
			{
			case MMT::TLV_Header_compressed_IP_packet:
				pTLVPacket = new MMT::HeaderCompressedIPPacket();
				break;
			default:
				pTLVPacket = new MMT::Undefined_TLVPacket();
			}

			if ((nRet = pTLVPacket->Unpack(bs)) < 0)
			{
				printf("Failed to unpack a TLV packet at file position: %" PRIu64 ".\n", pTLVPacket->start_bitpos >> 3);
				delete pTLVPacket;
				break;
			}

			if (left_bits < (((uint64_t)pTLVPacket->Data_length + 4ULL) << 3))
			{
				delete pTLVPacket;
				break;
			}

			// Detect the PLT and MPT, and show them
			if (((tlv_hdr >> 16) & 0xFF) == MMT::TLV_Header_compressed_IP_packet)
			{
				bool bFiltered = false;
				MMT::HeaderCompressedIPPacket* pHeaderCompressedIPPacket = (MMT::HeaderCompressedIPPacket*)pTLVPacket;

				uint16_t CID = pHeaderCompressedIPPacket->Context_id;
				if (filter_CID >= 0 && filter_CID != CID)
					goto Skip;

				// Check whether it is a control message MMT packet
				if (pHeaderCompressedIPPacket->MMTP_Packet == nullptr || (
					pHeaderCompressedIPPacket->MMTP_Packet->Payload_type != 2 &&
					(pHeaderCompressedIPPacket->MMTP_Packet->Packet_id != src_packet_id[0]&&
					 pHeaderCompressedIPPacket->MMTP_Packet->Packet_id != src_packet_id[1])))
					goto Skip;

				if (pHeaderCompressedIPPacket->MMTP_Packet->Packet_id == src_packet_id[0] ||
					pHeaderCompressedIPPacket->MMTP_Packet->Packet_id == src_packet_id[1])
				{
					bFiltered = true;
					nFilterTLVPackets++;
				}

				if (bListMMTPpacket && bFiltered)
				{
					static int PktIndex = 0;
					size_t ccLog = sizeof(szLog) / sizeof(szLog[0]);
					int ccWritten = 0;
					int ccWrittenOnce = MBCSPRINTF_S(szLog, ccLog, "PktIndex:%10d", PktIndex++);
					if (ccWrittenOnce > 0)
						ccWritten += ccWrittenOnce;

					if (ccWrittenOnce > 0)
					{
						ccWrittenOnce = MBCSPRINTF_S(szLog + ccWritten, ccLog - ccWritten, ", RAP: %d", pHeaderCompressedIPPacket->MMTP_Packet->RAP_flag);

						if (ccWrittenOnce > 0)
							ccWritten += ccWrittenOnce;
					}

					if (ccWrittenOnce > 0)
					{
						ccWrittenOnce = MBCSPRINTF_S(szLog + ccWritten, ccLog - ccWritten, ", %14s", 
							pHeaderCompressedIPPacket->MMTP_Packet->Payload_type == 0?"fragment MPU":(
							pHeaderCompressedIPPacket->MMTP_Packet->Payload_type == 2?"message":"unsupported"));

						if (ccWrittenOnce > 0)
							ccWritten += ccWrittenOnce;
					}

					if (ccWrittenOnce > 0)
					{
						ccWrittenOnce = MBCSPRINTF_S(szLog + ccWritten, ccLog - ccWritten, ", pkt_id:0x%04X", pHeaderCompressedIPPacket->MMTP_Packet->Packet_id);

						if (ccWrittenOnce > 0)
							ccWritten += ccWrittenOnce;
					}

					if (ccWrittenOnce > 0)
					{
						ccWrittenOnce = MBCSPRINTF_S(szLog + ccWritten, ccLog - ccWritten, ", pkt_seq_no:0x%04X", pHeaderCompressedIPPacket->MMTP_Packet->Packet_sequence_number);

						if (ccWrittenOnce > 0)
							ccWritten += ccWrittenOnce;
					}

					printf("%s.\n", szLog);
				}

				if (pHeaderCompressedIPPacket->MMTP_Packet->Payload_type == 0)
				{
					// Create a new ES re-packer to repack the NAL unit to an Annex-B bitstream
					for (int i = 0; i <= PIDN; i++) {
						if (pESRepacker[i] == nullptr)
						{
							ES_REPACK_CONFIG config;
							memset(&config, 0, sizeof(config));

							ES_BYTE_STREAM_FORMAT dstESFmt = ES_BYTE_STREAM_RAW;
							if (IS_HEVC_STREAM(asset_type))
							{
								config.codec_id = CODEC_ID_V_MPEGH_HEVC;
								dstESFmt = ES_BYTE_STREAM_HEVC_ANNEXB;
							}
							else if (IS_AVC_STREAM(asset_type))
							{
								config.codec_id = CODEC_ID_V_MPEG4_AVC;
								dstESFmt = ES_BYTE_STREAM_AVC_ANNEXB;
							}
							else if (asset_type == 'mp4a')
								config.codec_id = CODEC_ID_A_MPEG4_AAC;
							else
							{
								if (bExplicitVideoStreamDump)
								{
									// skip it
									printf("Video stream(asset_type: %d), need find known assert type for video stream at first!!!!\n", asset_type);
									continue;
								}
							}
							//memset(config.es_output_file_path, 0, sizeof(config.es_output_file_path));
							strcpy_s(config.es_output_file_path, _countof(config.es_output_file_path), Outputfiles[i].c_str());

							pESRepacker[i] = new CESRepacker(ES_BYTE_STREAM_NALUNIT_WITH_LEN, dstESFmt);
							pESRepacker[i]->Config(config);
							pESRepacker[i]->Open(nullptr);
						}
					}

					if (pHeaderCompressedIPPacket->MMTP_Packet->Packet_id == src_packet_id[0])
					{
						PIDN = 0;
					}
					else
					{
						PIDN = 1;
					}

					for (auto& m : pHeaderCompressedIPPacket->MMTP_Packet->ptr_MPU->Data_Units)
					{
						uint8_t actual_fragmenttion_indicator = pHeaderCompressedIPPacket->MMTP_Packet->ptr_MPU->Aggregate_flag == 1 ? 
							0 : (uint8_t)pHeaderCompressedIPPacket->MMTP_Packet->ptr_MPU->fragmentation_indicator;

						ProcessMFU(CID,
							pHeaderCompressedIPPacket->MMTP_Packet->Packet_id,
							pHeaderCompressedIPPacket->MMTP_Packet->Payload_type,
							asset_type,
							actual_fragmenttion_indicator,
							&m.MFU_data_bytes[0], (int)m.MFU_data_bytes.size(), pESRepacker[PIDN]);

						if (m.MFU_data_bytes.size() > 0 && g_verbose_level == 99)
						{
							fprintf(stdout, MMT_FIX_HEADER_FMT_STR ": %s \n", "", "NAL Unit",
								actual_fragmenttion_indicator == 0 ? "Complete" : (
								actual_fragmenttion_indicator == 1 ? "First" : (
								actual_fragmenttion_indicator == 2 ? "Middle" : (
								actual_fragmenttion_indicator == 3 ? "Last" : "Unknown"))));
							for (size_t i = 0; i < m.MFU_data_bytes.size(); i++)
							{
								if (i % 32 == 0)
									fprintf(stdout, "\n" MMT_FIX_HEADER_FMT_STR "  ", "", "");
								fprintf(stdout, "%02X ", m.MFU_data_bytes[i]);
							}
							fprintf(stdout, "\n");
						}

						if (actual_fragmenttion_indicator || actual_fragmenttion_indicator == 3)
							nFilterMFUs++;
					}
				}
				else if (pHeaderCompressedIPPacket->MMTP_Packet->Payload_type == 0x2)
				{
					// Check there is an existed CID there or not, if it does NOT exist, add one
					if (CIDPAMsgs.find(CID) == CIDPAMsgs.end())
					{
						TreePLTMPT emptyTreePLTMAP;
						CIDPAMsgs[CID] = emptyTreePLTMAP;
					}

					// Check whether the current PA message contains a complete message
					if (pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->fragmentation_indicator == 0 ||
						pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->fragmentation_indicator == 3 ||
						pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->Aggregate_flag == 1)
					{
						if (fullPAMessage.size() > 0)
						{
							for (auto& m : pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->messages)
							{
								auto& v = std::get<1>(m);
								if (v.size() > 0)
								{
									size_t orig_size = fullPAMessage.size();
									fullPAMessage.resize(orig_size + v.size());
									memcpy(&fullPAMessage[orig_size], &v[0], v.size());
								}
							}

							bool bChanged = false;
							if (ProcessPAMessage(CIDPAMsgs, CID, pHeaderCompressedIPPacket->MMTP_Packet->Packet_id, &fullPAMessage[0], (int)fullPAMessage.size(), &bChanged) == 0 && bChanged)
							{
								uint32_t new_asset_type = FindAssetType(CIDPAMsgs, CID, src_packet_id[PIDN]);
								if (new_asset_type != 0 && new_asset_type != asset_type)
								{
									//printf("assert_type from 0x%X to 0x%X.\n", asset_type, new_asset_type);
									asset_type = new_asset_type;
								}
							}

							fullPAMessage.clear();
						}
						else
						{
							for (auto& m : pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->messages)
							{
								bool bChanged = false;
								auto& v = std::get<1>(m);
								if (ProcessPAMessage(CIDPAMsgs, CID, pHeaderCompressedIPPacket->MMTP_Packet->Packet_id, &v[0], (int)v.size(), &bChanged) == 0 && bChanged)
								{
									uint32_t new_asset_type = FindAssetType(CIDPAMsgs, CID, src_packet_id[PIDN]);
									if (new_asset_type != 0 && new_asset_type != asset_type)
									{
										//printf("assert_type from 0x%X to 0x%X.\n", asset_type, new_asset_type);
										asset_type = new_asset_type;
									}
								}
							}
						}
					}
					else
					{
						assert(pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->messages.size() <= 1);
						// Need add the data into the ring buffer, and parse it later
						for (auto& m : pHeaderCompressedIPPacket->MMTP_Packet->ptr_Messages->messages)
						{
							auto& v = std::get<1>(m);
							if (v.size() > 0)
							{
								size_t orig_size = fullPAMessage.size();
								fullPAMessage.resize(orig_size + v.size());
								memcpy(&fullPAMessage[orig_size], &v[0], v.size());
							}
						}
					}
				}
			}

		Skip:
			left_bits -= (((uint64_t)pTLVPacket->Data_length + 4ULL) << 3);
			delete pTLVPacket;

			nParsedTLVPackets++;
		}
	}
	catch (...)
	{
		nRet = RET_CODE_ERROR;
	}

	// Release all PLT and MPT table in CIDPAMsgs
	for (auto& v : CIDPAMsgs)
	{
		for (auto& t : v.second)
		{
			delete std::get<0>(t);
			auto& vmpt = std::get<1>(t);
			for (auto& m : vmpt)
				delete m;
		}
	}

	for (int i = 0; i <= 1; i++) {
		if (pESRepacker[i] != nullptr)
		{
			pESRepacker[i]->Flush();

			pESRepacker[i]->Close();
			delete pESRepacker[i];
			pESRepacker[i] = nullptr;
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();

	printf("Total cost: %f ms\n", std::chrono::duration<double, std::milli>(end_time - start_time).count());

	printf("Total input TLV packets: %d.\n", nParsedTLVPackets);
	printf("Total MMT/TLV packets with the packet_id(0X%X&0X%X): %d\n", src_packet_id[0], src_packet_id[1], nFilterTLVPackets);
	printf("Total MFUs in MMT/TLV packets with the packet_id(0X%X&0X%X): %d\n", src_packet_id[0], src_packet_id[1], nFilterMFUs);

	return nRet;
}

int DumpPartialMMT()
{
	printf("Not implemented!\n");
	return -1;
}

int RefactorMMT()
{
	printf("Not implemented!\n");
	return -1;
}

int DumpMMT()
{
	int nDumpRet = 0;

	auto iter_showpack = g_params.find("showpack");
	if ((iter_showpack) != g_params.end() ||
		(iter_showpack = g_params.find("showIPv4pack")) != g_params.end() ||
		(iter_showpack = g_params.find("showIPv6pack")) != g_params.end() ||
		(iter_showpack = g_params.find("showHCIPpack")) != g_params.end() ||
		(iter_showpack = g_params.find("showTCSpack")) != g_params.end())
	{
		int64_t display_pages = DEFAULT_TLV_PACKETS_PER_DISPLAY;
		const char* szPages = iter_showpack->second.c_str();
		if (ConvertToInt((char*)szPages, (char*)szPages + iter_showpack->second.length(), display_pages))
		{
			if (display_pages <= 0)
				g_TLV_packets_per_display = std::numeric_limits<decltype(g_TLV_packets_per_display)>::max();
			else
				g_TLV_packets_per_display = decltype(g_TLV_packets_per_display)(display_pages);
		}

		SHOW_TLV_PACK_OPTION option = SHOW_TLV_ALL;
		if (STRICMP(iter_showpack->first.c_str(), "showIPv4pack") == 0)
		{
			option = SHOW_TLV_IPv4;
		}
		else if (STRICMP(iter_showpack->first.c_str(), "showIPv6pack") == 0)
		{
			option = SHOW_TLV_IPv6;
		}
		else if (STRICMP(iter_showpack->first.c_str(), "showHCIPpack") == 0)
		{
			option = SHOW_TLV_HCIP;
		}
		else if (STRICMP(iter_showpack->first.c_str(), "showTCSpack") == 0)
		{
			option = SHOW_TLV_TCS;
		}

		return ShowMMTTLVPacks(option);
	}
	else if (g_params.find("pid") == g_params.end() || (NeedShowMMTTable() && g_params.find("output") == g_params.end()))
	{
		if (g_params.find("output") == g_params.end())
		{
			// No output file is specified
			// Check whether pid and showinfo is there
			if (g_params.find("showinfo") != g_params.end() ||
				g_params.find("showPLT") != g_params.end() ||
				g_params.find("showMPT") != g_params.end())
			{
				ShowMMTPackageInfo();
				goto done;
			}
			else
			{
				printf("Please specify a output file name with --output.\n");
				goto done;
			}
		}
	}
	else if (g_params.find("pid") != g_params.end())
	{
		if (g_params.find("outputfmt") == g_params.end())
			g_params["outputfmt"] = "es";

		std::string& str_output_fmt = g_params["outputfmt"];

		if ((str_output_fmt.compare("es") == 0 || str_output_fmt.compare("wav") == 0 || str_output_fmt.compare("pcm") == 0) || 
			g_params.find("listMMTPpacket") != g_params.end() ||
			g_params.find("listMMTPpayload") != g_params.end())
		{
			nDumpRet = DumpMMTOneStream();
		}
		else if (str_output_fmt.compare("mmt") == 0)
		{
			if (g_params.find("destpid") == g_params.end())
				nDumpRet = DumpPartialMMT();
			else
				nDumpRet = RefactorMMT();
		}
	}
	else
	{
		printf("Unsupported!\n");
	}

done:
	return nDumpRet;
}
