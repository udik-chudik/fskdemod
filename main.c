#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

short readSample()
{
    /*
        Read 2 bytes from STDIN assuming 16 bit signed int low endian
        return short => sample
    */
    unsigned char buf[2];
    if (read(0, &buf, sizeof(buf)) <=0 )
    {
        exit(0);
    }
    short t = (buf[1] << 8) | buf[0];
    return t;
}
int main()
{
    
    int co = 0;
    short prev, next;
    prev = readSample();
    while (1)
    {
        short next = readSample();
        if (prev*next < 0)
        {
            //Zero cross detected
            
        }
        co++;
        if (co > 100)
            return 0;
    }
}