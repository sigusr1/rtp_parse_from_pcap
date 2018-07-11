#ifndef __RTP_PARSE__
#define __RTP_PARSE__

#include <stdio.h>
#include <map>
#include <string>

#include "multi_key.h"
#include "dynamic_ring_buffer.h"

#define __LITTLE_ENDIAN_BITFIELD

/* rtsp interleaved frame header struct */
typedef struct
{
    unsigned int magic : 8;     // $
    unsigned int channel : 8;   //0-1
    unsigned int rtp_len : 16;
}rtsp_interleaved_frame_hdr;


/*
 *
 *
 *    The RTP header has the following format:
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           synchronization source (SSRC) identifier            |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |            contributing source (CSRC) identifiers             |
 * |                             ....                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * RTP data header
 */

#define DISABLE_DAV_FILE

typedef struct {
#if defined(__BIG_ENDIAN_BITFIELD)
    unsigned int version:2;   /* protocol version */
    unsigned int p:1;         /* padding flag */
    unsigned int x:1;         /* header extension flag */
    unsigned int cc:4;        /* CSRC count */
    unsigned int m:1;         /* marker bit */
    unsigned int pt:7;        /* payload type */
#elif defined (__LITTLE_ENDIAN_BITFIELD)
    unsigned int cc:4;        /* CSRC count */
    unsigned int x:1;         /* header extension flag */
    unsigned int p:1;         /* padding flag */
    unsigned int version:2;   /* protocol version */
    unsigned int pt:7;        /* payload type */
    unsigned int m:1;         /* marker bit */
#else
#error  "Please choose endian"
#endif

    unsigned int seq:16;      /* sequence number */
    unsigned int ts;               /* timestamp */
    unsigned int ssrc;             /* synchronization source */
} rtp_hdr_t;

#define TOTAL_HEAD_LEN (sizeof(rtsp_interleaved_frame_hdr) + sizeof(rtp_hdr_t))

class rtp_parser
{
public:

	//src_ip and  dst_ip should be network byte order
	//src_port and dst_port should be  host byte order
	static rtp_parser* instance(unsigned int src_ip, unsigned short src_port, unsigned int dst_ip, unsigned short dst_port);
	//capture_time stand for when this data is captured
	void put_data(const char *payload, unsigned int len, struct timeval *capture_time);
private:
	
	//src_ip and  dst_ip should be network byte order
	//src_port and dst_port should be  host byte order
	rtp_parser(unsigned int src_ip, unsigned short src_port, unsigned int dst_ip, unsigned short dst_port);
	~rtp_parser();

	void search_frame(struct timeval *capture_time);
	//bool check_big_endian(); 

	//write frame info to file
	void write_frm_info();
	unsigned int time_diff(struct timeval *start, struct timeval *end);
private:

	typedef std::map< multi_key, rtp_parser* > stream_map;
	
	enum search_state
	{
		SEARCH_HDR,  //try search '$'
		BUILD_HDR,	 //have found '$', but the TOTAL_HEAD_LEN bytes head is not complete
		CHECK_HDR,	 //get a complete head, check if the head is valid
		BUILD_FRAME, //wait the whole frame to be completed
		CHECK_FRAME	 //check if the frame is valid
	};

	
	search_state state;

	static stream_map streams;
	dynamic_ring_buffer buffer;

	unsigned int rpt_pkt_len;	//current rtp packet len
	bool is_first_frm;
	bool is_file_created;
	FILE* dav_file;
	FILE* parse_file;
	std::string dav_file_name;
	std::string parse_file_name;

	struct timeval rtp_pkt_start_time;
	struct timeval rtp_pkt_end_time;

	struct timeval frm_start_time;
	struct timeval frm_end_time;

	struct timeval last_frm_end_time;

	bool wait_frame;
	unsigned int ssrc;
};

#endif
