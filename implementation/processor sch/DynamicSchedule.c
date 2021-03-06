/**
* Name: Okusanya Oluwadamilola
* Program: An implementation of the Tomasulo and Scoreboarding
* algorithms
* Date: Oct 20, 2015
**/


#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <queue>
#include <map>

using namespace std;

// MPIS64 instruction set
string r_inst[] = { "ADD.D", "SUB.D", "MUL.D", "DIV.D", "DADD"};
string i_inst[] = { "L.D", "S.D", "DADDI","DADDUI","DSUBI","BNEZ", "BEQZ" };
string j_inst[] = { };

set<string> r_set(r_inst, r_inst + 5);
set<string> i_set(i_inst, i_inst + 7);
set<string> j_set(j_inst, j_inst + 0);

//array of instructions
string add_array[] = { "ADD.D", "SUB.D", "DADD" };
string mult_array[] = { "MUL.D", "DIV.D", };
string load_array[] = { "L.D","DADDI", "DADDUI", "DSUBI"};
string store_array[] = { "S.D" };
string branch_array[] = { "BNEZ", "BEQZ" };

set<string> adderOpcs(add_array, add_array + 3);
set<string> multOpcs(mult_array, mult_array + 2);
set<string> storeOpcs(store_array, store_array + 1);
set<string> loadOpcs(load_array, load_array + 4);
set<string> branchOpcs(branch_array, branch_array + 2);

// instruction struct
struct Instruction {
	string opCode;
	string iStr;
	int rs;
	int rt;
	int rd;
	int imm;
	string goToLoop;
	int Offset;
	char type;
	int issue;
	int readOperands;
	int execute;
	int write;
	string loopLabel;
};

// scheduler struct
struct DynScheduler {
	int latency_fadd;
	int latency_fmul;
	int latency_fload;
	int latency_fdiv;
	int latency_ldint;
	int latency_sdint;

	int instructionCount;
	vector<Instruction> InstructionQueue;

	//Reservation Station
	vector<Instruction> RSLoad;
	vector<Instruction> RSAdd;
	vector<Instruction> RSMult;
	vector<Instruction> RSDiv;
	//vector<RStation> RSStore;

	Instruction instanceLdSt;
	Instruction instanceAdd;
	Instruction instanceMult;
	Instruction instanceDivide;
	map<int, int> operandsMap;
};

//read latencies from the file
void readLatency(struct DynScheduler &scheduler, string filename) {
	ifstream instFile(filename.c_str()); ////"620_proj_latencies"
	string line;
	int count;
	if (instFile.is_open()) {
		while (getline(instFile, line)) {
			//cout << "---------" << line << "---------" << '\n';
			istringstream iss(line);
			string producer, consumer;
			int latency;
			iss >> producer >> consumer >> latency;
			//cout << producer << " :" << consumer << ":" << latency << endl;
			if (producer == "FPMUL")
				scheduler.latency_fmul = latency;
			else if (producer == "FPDIV")
				scheduler.latency_fdiv = latency;
			else if (producer == "FPADD")
				scheduler.latency_fadd = latency;
			else if (producer == "FPLD")
				scheduler.latency_fload = latency;
			else if (producer == "LDINT")
				scheduler.latency_ldint = latency;
			else if (producer == "INT")
				scheduler.latency_sdint = latency;
		}
	}
	instFile.close();
}

//fill values for each instruction
void fillInstr(struct Instruction &instr, char a, vector<string> v,
		string i_str) {
	instr.rs = -1;
	instr.rt = -1;
	instr.rd = -1;
	instr.imm = -1;
	instr.Offset = -1;

	instr.type = a;
	instr.issue = -1;
	instr.readOperands = -1;
	instr.execute = -1;
	instr.write = -1;
	instr.iStr = i_str;

	switch (a) {
	case 'r':
		v[1].replace(0, 1, "");
		v[3].replace(0, 1, "");
		v[2].replace(0, 1, "");
		instr.opCode = v[0];
		instr.rd = atoi(v[1].c_str());
		instr.rs = atoi(v[2].c_str());
		instr.rt = atoi(v[3].c_str());
		break;
	case 'i':
		if (branchOpcs.count(v[0].c_str())) {
			instr.opCode = v[0];
			v[1].replace(0, 1, "");
			instr.rs = atoi(v[1].c_str());
			instr.rd = 0;
			instr.goToLoop = v[2];
		} else {
			v[1].replace(0, 1, "");
			v[3].replace(0, 1, "");
			instr.opCode = v[0];
			instr.rt = atoi(v[1].c_str());
			instr.rs = atoi(v[3].c_str());
			instr.rd = 0;
			instr.imm = atoi(v[2].c_str());
		}
		break;
	case 'j':
		instr.opCode = v[0];
		instr.Offset = atoi(v[1].c_str());
		break;
	}
}

void printInstr(struct Instruction istr) {
	//cout << "istr.type=" << istr.type << endl;
	switch (istr.type) {
	case 'r':
		cout << setw(35) << istr.iStr << setw(8) << istr.issue << setw(8)
				<< istr.execute << setw(8) << istr.write << endl;
		break;
	case 'i':
		cout << setw(35) << istr.iStr << setw(8) << istr.issue << setw(8)
				<< istr.execute << setw(8) << istr.write << endl;
		break;
	case 'j':
		cout << setw(35) << istr.iStr << setw(8) << istr.issue << setw(8)
				<< istr.execute << setw(8) << istr.write << endl;
		break;
	}
}

void InstructionParser(string &s, vector<Instruction> &iQ) {
	vector < string > res;
	string tok;
	stringstream ss(s);
	string loopLabel;
	struct Instruction instr;
	while (ss >> tok) {
		//cout << "tok=" << tok << endl;
		res.push_back(tok);
		if (ss.peek() == ' ') {
			ss.ignore();
		}
	}
	if (res[0] == "loop") {
		instr.loopLabel = res[0];
		res.erase(res.begin());
	}

	cout << "res[0] is : " << res[0] << endl;
	if (r_set.count(res[0])) {
		cout << "This instruction is of R type: " << s << endl;
		fillInstr(instr, 'r', res, s);
	} else if (i_set.count(res[0])) {
		cout << "This instruction is of I type: " << s << endl;
		fillInstr(instr, 'i', res, s);
	} else if (j_set.count(res[0])) {
		cout << "This instruction is of J type: " << s << endl;
		fillInstr(instr, 'j', res, s);
	} else {
		cout << " Invalid Opcode" << endl;
	}
	iQ.push_back(instr);
}

void printAllInstStatus(struct DynScheduler &scheduler) {
	cout << setw(35) << "Instr" << setw(8) << "issue" << setw(8) << "exec"
			<< setw(8) << "write" << endl;

	for (int r = 0; r < scheduler.InstructionQueue.size(); r++) {
		printInstr(scheduler.InstructionQueue[r]);
	}
}

void printScoreboardInstr(struct Instruction istr) {
	cout << setw(35) << istr.iStr << setw(8) << istr.issue << setw(8)
			<< istr.readOperands << setw(8) << istr.execute << setw(8)
			<< istr.write << endl;
}

void printScoreBoard(struct DynScheduler &scheduler) {
	cout << setw(35) << "Instr" << setw(8) << "issue" << setw(8) << "read"
			<< setw(8) << "exec" << setw(8) << "write" << endl;

	for (int r = 0; r < scheduler.InstructionQueue.size(); r++) {
		printScoreboardInstr(scheduler.InstructionQueue[r]);
	}
}

void ReplaceStringInPlace(std::string& subject, const std::string& search,
		const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

void initializeInst(struct Instruction &inst) {
	inst.issue = -1;
	inst.readOperands = -1;
	inst.execute = -1;
	inst.write = -1;
}

void initializeScheduler(struct DynScheduler &scheduler) {
	scheduler.instructionCount = 0;

	//setup the instance of each type
	//Instruction addInstance, ldStInstance, multInstance, divInstance;
	initializeInst(scheduler.instanceAdd);
	initializeInst(scheduler.instanceDivide);
	initializeInst(scheduler.instanceLdSt);
	initializeInst(scheduler.instanceMult);

	// setup Reservation station
	struct Instruction rs1, rs2, rs3, rs4, rs5, rs6, rs7, rs8;
	initializeInst(rs1);
	scheduler.RSLoad.push_back(rs1);

	initializeInst(rs2);
	scheduler.RSLoad.push_back(rs2);

	initializeInst(rs3);
	scheduler.RSAdd.push_back(rs3);

	initializeInst(rs4);
	scheduler.RSAdd.push_back(rs4);

	initializeInst(rs5);
	scheduler.RSMult.push_back(rs5);

	initializeInst(rs6);
	scheduler.RSMult.push_back(rs6);

	initializeInst(rs7);
	scheduler.RSDiv.push_back(rs7);

	initializeInst(rs8);
	scheduler.RSDiv.push_back(rs8);

	string filename;
	cout << "Enter the latency file along with it's path" << endl;
	cin >> filename;
	readLatency(scheduler, filename);

	cout << "Enter the instruction file along with it's path" << endl;
	cin >> filename;
	ifstream instFile(filename.c_str());	//"620_proj_isyntax"
	string s;
	if (instFile.is_open()) {
		while (getline(instFile, s)) {
			//cout << "---------" << line << "---------" << '\n';
			ReplaceStringInPlace(s, ",", " ");
			ReplaceStringInPlace(s, "#", "");
			ReplaceStringInPlace(s, ":", "");
			ReplaceStringInPlace(s, "(", " ");
			ReplaceStringInPlace(s, ")", "");
			InstructionParser(s, scheduler.InstructionQueue);
			scheduler.instructionCount++;
		}
	} else
		cout << "Unable to open file" << endl;
	instFile.close();
}

void noPipeLine(struct DynScheduler &scheduler, int pc, int cc) {
	Instruction inst = scheduler.InstructionQueue[pc];

	switch (inst.type) {
	case 'r':
		if (adderOpcs.count(inst.opCode) || multOpcs.count(inst.opCode)) {
			Instruction instOfInstance;
			if (adderOpcs.count(inst.opCode)) {
				instOfInstance = scheduler.instanceAdd;
			} else if (inst.opCode == mult_array[0]) { //multiply
				instOfInstance = scheduler.instanceMult;
			} else if (inst.opCode == mult_array[1]) { //div
				instOfInstance = scheduler.instanceDivide;
			}

			//check the issue No of the previous instruction and the last add/sub instruction
			Instruction preInst = scheduler.InstructionQueue[pc - 1];
			if (preInst.issue >= instOfInstance.issue) {
				inst.issue = preInst.issue + 1;
			} else {
				inst.issue = instOfInstance.issue + 1;
			}

			//check the cc of the operands one by one
			int lastCC = instOfInstance.write;            //18
			int rs_lastCC = scheduler.operandsMap[inst.rs];            //40
			int rt_lastCC = scheduler.operandsMap[inst.rt];            //12

			//decide issue
			if (preInst.issue > lastCC)
				inst.issue = preInst.issue + 1;
			else
				inst.issue = lastCC + 1;

			//decide read operand
			inst.readOperands = inst.issue + 1;
			if (rs_lastCC > inst.issue)
				inst.readOperands = rs_lastCC + 1;

			if (rt_lastCC > inst.issue && rt_lastCC > rs_lastCC)
				inst.readOperands = rt_lastCC + 1;

			//inst.readOperands = lastCC + 1;
			inst.execute = inst.readOperands + 1;
			if (adderOpcs.count(inst.opCode)) {
				inst.write = inst.execute + scheduler.latency_fadd;
				scheduler.instanceAdd = inst;
			} else if (inst.opCode == mult_array[0]) { //multiply
				inst.write = inst.execute + scheduler.latency_fmul;
				scheduler.instanceMult = inst;
			} else if (inst.opCode == mult_array[1]) { //div
				inst.write = inst.execute + scheduler.latency_fdiv;
				scheduler.instanceDivide = inst;
			}

		}
		scheduler.InstructionQueue[pc] = inst;
		scheduler.operandsMap[inst.rd] = inst.write;
		break;
	case 'i':
		// load and store
		if (loadOpcs.count(inst.opCode)) {
			Instruction instOfInstance = scheduler.instanceLdSt;

			//the first one
			if (instOfInstance.issue == -1) {
				inst.issue = 1;
				inst.readOperands = 2;
				inst.execute = inst.readOperands + 1;
				inst.write = inst.execute + scheduler.latency_ldint;
				scheduler.instanceLdSt = inst;
			} else {
				//check the issue No of the previous instruction and the last LD or SD instruction
				Instruction preInst = scheduler.InstructionQueue[pc - 1];
				if (preInst.issue >= instOfInstance.write) {
					inst.issue = preInst.issue + 1;
				} else {
					inst.issue = instOfInstance.write + 1;
				}
				inst.readOperands = inst.issue + 1;
				inst.execute = inst.issue + 2;
				inst.write = inst.execute + scheduler.latency_ldint;
				scheduler.instanceLdSt = inst;
			}

		} else if (storeOpcs.count(inst.opCode)
				|| branchOpcs.count(inst.opCode)) {
			//check the issue No of the previous instruction and the last LD or SD instruction
			Instruction instOfInstance = scheduler.instanceLdSt;
			Instruction preInst = scheduler.InstructionQueue[pc - 1];

			if (preInst.issue > instOfInstance.issue) {
				inst.issue = preInst.issue + 1;
			} else {
				inst.issue = instOfInstance.issue + 1;
			}

			//check the cc of the operand
			//cout << "preInst.rd -> preInst.rs -> preInst.rt : " << preInst.rd << " -> " << preInst.rs << " -> " << preInst.rt<< endl;
			//cout << "inst.rd -> inst.rs -> inst.rt : " << inst.rd << " -> " << inst.rs << " -> " << inst.rt<< endl;
			if(inst.rt != preInst.rt && preInst.type == 'i')
				inst.readOperands = preInst.readOperands + 1;
			else
				inst.readOperands = preInst.write + 1;
			inst.execute = inst.readOperands + 1;
			inst.write = inst.execute + 1;
			scheduler.instanceLdSt = inst;
		}
		scheduler.InstructionQueue[pc] = inst;
		scheduler.operandsMap[inst.rt] = inst.write;
		break;
	case 'j':
		break;
	}
}

void pipeLine(struct DynScheduler &scheduler, int pc, int cc) {
	Instruction inst = scheduler.InstructionQueue[pc];
	switch (inst.type) {
	case 'r':
		if (adderOpcs.count(inst.opCode) || multOpcs.count(inst.opCode)) {
			Instruction instOfInstance1, instOfInstance2;
			if (adderOpcs.count(inst.opCode)) {
				instOfInstance1 = scheduler.RSAdd[0];
				instOfInstance2 = scheduler.RSAdd[1];
			} else if (inst.opCode == mult_array[0]) { //multiply
				instOfInstance1 = scheduler.RSMult[0];
				instOfInstance2 = scheduler.RSMult[1];
			} else if (inst.opCode == mult_array[1]) { //div
				instOfInstance1 = scheduler.RSDiv[0];
				instOfInstance2 = scheduler.RSDiv[1];
			}

			//check the issue No of the previous instruction and the last add/sub instruction
			Instruction preInst = scheduler.InstructionQueue[pc - 1];
			if (preInst.issue >= instOfInstance1.issue
					&& preInst.issue >= instOfInstance2.issue) {
				inst.issue = preInst.issue + 1;
			} else {
				if (instOfInstance1.issue > preInst.issue)
					inst.issue = instOfInstance1.issue + 1;

				if (instOfInstance2.issue > preInst.issue)
					inst.issue = instOfInstance2.issue + 1;
			}

			//check the cc of the operands one by one
			int lastCC;
			if (instOfInstance1.issue > instOfInstance2.issue) {
				lastCC = instOfInstance1.issue;
			} else {
				lastCC = instOfInstance2.issue;
			}

			int rs_lastCC = scheduler.operandsMap[inst.rs]; //40
			int rt_lastCC = scheduler.operandsMap[inst.rt]; //12

			//decide issue： step 1 - > check the previous instruction
			if (preInst.issue > lastCC)
				inst.issue = preInst.issue + 1;
			else
				inst.issue = lastCC + 1;

			//decide issue： step 2 -> check the RS
			if (instOfInstance1.write != -1 && instOfInstance2.write != -1) {
				if (instOfInstance1.write < instOfInstance2.write) {
					if (inst.issue <= instOfInstance1.write)
						inst.issue = instOfInstance1.write + 1;
				} else {
					if (inst.issue <= instOfInstance2.write)
						inst.issue = instOfInstance2.write + 1;
				}
			}

			//decide exe cc
			inst.execute = inst.issue + 1;
			if (rs_lastCC > inst.issue)
				inst.execute = rs_lastCC + 1;

			if (rt_lastCC > inst.issue && rt_lastCC > rs_lastCC)
				inst.execute = rt_lastCC + 1;

			if (adderOpcs.count(inst.opCode)) {
				inst.write = inst.execute + scheduler.latency_fadd;

				if (instOfInstance1.write != -1
						&& instOfInstance2.write != -1) {
					if (instOfInstance1.write > instOfInstance2.write)
						scheduler.RSAdd[1] = inst;
					else
						scheduler.RSAdd[0] = inst;
				} else {
					if (instOfInstance1.write == -1)
						scheduler.RSAdd[0] = inst;
					else if (instOfInstance2.write == -1)
						scheduler.RSAdd[1] = inst;
				}
			} else if (inst.opCode == mult_array[0]) { //multiply
				inst.write = inst.execute + scheduler.latency_fmul;
				if (instOfInstance1.write != -1
						&& instOfInstance2.write != -1) {
					if (instOfInstance1.write > instOfInstance2.write)
						scheduler.RSMult[1] = inst;
					else
						scheduler.RSMult[0] = inst;
				} else {
					if (instOfInstance1.write == -1)
						scheduler.RSMult[0] = inst;
					else if (instOfInstance2.write == -1)
						scheduler.RSMult[1] = inst;
				}
			} else if (inst.opCode == mult_array[1]) { //div
				inst.write = inst.execute + scheduler.latency_fdiv;

				if (instOfInstance1.write != -1
						&& instOfInstance2.write != -1) {
					if (instOfInstance1.write > instOfInstance2.write)
						scheduler.RSDiv[1] = inst;
					else
						scheduler.RSDiv[0] = inst;
				} else {
					if (instOfInstance1.write == -1)
						scheduler.RSDiv[0] = inst;
					else if (instOfInstance2.write == -1)
						scheduler.RSDiv[1] = inst;
				}
			}
		}
		scheduler.InstructionQueue[pc] = inst;
		scheduler.operandsMap[inst.rd] = inst.write;
		break;
	case 'i':
		// load and store
		if (loadOpcs.count(inst.opCode)) {
			Instruction instOfInstance1, instOfInstance2;
			instOfInstance1 = scheduler.RSLoad[0];
			instOfInstance2 = scheduler.RSLoad[1];

			//the first one
			if (instOfInstance1.issue == -1 && instOfInstance2.issue == -1) {
				inst.issue = 1;
				inst.execute = 2;
				inst.write = inst.execute + scheduler.latency_ldint;
				scheduler.RSLoad[0] = inst;

			} else {
				//check the issue No of the previous instruction and the last LD or SD instruction
				Instruction preInst = scheduler.InstructionQueue[pc - 1];

				//RS1 and RS2 are busy
				if (instOfInstance1.write != -1
						&& instOfInstance2.write != -1) {
					if (instOfInstance1.write > instOfInstance2.write)
						inst.issue = instOfInstance2.write + 1;
					else
						inst.issue = instOfInstance1.write + 1;

				} else if (instOfInstance1.write != -1
						|| instOfInstance2.write != -1) {
					inst.issue = preInst.issue + 1;
				}

				if (inst.issue < preInst.issue)
					inst.issue = preInst.issue + 1;

				inst.execute = inst.issue + 1;
				inst.write = inst.execute + scheduler.latency_ldint;

				if (instOfInstance1.write != -1
						&& instOfInstance2.write != -1) {
					if (instOfInstance1.write > instOfInstance2.write)
						scheduler.RSLoad[1] = inst;
					else
						scheduler.RSLoad[0] = inst;
				} else {
					if (instOfInstance1.write == -1)
						scheduler.RSLoad[0] = inst;
					else if (instOfInstance2.write == -1)
						scheduler.RSLoad[1] = inst;
				}
			}

		} else if (storeOpcs.count(inst.opCode)
				|| branchOpcs.count(inst.opCode)) {
			//check the issue No of the previous instruction and the last LD or SD instruction
			Instruction instOfInstance1, instOfInstance2;
			instOfInstance1 = scheduler.RSLoad[0];
			instOfInstance2 = scheduler.RSLoad[1];
			Instruction preInst = scheduler.InstructionQueue[pc - 1];

			//cout << " preInst.issue : " << preInst.issue << endl;
			//cout << " instOfInstance1.write : " << instOfInstance1.write << endl;
			//cout << " instOfInstance2.write : " << instOfInstance2.write << endl;
			if (preInst.issue >= instOfInstance1.write
					|| preInst.issue >= instOfInstance2.write) {
				inst.issue = preInst.issue + 1;
			} else {
				if (instOfInstance1.issue >= preInst.issue)
					inst.issue = instOfInstance1.issue + 1;

				else if (instOfInstance2.issue >= preInst.issue)
					inst.issue = instOfInstance2.issue + 1;
			}

			//check the cc of the operand
			//cout << " inst.issue : " << inst.issue << endl;
			inst.execute = inst.issue + 1;
			inst.write = preInst.write + 1;
			if (instOfInstance1.write != -1 && instOfInstance2.write != -1) {
				if (instOfInstance1.write > instOfInstance2.write)
					scheduler.RSLoad[1] = inst;
				else
					scheduler.RSLoad[0] = inst;
			} else {
				if (instOfInstance1.write == -1)
					scheduler.RSLoad[0] = inst;
				else if (instOfInstance2.write == -1)
					scheduler.RSLoad[1] = inst;
			}
		}
		scheduler.InstructionQueue[pc] = inst;
		scheduler.operandsMap[inst.rt] = inst.write;
		break;
	case 'j':
		break;
	}
}

int main(int argc, char const *argv[]) {
	int clock_cycle = 1; 										//clock cycle
	int program_counter = 0;
	struct DynScheduler scheduler;
	initializeScheduler(scheduler);
	string answer;

	cout << " Do you want pipelining ? y/n" << endl;
	cin >> answer;

	cout << "pc = " << program_counter << " and scheduler.instructionCount="
			<< scheduler.instructionCount << endl;
	if (answer == "n" || answer == "N") {
		while (program_counter < scheduler.instructionCount) {
			noPipeLine(scheduler, program_counter, clock_cycle);
			program_counter++;
		}
		printScoreBoard(scheduler);
	} else if (answer == "y" || answer == "Y") {
		cout << "------Start to pipepline the instructions ------" << endl;
		while (program_counter < scheduler.instructionCount) {
			// at each CC attempt to issue a new instruction a location pc in
			pipeLine(scheduler, program_counter, clock_cycle);
			program_counter++;
		}
		printAllInstStatus(scheduler);
	}
	else if (answer == "b" || answer == "b"){
		while (program_counter < scheduler.instructionCount) {
			noPipeLine(scheduler, program_counter, clock_cycle);
			program_counter++;
		}
		printAllInstStatus(scheduler);
	} else if (answer == "b" || answer == "b"){

	
	} else{
		cout << "Wrong character" << endl;
		exit(0);	
	} 
	return 0;
}
