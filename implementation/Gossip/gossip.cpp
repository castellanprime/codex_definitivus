/**
* Name: OKUSANYA OLUWADAMILOLA
* Course: CSCI 512
* Due Date: December 9, 2015
*
* To complie, g++ -std=c++0x gossip.cpp -o gossip
*/


#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <iomanip>
#include <vector>
#include <random>


struct Node{
	bool i_status;  //infected status
	bool d_status;	//discouraged status

	bool can_infect();
	bool is_not_infected();
};

int regen(int max){
    std::random_device rd;
    std::mt19937 mersenne(rd());
    return  mersenne() % max;
}

void reset(Node grid[], int max){
	for (int n = 0; n < max; n++){
		grid[n].i_status = false;
		grid[n].d_status = false;
	}
}


bool Node::can_infect(){
	return (this->i_status && !this->d_status);
}

bool Node::is_not_infected(){
	return this->i_status == false;
}

void update_process(int MAX_OF_NODES, int k, Node network[]){
	for(int node = 0; node < MAX_OF_NODES; node++){
                if(network[node].can_infect()){
                    int node_to_infect = regen(MAX_OF_NODES);
                    if(network[node_to_infect].is_not_infected()){
                        network[node_to_infect].i_status = true;
                    }else{
                        if((rand() % k) == 0){
                            network[node].d_status = true;
                        }
                    }
                }
        }
}

int collate_infected(Node network[], int MAX_OF_NODES, int results){
	for (int n=0; n < MAX_OF_NODES; n++)
            if(network[n].i_status)
                results++;
	return results;
}

int main(int argc, char *argv[])
{

	if (argc != 3){
		std::cout << "Usage " << argv[0] << "[no of rounds]" << "[no of nodes to test]" << std::endl;
		exit(1);
	}

	std::cout << "Gossip simulation started" << std::endl;

	int MAX_OF_NODES = atoi(argv[2]);
	int MAX_STEPS = atoi(argv[1]);

	Node network[MAX_OF_NODES];
	srand(time(NULL));
	std::vector<int> final_i, final_c;	// containers to hold results for infected and clean nodes respectively		
	int random_node = 0;

    // Initialize network of nodes
    reset(network, MAX_OF_NODES);
    
    // Begin simulation
    for(int k = 1; k < 9; k++){
    	std::cout << "Starting sim for k = " << k << std::endl;
        random_node = regen(MAX_OF_NODES);
        network[random_node].i_status = true;
    
        for(int no_of_steps = 0; no_of_steps < MAX_STEPS; no_of_steps++ )
		update_process(MAX_OF_NODES, k, network);

        //get data
       	int count = 0, temp_c_results = 0, temp_i_results;
  
        temp_i_results = collate_infected(network, MAX_OF_NODES, count);
        
        temp_c_results = MAX_OF_NODES - temp_i_results;
        final_i.push_back(temp_i_results);
        final_c.push_back(temp_c_results);
  
      	reset(network, MAX_OF_NODES);
    
        std::cout << "End sim for k =  " << k << std::endl;
    }

	// Collate and print data
	std::cout << "Final results " << std::endl;
        std::cout << "k" << std::setw(12) << "Clean" << std::setw(12) << "Infected" << std::endl;
        for(int a = 1, b = 0, c = 0; a < 9, b < final_i.size(), c < final_c.size(); ++b, ++c)
            std::cout << a++ << std::setw(12) << final_c.at(c) << std::setw(12) << final_i.at(b) << std::endl;
    
    return 0;
}
