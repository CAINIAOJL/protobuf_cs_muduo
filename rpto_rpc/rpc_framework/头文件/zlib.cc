#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
#include <zlib.h>
 
 
int main(int argc, char *argv[])
{ 
    char inbuf[] = "Hello, This is a demo for compress and uncompress interface!\n"
                 "Written by windeal.li\n"
                 "email: 2378264731@qq.com\n";
    uLong inlen = sizeof(inbuf);
    char *outbuf = NULL;
    uLong outlen;
 
    outlen = compressBound(inlen);
    printf("in_len: %ld\n", inlen);
    printf("out_len: %ld\n", outlen);
 
    if ((outbuf = (char *)malloc(sizeof(char) * outlen)) == NULL){
        fprintf(stderr, "Failed to malloc memory for outbuf!\n");
        return -1;
    }
 
    /* compress */
    if (compress(outbuf, &outlen, inbuf, inlen) != Z_OK) {
        fprintf(stderr, "Compress failed!\n");
        return -1;
    }
    printf("Compress Sucess!\n");
    printf("\routlen:%ld, outbuf:%s\n", outlen, outbuf);
    //printf("the sizeof of outbuf is %ld\n", sizeof(outbuf));
    //printf("(sizeof(char*)) = %d\n", sizeof(char*));
  
    memset(inbuf, 0, sizeof(inbuf));
    /* Uncompress */
    if (uncompress(inbuf, &inlen, outbuf, outlen) != Z_OK){
        fprintf(stderr, "Uncompress failed!\n");
        return -1;
    }
    printf("Uncompress Success!\n");
    printf("\rinlen:%ld, inbuf:%s\n", inlen, inbuf);
 
    /* free memory */
    if (outbuf != NULL){
        free(outbuf);
        outbuf = NULL;
    }
 
    return 0;
}

//g++ -o zlib zlib.cc -fpermissive -lz