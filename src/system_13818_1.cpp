#include "StdAfx.h"

const char* stream_type_names[256] = {
/*0x00*/	"ITU-T | ISO/IEC Reserved",
/*0x01*/	"ISO/IEC 11172-2 Video",
/*0x02*/	"Rec. ITU-T H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream",
/*0x03*/	"ISO/IEC 11172-3 Audio",
/*0x04*/	"ISO/IEC 13818-3 Audio",
/*0x05*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 private_sections",
/*0x06*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 PES packets containing private data",
/*0x07*/	"ISO/IEC 13522 MHEG",
/*0x08*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Annex A DSM-CC",
/*0x09*/	"Rec. ITU-T H.222.1",
/*0x0A*/	"ISO/IEC 13818-6 type A",
/*0x0B*/	"ISO/IEC 13818-6 type B",
/*0x0C*/	"ISO/IEC 13818-6 type C",
/*0x0D*/	"ISO/IEC 13818-6 type D",
/*0x0E*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 auxiliary",
/*0x0F*/	"ISO/IEC 13818-7 Audio with ADTS transport syntax",
/*0x10*/	"ISO/IEC 14496-2 Visual",
/*0x11*/	"ISO/IEC 14496-3 Audio with the LATM transport syntax as defined in ISO/IEC 14496-3",
/*0x12*/	"ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets",
/*0x13*/	"ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC 14496_sections",
/*0x14*/	"ISO/IEC 13818-6 Synchronized Download Protocol",
/*0x15*/	"Metadata carried in PES packets",
/*0x16*/	"Metadata carried in metadata_sections",
/*0x17*/	"Metadata carried in ISO/IEC 13818-6 Data Carousel",
/*0x18*/	"Metadata carried in ISO/IEC 13818-6 Object Carousel",
/*0x19*/	"Metadata carried in ISO/IEC 13818-6 Synchronized Download Protocol",
/*0x1A*/	"IPMP stream (defined in ISO/IEC 13818-11, MPEG-2 IPMP)",
/*0x1B*/	"AVC video stream conforming to one or more profiles defined in Annex A of Rec. ITU-T H.264 | ISO/IEC 14496-10 or AVC video sub-bitstream of SVC as defined in 2.1.78 or MVC base view sub-bitstream, as defined in 2.1.85, or AVC video sub-bitstream of MVC, as defined in 2.1.88",
/*0x1C*/	"ISO/IEC 14496-3 Audio, without using any additional transport syntax, such as DST, ALS and SLS",
/*0x1D*/	"ISO/IEC 14496-17 Text",
/*0x1E*/	"Auxiliary video stream as defined in ISO/IEC 23002-3",
/*0x1F*/	"SVC video sub-bitstream of an AVC video stream conforming to one or more profiles defined in Annex G of Rec. ITU-T H.264 | ISO/IEC 14496-10",
/*0x20*/	"MVC video sub-bitstream of an AVC video stream conforming to one or more profiles defined in Annex H of Rec. ITU-T H.264 | ISO/IEC 14496-10",
/*0x21*/	"Video stream conforming to one or more profiles as defined in Rec. ITU-T T.800 | ISO/IEC 15444-1",
/*0x22*/	"Additional view Rec. ITU-T H.262 | ISO/IEC 13818-2 video stream for service-compatible stereoscopic 3D services (see Notes 3 and 4)",
/*0x23*/	"Additional view Rec. ITU-T H.264 | ISO/IEC 14496-10 video stream conforming to one or more profiles defined in Annex A for service-compatible stereoscopic 3D services (see Notes 3 and 4)",
/*0x24*/	"HEVC video stream or an HEVC temporal video sub-bitstream",
/*0x25*/	"HEVC temporal video subset of an HEVC video stream conforming to one or more profiles defined in Annex A of Rec.ITU - T H.265 | ISO / IEC 23008 - 2",
/*0x26*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x27*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x28*/	"HEVC enhancement sub-partition which includes TemporalId 0 of an HEVC video stream where all NALs units contained in the stream conform to one or more profiles defined in Annex G of Rec. ITU-T H.265 | ISO/IEC 23008-2",
/*0x29*/	"HEVC temporal enhancement sub-partition of an HEVC video stream where all NAL units contained in the stream conform to one or more profiles defined in Annex G of Rec. ITU-T H.265 | ISO/IEC 23008-2",
/*0x2a*/	"HEVC enhancement sub-partition which includes TemporalId 0 of an HEVC video stream where all NAL units contained in the stream conform to one or more profiles defined in Annex H of Rec. ITU-T H.265 | ISO/IEC 23008-2",
/*0x2b*/	"HEVC temporal enhancement sub-partition of an HEVC video stream where all NAL units contained in the stream conform to one or more profiles defined in Annex H of Rec. ITU-T H.265 | ISO/IEC 23008-2",
/*0x2c*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x2d*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x2e*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x2f*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x30*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x31*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x32*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x33*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x34*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x35*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x36*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x37*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x38*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x39*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x3a*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x3b*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x3c*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x3d*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x3e*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x3f*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x40*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x41*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x42*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x43*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x44*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x45*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x46*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x47*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x48*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x49*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x4a*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x4b*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x4c*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x4d*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x4e*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x4f*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x50*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x51*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x52*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x53*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x54*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x55*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x56*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x57*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x58*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x59*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x5a*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x5b*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x5c*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x5d*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x5e*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x5f*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x60*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x61*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x62*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x63*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x64*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x65*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x66*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x67*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x68*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x69*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x6a*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x6b*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x6c*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x6d*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x6e*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x6f*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x70*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x71*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x72*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x73*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x74*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x75*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x76*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x77*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x78*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x79*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x7a*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x7b*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x7c*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x7d*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x7e*/	"Rec. ITU-T H.222.0 | ISO/IEC 13818-1 Reserved",
/*0x7F*/	"IPMP stream",
/*0x80*/	"User Private",
/*0x81*/	"User Private",
/*0x82*/	"User Private",
/*0x83*/	"User Private",
/*0x84*/	"User Private",
/*0x85*/	"User Private",
/*0x86*/	"User Private",
/*0x87*/	"User Private",
/*0x88*/	"User Private",
/*0x89*/	"User Private",
/*0x8a*/	"User Private",
/*0x8b*/	"User Private",
/*0x8c*/	"User Private",
/*0x8d*/	"User Private",
/*0x8e*/	"User Private",
/*0x8f*/	"User Private",
/*0x90*/	"User Private",
/*0x91*/	"User Private",
/*0x92*/	"User Private",
/*0x93*/	"User Private",
/*0x94*/	"User Private",
/*0x95*/	"User Private",
/*0x96*/	"User Private",
/*0x97*/	"User Private",
/*0x98*/	"User Private",
/*0x99*/	"User Private",
/*0x9a*/	"User Private",
/*0x9b*/	"User Private",
/*0x9c*/	"User Private",
/*0x9d*/	"User Private",
/*0x9e*/	"User Private",
/*0x9f*/	"User Private",
/*0xa0*/	"User Private",
/*0xa1*/	"User Private",
/*0xa2*/	"User Private",
/*0xa3*/	"User Private",
/*0xa4*/	"User Private",
/*0xa5*/	"User Private",
/*0xa6*/	"User Private",
/*0xa7*/	"User Private",
/*0xa8*/	"User Private",
/*0xa9*/	"User Private",
/*0xaa*/	"User Private",
/*0xab*/	"User Private",
/*0xac*/	"User Private",
/*0xad*/	"User Private",
/*0xae*/	"User Private",
/*0xaf*/	"User Private",
/*0xb0*/	"User Private",
/*0xb1*/	"User Private",
/*0xb2*/	"User Private",
/*0xb3*/	"User Private",
/*0xb4*/	"User Private",
/*0xb5*/	"User Private",
/*0xb6*/	"User Private",
/*0xb7*/	"User Private",
/*0xb8*/	"User Private",
/*0xb9*/	"User Private",
/*0xba*/	"User Private",
/*0xbb*/	"User Private",
/*0xbc*/	"User Private",
/*0xbd*/	"User Private",
/*0xbe*/	"User Private",
/*0xbf*/	"User Private",
/*0xc0*/	"User Private",
/*0xc1*/	"User Private",
/*0xc2*/	"User Private",
/*0xc3*/	"User Private",
/*0xc4*/	"User Private",
/*0xc5*/	"User Private",
/*0xc6*/	"User Private",
/*0xc7*/	"User Private",
/*0xc8*/	"User Private",
/*0xc9*/	"User Private",
/*0xca*/	"User Private",
/*0xcb*/	"User Private",
/*0xcc*/	"User Private",
/*0xcd*/	"User Private",
/*0xce*/	"User Private",
/*0xcf*/	"User Private",
/*0xd0*/	"User Private",
/*0xd1*/	"User Private",
/*0xd2*/	"User Private",
/*0xd3*/	"User Private",
/*0xd4*/	"User Private",
/*0xd5*/	"User Private",
/*0xd6*/	"User Private",
/*0xd7*/	"User Private",
/*0xd8*/	"User Private",
/*0xd9*/	"User Private",
/*0xda*/	"User Private",
/*0xdb*/	"User Private",
/*0xdc*/	"User Private",
/*0xdd*/	"User Private",
/*0xde*/	"User Private",
/*0xdf*/	"User Private",
/*0xe0*/	"User Private",
/*0xe1*/	"User Private",
/*0xe2*/	"User Private",
/*0xe3*/	"User Private",
/*0xe4*/	"User Private",
/*0xe5*/	"User Private",
/*0xe6*/	"User Private",
/*0xe7*/	"User Private",
/*0xe8*/	"User Private",
/*0xe9*/	"User Private",
/*0xea*/	"User Private",
/*0xeb*/	"User Private",
/*0xec*/	"User Private",
/*0xed*/	"User Private",
/*0xee*/	"User Private",
/*0xef*/	"User Private",
/*0xf0*/	"User Private",
/*0xf1*/	"User Private",
/*0xf2*/	"User Private",
/*0xf3*/	"User Private",
/*0xf4*/	"User Private",
/*0xf5*/	"User Private",
/*0xf6*/	"User Private",
/*0xf7*/	"User Private",
/*0xf8*/	"User Private",
/*0xf9*/	"User Private",
/*0xfa*/	"User Private",
/*0xfb*/	"User Private",
/*0xfc*/	"User Private",
/*0xfd*/	"User Private",
/*0xfe*/	"User Private",
/*0xff*/	"User Private"
};

const char* table_id_names[256]={
/*0x00*/ "program_association_section",
/*0x01*/ "conditional_access_section (CA_section)",
/*0x02*/ "TS_program_map_section",
/*0x03*/ "TS_description_section",
/*0x04*/ "ISO_IEC_14496_scene_description_section",
/*0x05*/ "ISO_IEC_14496_object_descriptor_section",
/*0x06*/ "Metadata_section",
/*0x07*/ "IPMP_Control_Information_section (defined in ISO/IEC 13818-11)",
/*0x08*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x09*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x0a*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x0b*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x0c*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x0d*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x0e*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x0f*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x10*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x11*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x12*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x13*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x14*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x15*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x16*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x17*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x18*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x19*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x1a*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x1b*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x1c*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x1d*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x1e*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x1f*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x20*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x21*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x22*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x23*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x24*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x25*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x26*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x27*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x28*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x29*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x2a*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x2b*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x2c*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x2d*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x2e*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x2f*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x30*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x31*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x32*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x33*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x34*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x35*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x36*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x37*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x38*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x39*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x3a*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x3b*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x3c*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x3d*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x3e*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x3f*/ "Rec. ITU-T H.222.0 | ISO/IEC 13818-1 reserved",
/*0x40*/ "User private",
/*0x41*/ "User private",
/*0x42*/ "User private",
/*0x43*/ "User private",
/*0x44*/ "User private",
/*0x45*/ "User private",
/*0x46*/ "User private",
/*0x47*/ "User private",
/*0x48*/ "User private",
/*0x49*/ "User private",
/*0x4a*/ "User private",
/*0x4b*/ "User private",
/*0x4c*/ "User private",
/*0x4d*/ "User private",
/*0x4e*/ "User private",
/*0x4f*/ "User private",
/*0x50*/ "User private",
/*0x51*/ "User private",
/*0x52*/ "User private",
/*0x53*/ "User private",
/*0x54*/ "User private",
/*0x55*/ "User private",
/*0x56*/ "User private",
/*0x57*/ "User private",
/*0x58*/ "User private",
/*0x59*/ "User private",
/*0x5a*/ "User private",
/*0x5b*/ "User private",
/*0x5c*/ "User private",
/*0x5d*/ "User private",
/*0x5e*/ "User private",
/*0x5f*/ "User private",
/*0x60*/ "User private",
/*0x61*/ "User private",
/*0x62*/ "User private",
/*0x63*/ "User private",
/*0x64*/ "User private",
/*0x65*/ "User private",
/*0x66*/ "User private",
/*0x67*/ "User private",
/*0x68*/ "User private",
/*0x69*/ "User private",
/*0x6a*/ "User private",
/*0x6b*/ "User private",
/*0x6c*/ "User private",
/*0x6d*/ "User private",
/*0x6e*/ "User private",
/*0x6f*/ "User private",
/*0x70*/ "User private",
/*0x71*/ "User private",
/*0x72*/ "User private",
/*0x73*/ "User private",
/*0x74*/ "User private",
/*0x75*/ "User private",
/*0x76*/ "User private",
/*0x77*/ "User private",
/*0x78*/ "User private",
/*0x79*/ "User private",
/*0x7a*/ "User private",
/*0x7b*/ "User private",
/*0x7c*/ "User private",
/*0x7d*/ "User private",
/*0x7e*/ "User private",
/*0x7f*/ "User private",
/*0x80*/ "User private",
/*0x81*/ "User private",
/*0x82*/ "User private",
/*0x83*/ "User private",
/*0x84*/ "User private",
/*0x85*/ "User private",
/*0x86*/ "User private",
/*0x87*/ "User private",
/*0x88*/ "User private",
/*0x89*/ "User private",
/*0x8a*/ "User private",
/*0x8b*/ "User private",
/*0x8c*/ "User private",
/*0x8d*/ "User private",
/*0x8e*/ "User private",
/*0x8f*/ "User private",
/*0x90*/ "User private",
/*0x91*/ "User private",
/*0x92*/ "User private",
/*0x93*/ "User private",
/*0x94*/ "User private",
/*0x95*/ "User private",
/*0x96*/ "User private",
/*0x97*/ "User private",
/*0x98*/ "User private",
/*0x99*/ "User private",
/*0x9a*/ "User private",
/*0x9b*/ "User private",
/*0x9c*/ "User private",
/*0x9d*/ "User private",
/*0x9e*/ "User private",
/*0x9f*/ "User private",
/*0xa0*/ "User private",
/*0xa1*/ "User private",
/*0xa2*/ "User private",
/*0xa3*/ "User private",
/*0xa4*/ "User private",
/*0xa5*/ "User private",
/*0xa6*/ "User private",
/*0xa7*/ "User private",
/*0xa8*/ "User private",
/*0xa9*/ "User private",
/*0xaa*/ "User private",
/*0xab*/ "User private",
/*0xac*/ "User private",
/*0xad*/ "User private",
/*0xae*/ "User private",
/*0xaf*/ "User private",
/*0xb0*/ "User private",
/*0xb1*/ "User private",
/*0xb2*/ "User private",
/*0xb3*/ "User private",
/*0xb4*/ "User private",
/*0xb5*/ "User private",
/*0xb6*/ "User private",
/*0xb7*/ "User private",
/*0xb8*/ "User private",
/*0xb9*/ "User private",
/*0xba*/ "User private",
/*0xbb*/ "User private",
/*0xbc*/ "User private",
/*0xbd*/ "User private",
/*0xbe*/ "User private",
/*0xbf*/ "User private",
/*0xc0*/ "User private",
/*0xc1*/ "User private",
/*0xc2*/ "User private",
/*0xc3*/ "User private",
/*0xc4*/ "User private",
/*0xc5*/ "User private",
/*0xc6*/ "User private",
/*0xc7*/ "User private",
/*0xc8*/ "User private",
/*0xc9*/ "User private",
/*0xca*/ "User private",
/*0xcb*/ "User private",
/*0xcc*/ "User private",
/*0xcd*/ "User private",
/*0xce*/ "User private",
/*0xcf*/ "User private",
/*0xd0*/ "User private",
/*0xd1*/ "User private",
/*0xd2*/ "User private",
/*0xd3*/ "User private",
/*0xd4*/ "User private",
/*0xd5*/ "User private",
/*0xd6*/ "User private",
/*0xd7*/ "User private",
/*0xd8*/ "User private",
/*0xd9*/ "User private",
/*0xda*/ "User private",
/*0xdb*/ "User private",
/*0xdc*/ "User private",
/*0xdd*/ "User private",
/*0xde*/ "User private",
/*0xdf*/ "User private",
/*0xe0*/ "User private",
/*0xe1*/ "User private",
/*0xe2*/ "User private",
/*0xe3*/ "User private",
/*0xe4*/ "User private",
/*0xe5*/ "User private",
/*0xe6*/ "User private",
/*0xe7*/ "User private",
/*0xe8*/ "User private",
/*0xe9*/ "User private",
/*0xea*/ "User private",
/*0xeb*/ "User private",
/*0xec*/ "User private",
/*0xed*/ "User private",
/*0xee*/ "User private",
/*0xef*/ "User private",
/*0xf0*/ "User private",
/*0xf1*/ "User private",
/*0xf2*/ "User private",
/*0xf3*/ "User private",
/*0xf4*/ "User private",
/*0xf5*/ "User private",
/*0xf6*/ "User private",
/*0xf7*/ "User private",
/*0xf8*/ "User private",
/*0xf9*/ "User private",
/*0xfa*/ "User private",
/*0xfb*/ "User private",
/*0xfc*/ "User private",
/*0xfd*/ "User private",
/*0xfe*/ "User private",
/*0xFF*/ "Forbidden",
};

const char* frame_rate_names[16] = {
	/* 0*/ "reserved",
	/* 1*/ "24 000/1001 (23.976...)",
	/* 2*/ "24",
	/* 3*/ "25",
	/* 4*/ "30 000/1001 (29.97...)",
	/* 5*/ "30",
	/* 6*/ "50",
	/* 7*/ "60 000/1001 (59.94...)",
	/* 8*/ "reserved",
	/* 9*/ "reserved",
	/*10*/ "reserved",
	/*11*/ "reserved",
	/*12*/ "reserved",
	/*13*/ "reserved",
	/*14*/ "reserved",
	/*15*/ "No information"
};

const char* video_format_names[16] = {
	/* 0*/ "reserved",
	/* 1*/ "480i ITU-R BT.601-5",
	/* 2*/ "576i ITU-R BT.601-5",
	/* 3*/ "480p SMPTE 293M",
	/* 4*/ "1080i SMPTE 274M",
	/* 5*/ "720p SMPTE 296M",
	/* 6*/ "1080p SMPTE 274M",
	/* 7*/ "576p ITU-R BT.1358",
	/* 8*/ "2160p SMPTE ST 2036",
	/* 9*/ "540p ETSI TS 101 154",
	/*10*/ "900p ETSI TS 101 154",
	/*11*/ "4320p SMPTE ST 2036",
	/*12*/ "reserved",
	/*13*/ "reserved",
	/*14*/ "reserved",
	/*15*/ "No information"
};

const char* aspect_ratio_names[16] = {
	/* 0*/ "reserved",
	/* 1*/ "reserved",
	/* 2*/ "4:3 display aspect ratio",
	/* 3*/ "16:9 display aspect ratio",
	/* 4*/ "reserved",
	/* 5*/ "reserved",
	/* 6*/ "reserved",
	/* 7*/ "reserved",
	/* 8*/ "reserved",
	/* 9*/ "reserved",
	/*10*/ "reserved",
	/*11*/ "reserved",
	/*12*/ "reserved",
	/*13*/ "reserved",
	/*14*/ "reserved",
	/*15*/ "No information"
};

const char* bit_depth_names[16] = {
	/* 0*/ "reserved",
	/* 1*/ "8 bits",
	/* 2*/ "10 bits",
	/* 3*/ "reserved",
	/* 4*/ "reserved",
	/* 5*/ "reserved",
	/* 6*/ "reserved",
	/* 7*/ "reserved",
	/* 8*/ "reserved",
	/* 9*/ "reserved",
	/*10*/ "reserved",
	/*11*/ "reserved",
	/*12*/ "reserved",
	/*13*/ "reserved",
	/*14*/ "reserved",
	/*15*/ "No information"
};

const char* dynamic_range_type_names[16] = {
	/* 0*/ "SDR",
	/* 1*/ "ITU-R BT.2100 Perceptual Quantization",
	/* 2*/ "reserved",
	/* 3*/ "reserved",
	/* 4*/ "ITU-R BT.2100 Hybrid Log-Gamma",
	/* 5*/ "reserved",
	/* 6*/ "reserved",
	/* 7*/ "reserved",
	/* 8*/ "reserved",
	/* 9*/ "reserved",
	/*10*/ "reserved",
	/*11*/ "reserved",
	/*12*/ "reserved",
	/*13*/ "reserved",
	/*14*/ "reserved",
	/*15*/ "No information"
};

const char* color_space_names[16] = {
	/* 0*/ "No information",
	/* 1*/ "BT.601",
	/* 2*/ "BT.709",
	/* 3*/ "BT.2020",
	/* 4*/ "reserved",
	/* 5*/ "reserved",
	/* 6*/ "reserved",
	/* 7*/ "reserved",
	/* 8*/ "reserved",
	/* 9*/ "reserved",
	/*10*/ "reserved",
	/*11*/ "reserved",
	/*12*/ "reserved",
	/*13*/ "reserved",
	/*14*/ "reserved",
	/*15*/ "No information"
};

const char* MPEG4_audio_profile_and_level_names[256] = {
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"No audio profile and level defined for the associated MPEG-4 audio stream",
	"Main profile, level 1",
	"Main profile, level 2",
	"Main profile, level 3",
	"Main profile, level 4",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Scalable Profile, level 1",
	"Scalable Profile, level 2",
	"Scalable Profile, level 3",
	"Scalable Profile, level 4",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Speech profile, level 1",
	"Speech profile, level 2",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Synthesis profile, level 1",
	"Synthesis profile, level 2",
	"Synthesis profile, level 3",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"High quality audio profile, level 1",
	"High quality audio profile, level 2",
	"High quality audio profile, level 3",
	"High quality audio profile, level 4",
	"High quality audio profile, level 5",
	"High quality audio profile, level 6",
	"High quality audio profile, level 7",
	"High quality audio profile, level 8",
	"Low delay audio profile, level 1",
	"Low delay audio profile, level 2",
	"Low delay audio profile, level 3",
	"Low delay audio profile, level 4",
	"Low delay audio profile, level 5",
	"Low delay audio profile, level 6",
	"Low delay audio profile, level 7",
	"Low delay audio profile, level 8",
	"Natural audio profile, level 1",
	"Natural audio profile, level 2",
	"Natural audio profile, level 3",
	"Natural audio profile, level 4",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Mobile audio internetworking profile, level 1",
	"Mobile audio internetworking profile, level 2",
	"Mobile audio internetworking profile, level 3",
	"Mobile audio internetworking profile, level 4",
	"Mobile audio internetworking profile, level 5",
	"Mobile audio internetworking profile, level 6",
	"Reserved",
	"Reserved",
	"AAC profile, level 1",
	"AAC profile, level 2",
	"AAC profile, level 4",
	"AAC profile, level 5",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"High efficiency AAC profile, level 2",
	"High efficiency AAC profile, level 3",
	"High efficiency AAC profile, level 4",
	"High efficiency AAC profile, level 5",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"High efficiency AAC v2 profile, level 2",
	"High efficiency AAC v2 profile, level 3",
	"High efficiency AAC v2 profile, level 4",
	"High efficiency AAC v2 profile, level 5",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Audio profile and level not specified by the MPEG-4_audio_profile_and_level field in this descriptor"
};

