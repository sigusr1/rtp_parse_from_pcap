#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynamic_ring_buffer.h"

dynamic_ring_buffer::dynamic_ring_buffer(unsigned int size):buf(NULL)
{
	buf = (unsigned char *)malloc(size);

	if(buf)
	{
		capacity = size;
	}
	else
	{
		capacity = 0;
	}

	data_in_buf = 0;
	read_ptr = 0;
	write_ptr = 0;
	kmp_array = NULL;
	pattern_str = NULL;
	pattern_str_len = 0;
}

dynamic_ring_buffer::~dynamic_ring_buffer()
{

	if( buf != NULL )
	{
		free(buf);
	}

	if( kmp_array != NULL )
	{
		delete []kmp_array;
	}

	if( pattern_str != NULL )
	{
		delete []pattern_str;
	}
}

void dynamic_ring_buffer::write_data(const char *payload, unsigned int len)
{
	if (len == 0)
	{
		return;
	}
	
	int add_size =  data_in_buf + len - capacity;
	
	if ( add_size > 0 )
	{
		add_buffer(add_size);
	}

	if( write_ptr >= read_ptr )
	{
		unsigned int len_until_tail = capacity - write_ptr;
		if ( len_until_tail >= len )
		{
			memcpy(buf + write_ptr, payload, len);
		}
		else
		{
			// In this case, some data copy to the end of buffer, the left copy to the start of buffer
			//Since add_buffer promise us enough buffer, we don't check left space again

			unsigned int len_from_head = len - len_until_tail;						
			memcpy(buf + write_ptr, payload, len_until_tail);
			memcpy(buf, payload + len_until_tail, len_from_head);
		}
		
	}
	else
	{
		//In this case, add_buffer promise us read_ptr - write_ptr >= len
		memcpy(buf + write_ptr, payload, len);
	}
	
	write_ptr = (write_ptr + len) % capacity;
	data_in_buf += len;
}

void dynamic_ring_buffer::drop_data(unsigned int len)
{
	len  = (len >= data_in_buf ? data_in_buf : len);
	
	read_ptr = ( read_ptr + len ) % capacity;
	data_in_buf = data_in_buf - len;
}
unsigned int dynamic_ring_buffer::get_free_size()
{
	return capacity - data_in_buf;
}

unsigned int dynamic_ring_buffer::get_data_size()
{
	return data_in_buf;
}


void dynamic_ring_buffer::add_buffer(unsigned int size)
{
	unsigned int new_capacity = size + capacity;
	
	buf =  (unsigned char *)realloc(buf, new_capacity);
	
	if(buf == NULL)
	{
		COMMON_LOG("realloc_ring_buffer failed! current buf %u byte, add buf %u byte.", capacity - size, size);
		exit(1);
	}

	if( read_ptr > write_ptr )
	{//read_ptr > write_ptr, some data is left in the middle of the buffer, we should move them to the end of the buffer
	
		unsigned int left_data_len = capacity - read_ptr;
		memmove(buf + read_ptr + size, buf + read_ptr, left_data_len);
		read_ptr = read_ptr + size;
	}
	else if( read_ptr == write_ptr && data_in_buf == capacity ) 
	{//read_ptr == write_ptr && data_in_buf == capacity, the buffer is full

		if( write_ptr == 0)
		{// read_ptr = write_ptr = 0, the buffer is full, and the data is in order
			write_ptr = capacity;
		}
		else
		{
			unsigned int left_data_len = capacity - read_ptr;
			memmove(buf + read_ptr + size, buf + read_ptr, left_data_len);
			read_ptr = read_ptr + size;		
		}			
	}
	
	capacity = new_capacity;
}

bool dynamic_ring_buffer::kmp_init(const char *pattern)
{
	if( NULL == pattern )
	{
		return false;
	}
	
	pattern_str_len = strlen(pattern);

	if(kmp_array)
	{
		delete []kmp_array;
	}
	kmp_array = new int[pattern_str_len];

	if(pattern_str)
	{
		delete []pattern_str;
	}
	pattern_str = new unsigned char[pattern_str_len];

	memcpy(pattern_str, pattern, pattern_str_len);	//Record the pattern str for search

	//Calculate the kmp next array
	kmp_array[0] = -1;
	int i = -1;  
	int j = 0;	
	while (j < pattern_str_len - 1)  
	{  
		//pattern[i] is prefix, pattern[j] is suffix
		if (i == -1 || pattern[j] == pattern[i])   
		{  
			++i;  
			++j;  
			kmp_array[j] = i;  
		}  
		else   
		{  
			i = kmp_array[i];  
		}  
	}

	return true;
}  

int dynamic_ring_buffer::kmp_search_ring_buffer()
{  
	int i = 0;	
	int j = 0;	

	while (i < data_in_buf && j < pattern_str_len)  
	{  
		if (j == -1 || buf[(read_ptr + i) % capacity] == pattern_str[j])  
		{  
			i++;  
			j++;  
		}  
		else  
		{  
			j = kmp_array[j];  
		}  
	}
	
	if (j == pattern_str_len)	
		return ( i - j) % capacity;  
	else  
		return -1;	
}

void dynamic_ring_buffer::write_to_file( FILE *file, unsigned int len)
{
	if(file == NULL)
	{
		COMMON_LOG("%s", "write_to_file failed! file handle is null!");
		exit(1);
	}
	
	len  = (len >= data_in_buf ? data_in_buf : len);

	if (read_ptr < write_ptr)
	{
		fwrite(buf + read_ptr, len, 1, file);
	}
	else
	{
		unsigned int len_until_tail = capacity - read_ptr;
		if ( len_until_tail >= len )
		{
			fwrite(buf + read_ptr, len, 1, file);
		}
		else
		{
			// In this case, the data needed to wrtie file is distributed  into two parts , 
			// one at the end of buf, the other at the begin of the buf
			unsigned int len_from_head = len - len_until_tail;						
			fwrite(buf + read_ptr, len_until_tail, 1, file);
			fwrite(buf, len_from_head, 1, file);
		}		
	}
	
	read_ptr = ( read_ptr + len ) % capacity;
	data_in_buf = data_in_buf - len;
	
}

void dynamic_ring_buffer::print_buffer()
{
	COMMON_LOG("read_ptr:%u   write_ptr:%u  capacity:%u data_in_buf:%u", read_ptr, write_ptr, capacity, data_in_buf);
	
	if (read_ptr < write_ptr)
	{
		for(uint i = 0; i < capacity; i++)
		{
			if (i < read_ptr || i >= write_ptr)
			{
				printf("%4c", '*');
			}
			else
			{
				printf("%4x", (unsigned char)buf[i]);
			}

			if( (i + 1) % 16 == 0 )  
			{  
				printf("\n");  
			}  
		}
	}
	else if (read_ptr > write_ptr ||(read_ptr == write_ptr && data_in_buf == capacity))
	{
		for(uint i = 0; i < capacity; i++)
		{
			if (i >= read_ptr || i < write_ptr)
			{
				printf("%4x", buf[i]);
			}
			else
			{
				printf("%4c", '*');
			}

			if( (i + 1) % 16 == 0 )  
			{  
				printf("\n");  
			}  
		}

	}
	printf("\n");  	
}


unsigned char dynamic_ring_buffer::checksum_of_buffer(unsigned int len)
{
	unsigned char check_sum = 0;

	if ( len > data_in_buf )
	{
		COMMON_LOG("data_in_buf:%u   len:%u", data_in_buf, len);
		return 0;
	}
	
	for(uint i = 0; i < len; i++)
	{
		check_sum += buf[(read_ptr + i) % capacity];
	}

	return check_sum;
}

unsigned char dynamic_ring_buffer::parse_as_char(unsigned int index)
{
	if (index >= data_in_buf)
	{
		COMMON_LOG("data_in_buf:%u index:%u", data_in_buf, index);
		return 0;
	}

	return buf[(read_ptr + index) % capacity];
}

unsigned int dynamic_ring_buffer::parse_as_uint(unsigned int index)
{
	return parse_continuous_buffer_as_uint(index, sizeof(unsigned int));
}

bool dynamic_ring_buffer::check_bit_value(unsigned int byte_index, unsigned int bit_pos)
{
	if ( data_in_buf < 1 || byte_index > data_in_buf - 1 || bit_pos >= 8)
	{
		COMMON_LOG("data_in_buf:%u byte_index:%u bit_pos:%u", data_in_buf, byte_index, bit_pos);
		return 0;
	}

	return CHECK_BIT(buf[(read_ptr + byte_index) % capacity], bit_pos);
}

unsigned int dynamic_ring_buffer::parse_as_ushort(unsigned int index)
{
	return parse_continuous_buffer_as_uint(index, sizeof(unsigned short));
}

unsigned int dynamic_ring_buffer::parse_continuous_buffer_as_uint(unsigned int index, unsigned int size)
{
	unsigned int tmp = 0;

	if (data_in_buf < size || index > data_in_buf - size)
	{
		COMMON_LOG("data_in_buf:%u index:%u size:%u", data_in_buf, index, size);
		return 0;
	}

	/*
	//we assume the data in buffer is little endian	
	for( int i = size - 1; i >= 0; i--)
	{
		tmp = tmp +(buf[(read_ptr + index + i) % capacity] << (i * 8));
	}
	*/
	
	//we assume the data in buffer is big endian	
	for(int i = 0; i < size; i++)
	{
		tmp = buf[(read_ptr + index + i) % capacity] +(tmp << (i * 8));
	}
	
	return tmp;
}
