#include "/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.h"
#include <iostream>
#include <pthread.h>
#include<unistd.h>

pthread_t t1,t2,t3,t4,t5;


void* thread_func1(void *arg) {
           
    for(int j=0;j<2;j++){
                  for(int jj=0;jj<2;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in other threads  "<<"\n";
                     doWork(700);
                  }
            }
    return nullptr;

}
void* thread_func(void *arg) {
    
    for(int j=0;j<5;j++){
                  for(int jj=0;jj<5;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in other threads  "<<"\n";
                     doWork(700);
                  }
            }
    return nullptr;


}
void* thread_func4(void *arg) {

    for(int j=0;j<5;j++){
                  for(int jj=0;jj<5;jj++){
                     int *y = static_cast<int*>(arg);
                     // cout<<"Mem loc:"<<y<<endl;
                         std::cout << "Value of y is:" << *y << "\n";
                        std::cout<<"doing some work in thread 4 "<<"\n";
                     doWork(1000);
                  }
            }
    return nullptr;

}
void joinThread2(pthread_t& t) {
    std::cout << "Joining thread...\n";
    pthread_join(t, nullptr);  // Join the thread to ensure it finishes
}

void joinThread(pthread_t& t) {
    std::cout << "Joining thread...\n";
    joinThread2(t);  // Join the thread to ensure it finishes
    std::cout << "Thread joined successfully.\n";
}

void threadFunction() {
    int x = 5,y=20;

    pthread_create(&t2, nullptr, thread_func, &x);
    if(x>=0)
    {
         pthread_create(&t3, nullptr, thread_func, &x);
        
         if(x!=y){
            
             pthread_create(&t4, nullptr, thread_func4, &x);

             if((x+y)%2==0){

                pthread_create(&t1, nullptr, thread_func1, &x);
                pthread_join(t1, nullptr);


             }

        }
        else{
             
            if(x==10){

                    pthread_join(t4, nullptr);
            }
            else {
               
                pthread_join(t4, nullptr);
            }

                pthread_join(t3, nullptr);

        }
      
            // pthread_join(t4, nullptr);  // Join the thread to ensure it finishes
    }
    else 
    {
        if(x*x ==9999999)
        {
                pthread_join(t3, nullptr);
                pthread_join(t2, nullptr);  // Join the thread to ensure it finishes
        }
        else{

                pthread_join(t3, nullptr);
                pthread_join(t2, nullptr);  // Join the thread to ensure it finishes
        }
    
    }
   
    switch(x)
    {
        case 10: pthread_join(t1, nullptr);
        
        default: 
        {
            
        }
        // joinThread(t4);
    }

                    for(int i=0;i<5;i++)
                    {
                        for(int j=0;j<5;j++)
                        {
                            std::cout<<"doing some work in thread "<<"\n";
                            doWork(700);
                        }
                            doWork(200);

                    }

}

int main() {
    //  int x=10;
    // t1=std::thread(thread_func);
    threadFunction();
    pthread_join(t1, nullptr);
    pthread_join(t3, nullptr);
    pthread_join(t4, nullptr);
}
