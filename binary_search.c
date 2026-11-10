#include <stdio.h>
#include <stdlib.h>

int binary_search(int *arr,int n,int target)
{
    int left=0,right=n-1;
    while(left<=right)
    {
        int mid=left+(right-left)/2;
        if(arr[mid]==target)
        {
            return mid;
        }
        else if(arr[mid]<target)
        {
            left=mid+1;
        }
        else if(arr[mid]>target)
        {
            right=mid-1;
        }
    }
    return -1;
}

int main(int argc,char *argv[])
{
    int b[5]={1,2,3,4,5};
    int len=sizeof(b)/sizeof(b[0]);
    int target=atoi(argv[1]);

    int index=binary_search(b,len,target);
    
    printf("target %d is at index %d\n",target,index);

    return 0;
}