/**
* @file: join.cpp
* @author: Okusanya Oluwadamilola
* @date: March 20, 2015
* @brief:  An efficient join algorithm
*   Execute an efficient join algorithm in C/C++ for this query	
*   SELECT V.eLevel, count(*), sum(X.fee), avg(X.fee)
*   FROM Veternians V, Examine X, Dogs D
*   WHERE V.vid = X.vid AND X.did = D.did AND D.age > 10
*   GROUP BY V.eLevel;
* 
*   The tables are
*   Veternians(vid:integer, eLevel:integer);
*   Examine(vid:integer, did:integer, fee:integer);
*   Dogs(did:integer, age:integer);
* 
* @bugs: None so far
* @improvements: Need to improve times for extremely constrained memory
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <vector>



// Constants 
static char filename_new[40];
static char filename[40];
static std::set<long> didSet;

// Relation class
typedef struct Relation{
	long eVid;
	long eDid;
	int eFee;	
} Relation;



// To_string methods , one for each class
std::string relation_to_string(Relation relation){
	char line[40];	
	sprintf(line, "%ld %ld %d", relation.eVid, relation.eDid,
			relation.eFee);
	std::string str(line);
	return str;
}


// First step on the query execution plan
// A scan of the Dogs table
void tablescan_for_Dogs(){	
	int dAge;
	long dDid;

	std::ifstream infile("DFile");

	int count = 0;
	if (infile.is_open()){
		while (infile >> dDid >> dAge){
			if (dAge > 10){
				didSet.insert(dDid);				
			}
		}

	} else {
		std::cout << "File could not be opened";
	}
		
	infile.close();
}


// Second step on the query execution plan
// Retrieve all tuples where Examine.did == Dogs.did
void hash_join_Examine_Dogs(){

	std::ifstream infile("EFile");
	strcpy(filename_new, "File");
	strcat(filename_new, "_ex");		
	std::ofstream outfile(filename_new);

	long eDid, eVid;
	int eFee;

	std::set<long>::iterator it;
	if (infile.is_open()){
		while (infile >> eVid >> eDid >> eFee){
			if (*(it = didSet.find(eDid)) == eDid){
				outfile << eVid << " "<< eDid 
					<< " "<< eFee << std::endl;
			}
		}
	} else {
		std::cout << "Files could not be opened";
	}	

	infile.close();
	outfile.close();

	didSet.clear();

}


// Third step on the query execution plan
// Retrieve all tuples where Veternians.did == Examine.did
void hash_join_Veternians_Examine(){
	
	std::unordered_map<int, Relation> m;
	
	std::ifstream infile(filename_new);
	std::ifstream midfile("VFile");
	strcpy(filename, filename_new);
	strcat(filename, "_v");		
	std::ofstream outfile(filename);

	char lines[120];
	long eDid, eVid, vVid;
	int eFee, eLevel;
	
	if (infile.is_open() && midfile.is_open()){
		while (infile >> eVid >> eDid >> eFee){
			Relation relation;
			relation.eVid = eVid;
			relation.eDid = eDid;
			relation.eFee = eFee;
			m[eVid] = relation;
		}
	
		while (midfile >> vVid >> eLevel ){
			if (m.find(vVid) != m.end()){
				sprintf(lines, "%ld %d %s\n", vVid, eLevel, 
					relation_to_string(m[vVid]).c_str());
				std::string str(lines);
				outfile << str;
			}
		}
	} else{
		std::cout << "Files could not be opened";
	}

	infile.close();
	midfile.close();
	outfile.close();

	m.clear();

}

// Final step on the query execution plan
// Retrieve all tuples with fields 
// Veternians.eLevel, count(*), sum(Examine.fee), avg(Examine.fee)
void select(){
	
	long vVid, eVid, eDid;
	int eLevel, eFee, tempSum = 0, recordCount = 0;

	char filename2[40];	
	std::ifstream infile(filename);
	/**
	strcpy(filename2, filename);
	strcat(filename2, "_fin");
	std::ofstream outfile(filename2);
	strcpy(filename_new, filename2);
	*/

	std::multimap<int, int> m;
	if (infile.is_open()){
		while (infile >> vVid >> eLevel >> eVid >> eDid >> eFee){
			m.insert(std::make_pair(eLevel, eFee));	
		}

		std::cout << "V.eLevel" << "\t" << "count" << "\t" 
		   << "sum" << "\t" << "avg" << std::endl;
		for (int i = 1; i < 5; i++){
			std::pair<std::multimap<int, int>::iterator, 
					std::multimap<int, int>::iterator> ret;
			ret = m.equal_range(i);
			for (std::multimap<int, int>::iterator it= ret.first; 
					it!=ret.second; ++it){
				recordCount++;
				tempSum += it->second;
			}
			
			std::cout << i << "\t\t"<< recordCount <<  "\t"<< tempSum 
					<<  "\t" << std::setprecision(4) 
					<< (tempSum / (float)recordCount) << std::endl;	
			recordCount = 0;
			tempSum = 0;
		}			
	} else{
		std::cout << "Files could not be opened";
	}

	infile.close();
	m.clear();
	
}


int main(void){	
		
	tablescan_for_Dogs();

	hash_join_Examine_Dogs();
		
	hash_join_Veternians_Examine();

	select();
	
}
