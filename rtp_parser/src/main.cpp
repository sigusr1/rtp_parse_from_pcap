#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "rtp_parser.h"
#include "../include/nids.h"


#define int_ntoa(x)	inet_ntoa(*((struct in_addr *)&x))

// struct tuple4 contains addresses and port numbers of the TCP connections
// the following auxiliary function produces a string looking like
// 10.0.0.1,1024,10.0.0.2,23
char * adres (struct tuple4 addr)
{
  static char buf[256];
  strcpy (buf, int_ntoa (addr.saddr));
  sprintf (buf + strlen (buf), ",%i,", addr.source);
  strcat (buf, int_ntoa (addr.daddr));
  sprintf (buf + strlen (buf), ",%i", addr.dest);
  return buf;
}

void tcp_resume( struct tcphdr *this_tcphdr, struct ip *this_iphdr, int *resume)
{
  *resume =  NIDS_TCP_RESUME_CLIENT;

}
void tcp_callback (struct tcp_stream *a_tcp, void ** this_time_not_needed, struct timeval *capture_time)
{
  char buf[1024];
  strcpy (buf, adres (a_tcp->addr)); // we put conn params into buf
  if (a_tcp->nids_state == NIDS_JUST_EST || a_tcp->nids_state == NIDS_RESUME)
  {
    // connection described by a_tcp is established
    // here we decide, if we wish to follow this stream
    // sample condition: if (a_tcp->addr.dest!=23) return;
    // in this simple app we follow each stream, so..
    a_tcp->client.collect++; // we want data received by a client
    a_tcp->server.collect++; // and by a server, too

	//we don't care urgent data
    //a_tcp->server.collect_urg++; 
    //a_tcp->client.collect_urg++; 
    fprintf (stderr, "%s established\n", buf);
    return;
  }
  if (a_tcp->nids_state == NIDS_CLOSE)
  {
    // connection has been closed normally
    fprintf (stderr, "%s closing\n", buf);
    return;
  }
  if (a_tcp->nids_state == NIDS_RESET)
  {
    // connection has been closed by RST
    fprintf (stderr, "%s reset\n", buf);
    return;
  }

  if (a_tcp->nids_state == NIDS_DATA)
  {
    // new data has arrived; gotta determine in what direction
    // and if it's urgent or not

    struct half_stream *hlf;

	//Ignore urgent data
	/*
    if (a_tcp->server.count_new_urg)
    {
      // new byte of urgent data has arrived 
      strcat(buf,"(urgent->)");
      buf[strlen(buf)+1]=0;
      buf[strlen(buf)]=a_tcp->server.urgdata;
      write(1,buf,strlen(buf));
      return;
    }
    */
    rtp_parser* stream = NULL;

    if (a_tcp->client.count_new)
    {
      // new data for client
      hlf = &a_tcp->client; // from now on, we will deal with hlf var,
                            // which will point to client side of conn
      strcat (buf, "(<-)"); // symbolic direction of data
      stream = rtp_parser::instance(a_tcp->addr.daddr, a_tcp->addr.dest, a_tcp->addr.saddr, a_tcp->addr.source);
      stream->put_data((const char *)hlf->data, hlf->count_new, capture_time);

    }
    else
    {
      hlf = &a_tcp->server; // analogical
      strcat (buf, "(->)");
      stream = rtp_parser::instance(a_tcp->addr.saddr, a_tcp->addr.source, a_tcp->addr.daddr, a_tcp->addr.dest);
      stream->put_data((const char *)hlf->data, hlf->count_new, capture_time);

    }
    //fprintf(stderr,"%s\n",buf); // we print the connection parameters
                                // (saddr, daddr, sport, dport) accompanied
                                                 // by data flow direction (-> or <-)
  }
  return ;
}

int main (int argc, char **argv)
{

  //check command line arguments 
  if (argc < 2) 
  { 
    fprintf(stderr, "Usage: %s [pcap file]\n", argv[0]); 
    exit(1); 
  }

  // here we can alter libnids params, for instance:
  nids_params.filename = argv[1];
  //nids_params.syslog_level = 8;
  if (!nids_init ())
  {
  	fprintf(stderr,"%s\n",nids_errbuf);
  	exit(1);
  }

  nids_register_tcp ((void (*))tcp_callback);
  nids_register_tcp_resume((void (*))tcp_resume);

  nids_run ();

  return 0;
}


//Follow is old pcap parse code, we replcace it with libnids

//void get_packet(unsigned char *arg, const struct pcap_pkthdr *pkt_hdr, const unsigned char *packet)  
//{
//	struct ether_header *eth_hdr;	//ethernet frame head
//	struct iphdr *ip_hdr;			//ip datagram head
//	struct tcphdr *tcp_hdr;			//tcp head
//	const unsigned char *payload;
//
//	//int *id = (int *)arg;  
//	//printf("id: %d\n", ++(*id));  
//	//printf("Packet length: %d\n", pkt_hdr->len);  
//	//printf("Number of bytes: %d\n", pkt_hdr->caplen);  
//	//printf("Recieved time: %s", ctime((const time_t *)&pkt_hdr->ts.tv_sec));   
//
//  	if (pkt_hdr->caplen != pkt_hdr->len)
//	{
//		printf("%d != %d!!! Don't have complete packet. Skipping.\n",
//			pkt_hdr->caplen, pkt_hdr->len);
//		return;
//	}
//
//	eth_hdr = (struct ether_header *)packet;
//
//	if(ntohs(eth_hdr->ether_type) == ETHERTYPE_IP)// Just Process ip datagram
//	{
//		ip_hdr=(struct iphdr *)(packet + sizeof(struct ether_header));//get ip datagram head's address
//		unsigned int ip_head_lenth = ip_hdr->ihl * 4;
//		unsigned int ip_packet_lenth = ntohs(ip_hdr->tot_len);
//		unsigned int src_ip = 0;
//		unsigned int dst_ip = 0;
//		unsigned short src_port = 0;
//		unsigned short dst_port = 0;
//		rtp_parse* stream = NULL;
//		
//		switch(ip_hdr->protocol)
//		{
//			case IPPROTO_TCP:
//				tcp_hdr = (struct tcphdr*)(packet + sizeof(struct ether_header) + ip_head_lenth);//tcp head address
//				payload = (const unsigned char *)tcp_hdr + tcp_hdr->doff *4;//tcp data address
//				
//				src_ip = /*ntohl*/(ip_hdr->saddr);
//				dst_ip = /*ntohl*/(ip_hdr->daddr);
//				src_port = ntohs(tcp_hdr->source);
//				dst_port = ntohs(tcp_hdr->dest);
//
//				stream = rtp_parse::instance(src_ip, src_port, dst_ip, dst_port);
//				stream->put_data((const char *)payload, ip_packet_lenth - ip_head_lenth - tcp_hdr->doff *4);
//				
//				break;			
//			//default:
//				//todo
//				//printf("Unknown IP protocol %d.\n", ip_hdr->protocol);
//		}
//	}
//}
//
//int main(int argc, char **argv) 
//{   
//	//check command line arguments 
//	if (argc < 2) 
//	{ 
//		fprintf(stderr, "Usage: %s [pcap file]\n", argv[0]); 
//		exit(1); 
//	}
//	
//	//open the pcap file 
//	pcap_t *file; 
//	char err_buf[PCAP_ERRBUF_SIZE];
//	file = pcap_open_offline(argv[1], err_buf);
//	if (file == NULL) 
//	{ 
//		fprintf(stderr,"Couldn't open pcap file %s: %s\n", argv[1], err_buf); 
//		exit(1);		
//	} 
//
//
//	/*
//	// construct a filter
//
//	struct bpf_program filter;
//	int ret = -1;
//	
//	ret = pcap_compile(file, &filter, "tcp src port 80 and src host 172.29.3.137", 1, 0);  
//	if (ret < 0)
//	{
//		fprintf(stderr, "Couldn't compile filter: %s\n", pcap_geterr(file));
//		exit(1);		
//	}
//	
//	ret = pcap_setfilter(file, &filter);
//	if (ret < 0)
//	{
//		fprintf(stderr, "Set filter failed: %s\n", pcap_geterr(file));
//		exit(1);		
//	}
//	*/
//
//	int id = 0;  
//	pcap_loop(file, -1, get_packet, (unsigned char*)&id);	//loop until file end
//	pcap_close(file);	//close the pcap file 
//
//	return 0;
//}
