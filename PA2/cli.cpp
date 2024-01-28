#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>
#include <pthread.h>
// Alpay Nacar
using namespace std;

pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;

void * listener(void * arg){
	FILE * pipeFile = (FILE *)arg;
	char buf[256];
	
	while(!fgets(buf, sizeof(buf), pipeFile)){} // spin until you can read
	
	pthread_mutex_lock(&write_lock);
	cout << "---- " << pthread_self() << endl;
	fsync(STDOUT_FILENO);
	do {
		cout << buf;
		fsync(STDOUT_FILENO);
	} while(fgets(buf, sizeof(buf), pipeFile));
	cout << "---- " << pthread_self() << endl;
	fsync(STDOUT_FILENO);
	pthread_mutex_unlock(&write_lock);

	return NULL;
}

void run_command(char* args[4], const string& redirect, const string& redirection_filename, bool isBackground, vector<pthread_t>& threads, vector<int>& processIDs){
	if(redirect == ">"){
		int pid = fork();
		if(pid < 0) {
			cout << "cannot fork\n";
			return;
		} else if(pid == 0){ // child
			dup2(open(redirection_filename.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU), STDOUT_FILENO);
			execvp(args[0], args);
		}
		// parent
		if(isBackground){
			processIDs.push_back(pid);
		} else {
			waitpid(pid, NULL, 0);
		}
	} else {
		int fd[2];
		pipe(fd); // for communication between process and listener
		int pid = fork();
		if(pid < 0) {
			cout << "cannot fork\n";
			return;
		} else if(pid == 0){
			// child
			if(redirect == "<"){
				dup2(open(redirection_filename.c_str(), O_RDONLY), STDIN_FILENO);
			}
			dup2(fd[1], STDOUT_FILENO);
			execvp(args[0], args);
		}
		// parent
		close(fd[1]);
		pthread_t th;
		FILE * pipeFile = fdopen(fd[0], "r");
		pthread_create(&th, NULL, listener, (void*) pipeFile);
		
		if(!isBackground){
			wait(NULL);
			pthread_join(th, NULL);
		} else {
			processIDs.push_back(pid);
			threads.push_back(th);
		}
	}
}

void wait_all(vector<pthread_t>& threads, vector<int>& processIDs){
	for(unsigned int i=0; i<processIDs.size(); i++){
		waitpid(processIDs[i], NULL, 0);
	}
	processIDs.clear();
	for(unsigned int i=0; i<threads.size(); i++){
		pthread_join(threads[i], NULL);
	}
	threads.clear();
}

int main() {
	ifstream commands_file;
	commands_file.open("commands.txt");
	ofstream parse_txt;
	parse_txt.open("parse.txt");

	vector<pthread_t> threads;
	vector<int> processIDs;
	
	string line;
	while(getline(commands_file, line)){
		istringstream ss(line);
		string optional, command, inputs = "", options = "", redirection_filename = "", redirect = "-";
		bool isBackground = false;

		ss >> command;
		if(command == "wait"){
			parse_txt << "----------" << endl
			 	      << "Command: " << command << endl
				  	  << "----------" << endl;
			parse_txt.flush();
			wait_all(threads, processIDs);
			continue;
		}
		
		// parse arguments
		char * args[4] = {NULL, NULL, NULL, NULL};
		args[0] = strdup(command.c_str());
		for(int i=1; i <= 4; i++){
			ss >> optional;
			if(optional == "<" || optional == ">"){
				redirect = optional[0];
				ss >> redirection_filename;
			} else if(optional == "&"){
				isBackground = true;
			} else {
				if(optional[0] == '-'){
					options = optional;
				} else {
					inputs = optional;
				}
				args[i] = strdup(optional.c_str());
			}
		}

		parse_txt << "----------" << endl
			 	  << "Command: " << command << endl
				  << "Inputs: " << inputs << endl
				  << "Options: " << options << endl
				  << "Redirection: " << redirect << endl
				  << "Background Job: " << (isBackground ? "y" : "n") << endl
				  << "----------" << endl;
		parse_txt.flush();

		run_command(args, redirect, redirection_filename, isBackground, threads, processIDs);
	}
	
	wait_all(threads, processIDs);

	commands_file.close();
	parse_txt.close();
    
	return 0;
}