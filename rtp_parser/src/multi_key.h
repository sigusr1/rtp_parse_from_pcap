#ifndef __MULTI_KEY__
#define __MULTI_KEY__

class multi_key 
{
public:
	multi_key(unsigned int k1, unsigned short k2, unsigned int k3, unsigned short k4)
      : key1(k1), key2(k2), key3(k3), key4(k4)
      {
      }  

	bool operator<(const multi_key &right) const 
	{
		if ( key1 == right.key1 ) 
		{
			if ( key2 == right.key2 ) 
			{
				if ( key3 == right.key3 ) 
				{
					return key4 < right.key4;
				}
				else 
				{
					return key3 < right.key3;
				}
			}
			else 
			{
				return key2 < right.key2;
			}
		}
		else 
		{
			return key1 < right.key1;
		}
	}

private:
	unsigned int	key1;
	unsigned short	key2;
	unsigned int	key3;
	unsigned short	key4;
};

#endif
