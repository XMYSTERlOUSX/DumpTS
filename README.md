# Terminology
- **TS**: transport stream, the file extension is normally *.ts, .tts, .m2ts, .mts*
- **ISOBMFF**: ISO-based Media File Format, the file extension is normally *.mp4, .mov, m4a, m4v, .m4s*, .heif, .heic
- **Matroska**: a multimedia container format based on EBML (Extensible Binary Meta Language), the file extension is normally *.mkv, .mka, .mk3d and .webm*
- **MMT**: MPEG Media Transport stream, the file extension is normally *.mmts 

# What is DumpTS?
DumpTS is a simple utility tool to process the multimedia files packed into main-stream multimedia container formats, which provides these kinds of features:

- Extract and repack the elementary stream data or PSI sections data from *TS, ISOBMFF, Matroska and MMT/TLV* file
- Show media information of elementary streams, *ISOBMFF* box, *Matroska EBML* element and *MMT/TLV* packet/message/table/descriptors.
- Re-factor a *TS* stream file in place
- Extract some elementary streams, and reconstruct a partial *TS* file
- Provide some utility features for *ISOBMFF* file reconstruction
- Provide some utility functions for codec and container technology, for example, Huffman Codebook, CRC, container layout...


# How to build?

Windows:
Use VS2015/VS2017 to open DumpTS.sln to build it

Linux:
TODO...

# How to run it?

*Usage: DumpTS.exe MediaFileName \[OPTION\]...*

|Option|Value|Description|
|:--|:----:|:--|
|**--output**|*filename*|the output dumped file path|
|**--pid**|*0xhhhh*|the PID of dumped TS stream, or the packet_id of dumped MMT asset|
|**--trackid**|*xx*|the track ID of a ISOBMFF/Matroska file|
|**--CID**|*xx*|the context ID of a header compressed IP packet in MMT/TLV stream|
|**--destpid**|*0xhhhh*|the PID of source stream will be replaced with this PID|
|**--srcfmt**|*ts, m2ts, tts, <br>mp4, <br>mkv, <br>huffman_codebook, <br>aiff, <br>mmt*|the source media format, Including: ts, m2ts, mp4, mkv and huffman_codebook,if it is not specified, find the sync-word to decide it. <BR>BTW:<BR>**mp4**: <br>it is for the ISOBMFF, for example, .mov, .mp4, .m4s, .m4a, .heic, .heif...<BR>**mkv**:<br>it is for Matroska based file-format, for example, .mkv, .webm...<BR>**huffman_codebook:**<br>the VLC tables<BR>**aiff:**<br>AIFF or AIFF-C<br>**mmt:**<br>The MMT/TLV stream|
|**--outputfmt**|*ts, m2ts, <br>pes, <br>es, <br>wav, pcm, <br>binary_search_table*|the destination dumped format, including: ts, m2ts, pes, es and so on<br>**binary_search_table:**<br>generate the binary search table for Huffman VLC codebook|
|**--stream_id**|*0xhh*|the stream_id in PES header of dumped stream|
|**--stream_id_extension**|*0xhh*|the stream_id_extension in PES header of dumped stream|
|**--removebox**|*xxxx*|remove the box elements in MP4 file|
|**--boxtype**|*xxxx*|**For ISOBMFF/mp4 source:**<BR>the box type FOURCC, i.e. --boxtype=stsd<BR>**For Matroska/mkv source:**<BR>the EBML ID, i.e. --boxtype=0x1A45DFA3|
|**--crc**|*crc-type, all*|Specify the crc type, if crc type is not specified, list all crc types, if 'all' is specified, calculate all types of crc values|
|**--showinfo**|*N/A*|print the media information of elementary stream, for example, PMT stream types, stream type, audio sample rate, audio channel mapping, video resolution, frame-rate and so on|
|**--showpack**|*N/A*|Show packs in the specified TS/MMT/TLV stream file |
|**--showpts**|*N/A*|print the pts of every elementary stream packet|
|**--listcrc**|*N/A*|List all CRC types supported in this program|
|**--listmp4box**|*N/A*|List box types and descriptions defined in ISO-14496 spec|
|**--listmkvEBML**|*N/A*|List EBML elements defined in Matroska spec|
|**--dashinitmp4**|*filename*|the initialization MP4 file to describe the DASH stream global information|
|**--VLCTypes**|*[ahdob][ahdob][ahdob]*|Specify the number value literal formats, a: auto; h: hex; d: dec; o: oct; b: bin, for example, "aah" means:<br>Value and length will be parsed according to literal string, codeword will be parsed according as hexadecimal|
|**--verbose**|*0~n*|print more message in the intermediate process|
 
Here are some examples of command lines:  
```
DumpTS c:\00001.m2ts --showinfo
```
it will show the PAT and PMT informations of 00001.m2ts

```
DumpTS c:\00001.m2ts --output=c:\00001.hevc --pid=0x1011 --srcfmt=m2ts --outputfmt=es --showpts  
```
It will dump a hevc es stream with PID 0x1011 from the m2ts stream file: c:\00001.m2ts, and print the PTS of every frame.

```
DumpTS C:\test.ts --output=c:\00001.m2ts --pid=0x100 --destpid=0x1011 --srcfmt=ts --outputfmt=m2ts  
```
It will re-factor the file: c:\test.ts, and replace the PID 0x100 with 0x1011 in TS pack and PSI, and convert it to a m2ts

```
DumpTS C:\00022.m2ts --output=c:\00022.mlp --pid=0x1100 --srcfmt=m2ts --outputfmt=es 
--stream_id_extension=0x72  
```
It will dump a MLP sub-stream from C:\00022.m2ts with the PID 0x1100 and stream\_id\_extension in PES: 0x72
```
DumpTS C:\test.mp4 --showinfo --removebox='unkn'
```
Show the MP4 file box layout, and remove box with type 'unkn'
```
DumpTS C:\tes.mp4 --output=c:\test.hevc --trackid=1 --outputfmt=es
```
Dump the track#1 of test.mp4, and save its elementary stream data to file test.hevc, the VSP, SPS and PPS will be merged into elementary stream data.
```
DumpTS C:\test.mp4 --trackid=1 --boxtype=stsd --showinfo
```
Show the 'stsd' box information, for example, HEVC/AVC resolution, chroma, bit-depth and so on
```
DumpTS C:\av1.webm --showinfo
```
Show the tree view for the EBML elements of av1.webm
```
DumpTS e:\tearsofsteel_4sec0025_3840x2160.y4m-20000.av1.webm --trackid=1 --output=e:\tearsofsteel_4sec0025_4K.av1
```
Extract av1 video stream from .webm file
```
DumpTS e:\00301.mmts --showinfo
```
Show the MMT payload information, including PLT, MPT and asset/elementary information
```
DumpTS e:\00301.mmts --showpack
```
Show the detailed information for MMT packets, payloads, messages, tables and descriptors
```
DumpTS e:\00301.mmts --CID=0 --pid=0x100 --output=e:\00301.hevc
```
Extract HEVC stream from header compressed IP packet with context_id: 0 and MMT packet id: 0x100 from 00301.mmts
```
dumpts AACScalefactorHuffmanCodebook.txt --VLCTypes=aah --showinfo --srcfmt=huffman_codebook
```
Load huffman-codebook from the specified file, and print its huffman-tree
```
dumpts AACScalefactorHuffmanCodebook.txt --VLCTypes=aah --srcfmt=huffman_codebook --outputfmt=binary_search_table
```
Load huffman-codebook from the specified file, and print binary search table for huffman-tree