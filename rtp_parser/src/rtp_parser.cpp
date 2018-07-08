#include <stdlib.h>
#include <arpa/inet.h>

#include "rtp_parser.h"

rtp_parser::stream_map rtp_parser::streams;
rtp_parser* rtp_parser::instance(unsigned int src_ip, unsigned short src_port, unsigned int dst_ip, unsigned short dst_port)
{
	multi_key key(src_ip, src_port, dst_ip, dst_port);
	stream_map::iterator it;

	it = streams.find(key);

	if( it == streams.end())
	{
		rtp_parser *new_stream = new rtp_parser(src_ip, src_port, dst_ip, dst_port);
		streams[key] = new_stream;
		return new_stream;
	}
	else
	{
		return it->second;
	}
}

rtp_parser::rtp_parser(unsigned int src_ip, unsigned short src_port, unsigned int dst_ip, unsigned short dst_port)
	: buffer(1024), 
	state(SEARCH_HDR), 
	rpt_pkt_len(0), 
	is_file_created(false), 
	dav_file(NULL), 
	parse_file(NULL),
	is_first_frm(true),
	wait_frame(true),
	ssrc(0)
{
	buffer.kmp_init("$");
	//is_big_endian = check_big_endian();

	struct in_addr src;
	struct in_addr dst;

	char src_port_str[10];
	char dst_port_str[10];

	src.s_addr = src_ip;
	dst.s_addr = dst_ip;

	snprintf(src_port_str, sizeof(src_port_str), "%u", src_port);
	snprintf(dst_port_str, sizeof(dst_port_str), "%u", dst_port);
	
	dav_file_name =  "src[" + std::string(inet_ntoa(src)) + "[" + src_port_str + "]]--dst[" + std::string(inet_ntoa(dst)) + "[" + dst_port_str + "]].dav";
	parse_file_name =  "src[" + std::string(inet_ntoa(src)) + "[" + src_port_str + "]]--dst[" + std::string(inet_ntoa(dst)) + "[" + dst_port_str + "]].txt";

}

rtp_parser::~rtp_parser()
{
	if (NULL != dav_file)
	{
		fclose(dav_file);
	}
	
	if (NULL != parse_file)
	{
		fclose(parse_file);
	}
}
void rtp_parser::put_data(const char *payload, unsigned int len, struct timeval *capture_time)
{
	buffer.write_data(payload, len);
	search_frame(capture_time);
}

void rtp_parser::search_frame(struct timeval *capture_time)
{
	int ret;
	bool frm_end = false;

	switch(state)
	{
		case SEARCH_HDR:
			
			ret = buffer.kmp_search_ring_buffer();
			if ( ret == -1)
			{//head not find
				buffer.drop_data(buffer.get_data_size());//drop all invalid data
				return;	//wait next packet
			}
			else
			{// find head
				buffer.drop_data(ret);	//drop no use data, the frame head is in the front of buffer
				state = BUILD_HDR;
				rtp_pkt_start_time = *capture_time;

				if (wait_frame)
				{
					frm_start_time = *capture_time;
					wait_frame = false;
				}
			}
			
			break;
		case BUILD_HDR:
			if (buffer.get_data_size() < TOTAL_HEAD_LEN)
			{
				return;	//wait next packet
			}
			else
			{
				state = CHECK_HDR;			
			}
			
			break;
		case CHECK_HDR:

			//todo convert buffer to rtp_hdr_t struct
			if (buffer.parse_as_char(1) != 0)
            {
				buffer.drop_data(1);
				state = SEARCH_HDR;
				break;
            }

			if (ssrc == 0)
			{
				ssrc = buffer.parse_as_uint(12);
				COMMON_LOG("ssrc:%x", ssrc);
				COMMON_LOG("ssrc:%x  %x %x %x ", buffer.parse_as_char(0), buffer.parse_as_char(1), buffer.parse_as_char(2), buffer.parse_as_char(3));
				COMMON_LOG("ssrc:%x  %x %x %x ", buffer.parse_as_char(4), buffer.parse_as_char(5), buffer.parse_as_char(6), buffer.parse_as_char(7));
				COMMON_LOG("ssrc:%x  %x %x %x ", buffer.parse_as_char(8), buffer.parse_as_char(9), buffer.parse_as_char(10), buffer.parse_as_char(11));
				COMMON_LOG("ssrc:%x  %x %x %x ", buffer.parse_as_char(12), buffer.parse_as_char(13), buffer.parse_as_char(14), buffer.parse_as_char(15));
			}
			else if (ssrc != buffer.parse_as_uint(12))
			{
				buffer.drop_data(1);
				state = SEARCH_HDR;
				break;
			}
			rpt_pkt_len = buffer.parse_as_ushort(2);
			state = BUILD_FRAME;			
			
			break;
		case BUILD_FRAME:
			if (buffer.get_data_size() < rpt_pkt_len)
			{
				return; //wait the frame to be completed
			}
			else
			{// whole frame arrive
				
				rtp_pkt_end_time = *capture_time;
				if (!is_file_created)
				{
					dav_file = fopen(dav_file_name.c_str(), "wb");
					if ( dav_file == NULL )
					{
						COMMON_LOG("create file[%s] failed!",dav_file_name.c_str());
						exit(1);
					}

					parse_file = fopen(parse_file_name.c_str(), "wb");
					if ( parse_file == NULL )
					{
						COMMON_LOG("create file[%s] failed!",parse_file_name.c_str());
						exit(1);
					}

					is_file_created = true;
				}

				//we must write frame info before writing this frame to file , 
				//because write_to_file will drop this frame. 
				if (buffer.check_bit_value(5, 7))   /* check if the last rtp packet of current frame(MAKER bit) */
				{
					frm_end_time = *capture_time;
					wait_frame = true;
					frm_end = true;
				}
				
				write_frm_info();

				buffer.write_to_file(dav_file, rpt_pkt_len + sizeof(rtsp_interleaved_frame_hdr));

				if (frm_end)
				{
					last_frm_end_time = *capture_time;
				}
				
				state = SEARCH_HDR;
			}	
			
			break;
		default:			
			COMMON_LOG("state %d error!", state); 
			return;
	}

	search_frame(capture_time); //check is there another rtp packet in memory
	return; 				
}

/*
bool rtp_parser::check_big_endian()
{ 
    union { 
        uint i; 
        char c[4]; 
    } bint = {0x01020304}; 
 
    return bint.c[0] == 1;  
}
*/

void rtp_parser::write_frm_info()
{
	fprintf(parse_file, "rtp seq:%8hu, Marker:%d, Len:%7u,   Begin_Rcv_Time:%10lu.%6lus,   End_Rcv_Time:%10lu.%6lus,  Cost_Time:%8uus \n", 
		buffer.parse_as_ushort(6), buffer.check_bit_value(5, 7), rpt_pkt_len, rtp_pkt_start_time.tv_sec, rtp_pkt_start_time.tv_usec, rtp_pkt_end_time.tv_sec, rtp_pkt_end_time.tv_usec, time_diff(&rtp_pkt_start_time, &rtp_pkt_end_time));

	if (buffer.check_bit_value(5, 7))
	{
		if (is_first_frm)
		{
			fprintf(parse_file, "\nFrame info: Begin_Rcv_Time:%10lu.%6lus,	 End_Rcv_Time:%10lu.%6lus,	Cost_Time:%8uus \n\n", 
				 frm_start_time.tv_sec, frm_start_time.tv_usec, frm_end_time.tv_sec, frm_end_time.tv_usec, time_diff(&frm_start_time, &frm_end_time));

			is_first_frm = false;
		}
		else
		{
			fprintf(parse_file, "\nFrame info: Begin_Rcv_Time:%10lu.%6lus,	 End_Rcv_Time:%10lu.%6lus,	Cost_Time:%8uus Frm_Interval:%8uus \n\n", 
				 frm_start_time.tv_sec, frm_start_time.tv_usec, frm_end_time.tv_sec, frm_end_time.tv_usec, time_diff(&frm_start_time, &frm_end_time), time_diff(&last_frm_end_time, &frm_end_time));
		
		}
			
	}
}

unsigned int rtp_parser::time_diff(struct timeval *start, struct timeval *end)
{
	unsigned long long micro_seconds_of_start = start->tv_sec * 1000 * 1000 + start->tv_usec;
	unsigned long long micro_seconds_of_end = end->tv_sec * 1000 * 1000 + end->tv_usec;

	if (micro_seconds_of_end >= micro_seconds_of_start)
	{
		return micro_seconds_of_end - micro_seconds_of_start;
	}
	else
	{
		return micro_seconds_of_start - micro_seconds_of_end;
	}
}
