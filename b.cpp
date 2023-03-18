#include <iostream> 
#include <stdlib.h>
#include <cstring>

class A{
    public:
        int a; 
        int b; 
    
    public:
        int test(){
            return 0;
        }
};

int main(void){
    float a = 0.707; 
    float b = a * 300;
    std::cout<<b<<std::endl;
}