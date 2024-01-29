#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

int isInteger(char* char_ptr){
    while((*char_ptr) != '\0'){
        if((*char_ptr) < '0' || (*char_ptr) > '9'){
            return 0;
        }
        char_ptr++;
    }
    return 1;
}

int count1[2] = {0, 0}; // for both teams
int count2 = 0;
sem_t sem1[2]; // for both teams
sem_t sem2;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
int car_id = 0;

void* fan_thread(void* arg){
    char* team = (char*) arg;
    int my_team_idx = team[0] - 'A';
    int other_team_idx = (my_team_idx+1)%2;
    int captain = -1; // car id

    printf("Thread ID: %ld, Team: %s, I am looking for a car\n", pthread_self(), team);

    // find a spot in the car
    pthread_mutex_lock(&mutex1);
    count1[my_team_idx]++;
    if(count1[my_team_idx]==4){
        captain = car_id++;
        count1[my_team_idx] = 0;
        for(int i=0; i<4; i++){
            sem_post(&sem1[my_team_idx]);
        }
    } else if(count1[my_team_idx]==2 && count1[other_team_idx]>=2){
        captain = car_id++;
        count1[my_team_idx] -= 2;
        count1[other_team_idx] -= 2;
        for(int i=0; i<2; i++){
            sem_post(&sem1[0]);
            sem_post(&sem1[1]);
        }
    } else {
        pthread_mutex_unlock(&mutex1);
    }
    sem_wait(&sem1[my_team_idx]);

    printf("Thread ID: %ld, Team: %s, I have found a spot in a car\n", pthread_self(), team);

    // wait for others to print
    pthread_mutex_lock(&mutex2);
    count2++;
    if(count2 == 4){
        for(int i=0; i<4; i++){
            sem_post(&sem2);
        }
        count2 = 0;
    }
    pthread_mutex_unlock(&mutex2);
    sem_wait(&sem2);

    if(captain != -1){ // driver only
        printf("Thread ID: %ld, Team: %s, I am the captain and driving the car with ID %ld\n", pthread_self(), team, captain);
        pthread_mutex_unlock(&mutex1);
    }

    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    if(argc == 3 && isInteger(argv[1]) && isInteger(argv[2])){
        int team_a_fan_count = atoi(argv[1]);
        int team_b_fan_count = atoi(argv[2]);
        int total_fan_count = team_a_fan_count + team_b_fan_count;

        // argument validation
        if(team_a_fan_count%2==0 && team_b_fan_count%2==0 && (total_fan_count)%4==0){
            sem_init(&sem1[0], 0, 0);
            sem_init(&sem1[1], 0, 0);
            sem_init(&sem2, 0, 0);

            // create threads
            pthread_t threads[total_fan_count];
            for(int i=0; i<total_fan_count; i++){
                char* team = (char*)malloc(sizeof(char));
                (*team) = (i<team_a_fan_count ? 'A' : 'B');
                pthread_t th;
                pthread_create(&th, NULL, fan_thread, (void*)team);
                threads[i] = th;
            }
            
            // join threads
            for(int i=0; i<total_fan_count; i++){
                pthread_join(threads[i], NULL);
            }
        }
    }
    printf("The main terminates\n");
    return 0;
}
