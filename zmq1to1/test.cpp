
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <time.h>
#include <atomic>
#include <list>
#include <memory>
#include <atomic>
#include <unistd.h>
#include "ypipe.hpp"


#define PRINT_THREAD_INTO() printf("%s %lu into\n", __FUNCTION__, pthread_self())
#define PRINT_THREAD_LEAVE() printf("%s %lu leave\n", __FUNCTION__, pthread_self())

#if 0
#define PRINT_THREAD_INTO()
#define PRINT_THREAD_LEAVE()
#endif



#define QUEUE_CHUNK_SIZE          100


static int s_queue_item_num = 2000000; // 每个线程插入的元素个数
static int s_producer_thread_num = 1;  // 生产者线程数量
static int s_consumer_thread_num = 1;  // 消费线程数量

static int s_count_push = 0;
static int s_count_pop = 0;

ypipe_t<int, QUEUE_CHUNK_SIZE> yqueue;




int64_t get_current_millisecond()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000);
}




typedef void *(*thread_func_t)(void *argv);


static int lxx_atomic_add(int *ptr, int increment)
{
  int old_value = *ptr;
  __asm__ volatile("lock; xadd %0, %1 \n\t"
                   : "=r"(old_value), "=m"(*ptr)
                   : "0"(increment), "m"(*ptr)
                   : "cc", "memory");
  return *ptr;
}




void *yqueue_producer_thread(void *argv)
{
    PRINT_THREAD_INTO();
    int count = 0;
    for (int i = 0; i < s_queue_item_num; i++)
    {
        yqueue.write(count, false); 
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.flush();
    }

    PRINT_THREAD_LEAVE();
    return NULL;
}



void *yqueue_consumer_thread(void *argv)
{
    int last_value = -1;
    PRINT_THREAD_INTO();

    while (true)
    {
        int value = 0;
        if (yqueue.read(&value))
        {
            if (s_consumer_thread_num == 1 && s_producer_thread_num == 1 && (last_value + 1) != value) // 只有一入一出的情况下才有对比意义
            {
                printf("pid:%lu, -> value:%d, expected:%d\n", pthread_self(), value, last_value + 1);
            }
            lxx_atomic_add(&s_count_pop, 1);
            last_value = value;
        }
        else
        {
            // printf("%s %lu no data, s_count_pop:%d\n", __FUNCTION__, pthread_self(), s_count_pop);
            // usleep(100);

            // sched_yield()函数是一个调度函数，它会让出当前线程的CPU时间片，让其他就绪状态的线程获得CPU时间片来进行执行，但并不会导致当前线程阻塞。
            sched_yield();
        }

        if (s_count_pop >= s_queue_item_num * s_producer_thread_num)
        {
            // printf("%s dequeue:%d, s_count_pop:%d, %d, %d\n", 
            //           __FUNCTION__, last_value, s_count_pop, s_queue_item_num, s_consumer_thread_num);
            break;
        }
    }
    PRINT_THREAD_LEAVE();
    return NULL;
}





void *yqueue_consumer_thread_yield(void *argv)
{
    int last_value = -1;
    PRINT_THREAD_INTO();

    while (true)
    {
        int value = 0;
        if (yqueue.read(&value))
        {
            if (s_consumer_thread_num == 1 && s_producer_thread_num == 1 && (last_value + 1) != value) // 只有一入一出的情况下才有对比意义
            {
                printf("pid:%lu, -> value:%d, expected:%d\n", pthread_self(), value, last_value + 1);
            }
            lxx_atomic_add(&s_count_pop, 1);
            last_value = value;
        }
        else
        {
            // printf("%s %lu no data, s_count_pop:%d\n", __FUNCTION__, pthread_self(), s_count_pop);
            // usleep(100);

            // sched_yield()函数是一个调度函数，它会让出当前线程的CPU时间片，让其他就绪状态的线程获得CPU时间片来进行执行，但并不会导致当前线程阻塞。
            sched_yield();
        }

        if (s_count_pop >= s_queue_item_num * s_producer_thread_num)
        {
            // printf("%s dequeue:%d, s_count_pop:%d, %d, %d\n", 
            //           __FUNCTION__, last_value, s_count_pop, s_queue_item_num, s_consumer_thread_num);
            break;
        }
    }
    PRINT_THREAD_LEAVE();
    return NULL;
}




void *yqueue_producer_thread_batch(void *argv)
{
    PRINT_THREAD_INTO();
    int count = 0;
    int item_num = s_queue_item_num / 10;
    for (int i = 0; i < item_num; i++)
    {
        yqueue.write(count, true);  // 写true
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, true);
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, true);
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, true);
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, true);
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, true);
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, true);
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, true);
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, true);
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.write(count, false);   //最后一个元素 写false
        count = lxx_atomic_add(&s_count_push, 1);
        yqueue.flush();
    }
    PRINT_THREAD_LEAVE();
    return NULL;
}




std::mutex ypipe_mutex_;
std::condition_variable ypipe_cond_;


void *yqueue_producer_thread_condition(void *argv)
{
    PRINT_THREAD_INTO();
    int count = 0;
    for (int i = 0; i < s_queue_item_num; i++)
    {
        yqueue.write(count, false); 
        count = lxx_atomic_add(&s_count_push, 1);

        // yqueue.flush();
        // std::unique_lock<std::mutex> lock(ypipe_mutex_);
        //  ypipe_cond_.notify_one();

        if (!yqueue.flush())
        {
            // printf("notify_one\n");
            std::unique_lock<std::mutex> lock(ypipe_mutex_);
            ypipe_cond_.notify_one();
        }
    }
    std::unique_lock<std::mutex> lock(ypipe_mutex_);
    ypipe_cond_.notify_one();

    PRINT_THREAD_LEAVE();
    return NULL;
}




void *yqueue_consumer_thread_condition(void *argv)
{
    int last_value = -1;
    PRINT_THREAD_INTO();

    while (true)
    {
        int value = 0;
        if (yqueue.read(&value))
        {
            if (s_consumer_thread_num == 1 && s_producer_thread_num == 1 && (last_value + 1) != value) // 只有一入一出的情况下才有对比意义
            {
                printf("pid:%lu, -> value:%d, expected:%d\n", pthread_self(), value, last_value + 1);
            }
            lxx_atomic_add(&s_count_pop, 1);
            last_value = value;
        }
        else
        {
            // printf("%s %lu no data, s_count_pop:%d\n", __FUNCTION__, pthread_self(), s_count_pop);
            // usleep(100);

            std::unique_lock<std::mutex> lock(ypipe_mutex_);

            //  printf("wait\n");
            ypipe_cond_.wait(lock);
            
            // sched_yield();
        }

        if (s_count_pop >= s_queue_item_num * s_producer_thread_num)
        {
            // printf("%s dequeue:%d, s_count_pop:%d, %d, %d\n", 
            //         __FUNCTION__, last_value, s_count_pop, s_queue_item_num, s_consumer_thread_num);
            break;
        }
    }

    // printf("%s dequeue: last_value:%d, s_count_pop:%d, %d, %d\n", __FUNCTION__,
    //        last_value, s_count_pop, s_queue_item_num, s_consumer_thread_num);
    
    PRINT_THREAD_LEAVE();
    return NULL;
}




int test_queue(thread_func_t func_push, thread_func_t func_pop, void *argv)
{
    int64_t start = get_current_millisecond();


    pthread_t tid_push[s_producer_thread_num] = {0};
    for (int i = 0; i < s_producer_thread_num; i++)
    {
        int ret = pthread_create(&tid_push[i], NULL, func_push, argv);
        if (0 != ret)
        {
            printf("create thread failed\n");
        }
    }


    pthread_t tid_pop[s_consumer_thread_num] = {0};
    for (int i = 0; i < s_consumer_thread_num; i++)
    {
        int ret = pthread_create(&tid_pop[i], NULL, func_pop, argv);
        if (0 != ret)
        {
            printf("create thread failed\n");
        }
    }


    for (int i = 0; i < s_producer_thread_num; i++)
    {
        pthread_join(tid_push[i], NULL);
    }
    for (int i = 0; i < s_consumer_thread_num; i++)
    {
        pthread_join(tid_pop[i], NULL);
    }


    int64_t end = get_current_millisecond();
    int64_t temp = s_count_push;
    int64_t ops = (temp * 1000) / (end - start);   // 计算每秒操作次数

    // %u 格式化输出无符号十进制整数
    printf("spend time:%ldms, push:%d, pop:%d, ops:%lu\n", (end - start), s_count_push, s_count_pop, ops);

    return 0;
}




int main(int argc, char **argv)
{
    if (argc >= 4 && atoi(argv[3]) > 0) {
        s_queue_item_num = atoi(argv[3]);
    }

    if (argc >= 3 && atoi(argv[2]) > 0) {
        s_consumer_thread_num = atoi(argv[2]);
    }

    if (argc >= 2 && atoi(argv[1]) > 0) {
        s_producer_thread_num = atoi(argv[1]);
    }

    printf("\nthread num - producer:%d, consumer:%d, push:%d\n", 
            s_producer_thread_num, s_consumer_thread_num, s_queue_item_num);


    for (int i = 0; i < 1; i++)
    {

        if (s_consumer_thread_num == 1 && s_producer_thread_num == 1)
        {
            s_count_push = 0;
            s_count_pop = 0;

            printf("\n###### use ypipe_t ######\n");
            test_queue(yqueue_producer_thread, yqueue_consumer_thread, NULL);

            s_count_push = 0;
            s_count_pop = 0;

            printf("\n###### use ypipe_t batch ######\n");
            test_queue(yqueue_producer_thread_batch, yqueue_consumer_thread_yield, NULL);

            s_count_push = 0;
            s_count_pop = 0;

            printf("\n###### use ypipe_t condition ######\n");
            test_queue(yqueue_producer_thread_condition, yqueue_consumer_thread_condition, NULL);

        }
        else
        {
            printf("\nypipe_t only support one write one read thread, but you write %d thread and read %d thread so no test it.\n",
                    s_producer_thread_num, s_consumer_thread_num);
        } 
    }

    printf("\nfinish\n");
    return 0;
}






