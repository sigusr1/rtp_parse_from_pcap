#ifndef __RING_BUFFER__
#define __RING_BUFFER__

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define COMMON_LOG(fmt, ...)    printf("%s:%d: " fmt "\n", __FUNCTION__, __LINE__, __VA_ARGS__)

class dynamic_ring_buffer
{
public:
	dynamic_ring_buffer(unsigned int size);
	~dynamic_ring_buffer();

	//write data to ring buffer
	void write_data(const char *payload, unsigned int len);

	//drop old data from ring buffer
	void drop_data(unsigned int len);

	//generate the kmp next array for pattern string
	//Return true on success, else return false
	bool kmp_init(const char *pattern);

	//search the pattern string in the ring buffer, search region is[read_ptr, write_ptr), the search region may be wrapped
	//Return the start index of pattern string if found, else return -1
	//Note: if found, the return value is read_ptr's offset value
	int kmp_search_ring_buffer();

	//return available data size	
	unsigned int get_data_size();

	//calculate the checksum of first len bytes
	unsigned char checksum_of_buffer(unsigned int len);

	//get the (read_ptr + index)'s value
	unsigned char parse_as_char(unsigned int index);

	//from address (read_ptr + index), parse an unsigned int
	//we assume the data in buffer is big endian
	unsigned int parse_as_uint(unsigned int index);

	//from address (read_ptr + index), parse an unsigned short
	//we assume the data in buffer is big endian
	unsigned int parse_as_ushort(unsigned int index);

	//check byte's bit value, the byte's address is (read_ptr + index)
	bool check_bit_value(unsigned int byte_index, unsigned int bit_pos);

	//write the first len bytes to file and drop them
	void write_to_file( FILE *file, unsigned int len);

	void print_buffer();
private:
	//return free buf size
	unsigned int get_free_size();
	
	//add size bytes buffer
	void add_buffer(unsigned int size);

	//from address (read_ptr + index), parse continuous bytes to an unsigned int
	//we assume the data in buffer is big endian
    unsigned int parse_continuous_buffer_as_uint(unsigned int index, unsigned int size);
private:
	unsigned char *buf;		//start address of ring buffer
	unsigned int capacity;	//buffer size

	unsigned int data_in_buf;
	unsigned int read_ptr;		// read pointer, point to the start address of data, offset value
	unsigned int write_ptr;		//write pointer, point to the start address where you can write new data, offset value
	int *kmp_array;				// kmp next array 
	unsigned char *pattern_str;			//Key word
	int pattern_str_len;
};

#endif
