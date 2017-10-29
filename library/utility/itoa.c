#include "library.h"
#include <errno.h>


static char const __numstr[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char *ultoa(unsigned long n, char *buffer, int radix)
{
	char *p = buffer;
	char buf[34];
	char *bufp;
	unsigned short len;
	
	if ((unsigned int)(radix - 2) > 34)
	{
		errno = EDOM;
		*buffer = '\0';
		return buffer;
	}
	bufp = buf + sizeof(buf);
	len = -1;
	do
	{
		*--bufp = __numstr[n % radix];	/* grab each digit */
		len++;
	} while ((n /= radix) > 0);

	/* reverse and return it */
	do
	{
		*p++ = *bufp++;
	} while (len--);
	*p = '\0';
	return buffer;
}


char *ltoa(long n, char *buffer, int radix)
{
	char *p = buffer;

	if (n < 0 && radix == 10)
	{
		*p++ = '-';
		n = -n;
	}

	ultoa(n, p, radix);

	return buffer;
}


char *itoa(int n, char *buffer, int radix)
{
	return ltoa(n, buffer, radix);
}
