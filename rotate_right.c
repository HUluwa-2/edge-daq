#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void swap(char *a,char *b)
{
    char temp=*a;
    *a=*b;
    *b=temp;
}

void reverse_range(char *s,int left,int right)
{
    while(left<right)
    {
        swap(&s[left],&s[right]);
        left++;
        right--;
    }
}

void rotate_right(char *s,int n,int k)
{
    if(s==NULL||n<=0)
    return;

    k=k%n;
    reverse_range(s,0,n-1);
    reverse_range(s,0,k-1);
    reverse_range(s,k,n-1);
}

int main(int argc,char *argv[])
{
    char *s=argv[1];
    int n=strlen(s);
    int k=atoi(argv[2]);
    rotate_right(s,n,k);
    printf("rotated string is %s\n",s);

    return 0;
}