#include <iostream>
#include <semaphore.h>
// Alpay Nacar
using namespace std;

class HeapManager{
private:
    struct node {
        int id, size, idx;
        node* next;
        node(int size, int idx, node* next = NULL, int id = -1) : id(id), size(size), idx(idx), next(next) {};
    };
    node* root;
    sem_t sem;
    sem_t sem_print;

public:
    HeapManager() : root(NULL) {
        sem_init(&sem, 0, 1);
        sem_init(&sem_print, 0, 1);
    };

    ~HeapManager() {
        sem_wait(&sem);
        cout << "Execution is done\n";
        print();
        // to prevent memory leak, free linked list
        node* ptr = root;
        while(ptr){
            node* temp = ptr;
            ptr = ptr->next;
            delete temp;
        }
        sem_post(&sem); 
    }

    int initHeap(int size){
        sem_wait(&sem);
        // assumed input size is always positive
        root = new node(size, 0);
        cout << "Memory initialized\n";
        print();
        sem_post(&sem); 
        return 1;
    }
    
    int myMalloc(int ID, int size){
        sem_wait(&sem);
        node* ptr = root;
        while(ptr){
            if(ptr->id == -1 && ptr->size >= size){
                // add node for the remaining space
                if(ptr->size != size){
                    ptr->next = new node(ptr->size - size, ptr->idx + size, ptr->next);
                }
                ptr->id = ID;
                ptr->size = size;
                cout << "Allocated for thread " << ID << endl;
                print();
                sem_post(&sem);
                return ptr->idx;
            }
            ptr = ptr->next;
        }
        cout << "Can not allocate, requested size " << size << " for thread " << ID << " is bigger than remaining size\n";
        print();
        sem_post(&sem); 
        return -1;
    }

    int myFree(int ID, int index){
        sem_wait(&sem);
        // find index
        node *prev = NULL, *curr = root;
        while(curr && curr->idx+curr->size <= index){
            prev = curr;
            curr = curr->next;
        }
        // if id matches
        if(curr->id == ID){
            // if chunk is freed int he middle of a chunk
            if(curr->idx < index){
                // first half should stay as it is, second half should be freed only, therefore we add the second half as another node
                int non_affected_size = index - curr->idx;
                int affected_size = curr->idx + curr->size - index;
                curr->size = non_affected_size;
                prev = curr;
                curr = curr->next = new node(affected_size, index, curr->next);
            }
            curr->id = -1;
            // coalescing with next node
            if(curr->next && curr->next->id == -1){
                curr->size += curr->next->size;
                node* temp = curr->next;
                curr->next = curr->next->next;
                delete temp;
            }
            // coalescing with prev node
            if(prev && prev->id == -1){
                prev->size += curr->size;
                prev->next = curr->next;
                delete curr;
            }
            cout << "Freed for thread " << ID << endl;
            print();
            sem_post(&sem); 
            return 1;
        }
        // if not matches
        cout << "Can not free, requested index " << index << " for thread " << ID << "\n";
        print();
        sem_post(&sem); 
        return -1;
    }

    void print(){
        sem_wait(&sem_print);
        node* ptr = root;
        while(ptr){
            cout << '[' << ptr->id << ']' << '[' << ptr->size << ']' << '[' << ptr->idx << ']';
            if(ptr->next) cout << "---";
            ptr = ptr->next;
        }
        cout << endl;
        sem_post(&sem_print);
    }

};
