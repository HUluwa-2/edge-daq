#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void swap(char *a,char *b)
{
    char temp=*a;
    *a=*b;
    *b=temp;
}
void reverse_str(char *s)
{
    if(s==NULL)
    return;

    int left=0;
    int right=strlen(s)-1;
    while(left<right)
    {
        swap(&s[left],&s[right]);
        left++;
        right--;
    }
}

int main(int argc,char *argv[])
{
    char *s=argv[1];
    reverse_str(s);
    printf("reversed string is %s\n",s);
    return 0;
}