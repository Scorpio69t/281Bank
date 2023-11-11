// Project Identifier: 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
#include <getopt.h>
#include <fstream>

#include "bank.h"

int main(int argc, char* argv[]){
    std::ios_base::sync_with_stdio(false);
    Bank bank;
    try{
        bank.get_options(argc, argv);
        bank.read_clients();
        bank.run();
        bank.query();
    }catch(std::runtime_error& e){
        std::cerr<<e.what()<<std::endl;
        exit(1);
    }
    return 0;
    
}

void Bank::get_options(int argc, char** argv){
    int option_index = 0, option = 0;
    opterr = false;
    struct option longOpts[] = {{"help", no_argument, nullptr, 'h'},
                                {"verbose", no_argument, nullptr, 'v'},
                                {"file", required_argument, nullptr, 'f'}};
    while((option = getopt_long(argc, argv, "hvf:", longOpts, &option_index)) != -1){
        switch (option)
        {
        case 'h':
            std::cout << "This program reads a TXT file that describes details of all clients of the bank\n"
                      << "and a TXT file recording all transactions during a period.\n"
                      << "Have fun with our program!\n"
                      <<  "Usage: \'./bank\n\t[--verbose | -v]\n"
                      <<                      "\t[--file <TXT Registration File> | -f]\n"
                      <<                      "\t[--help | -h]\n"
                      <<                      "\t< <TXT Command File>\n"
                      <<                      "\t> <TXT Output File>\n" << std::endl;
            exit(0);
        
        case 'v':
            verb = true;
            break;

        case 'f':
            regFileName = optarg;
            break;
        
        default:
            throw std::runtime_error("Invalid command line options!\nExiting...");
            break;
        }
    }
}

void Bank::read_clients(){
    std::ifstream file(regFileName);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string name;
            std::string part;
            int idx = 0;
            info inf;
            while (std::getline(iss, part, '|')) {
                switch (idx)
                {
                case 0:
                    str2TS(part, inf.tS);
                    break;
                
                case 1:
                    name = part;
                    break;
                
                case 2:
                    inf.pin = std::stoi(part);
                    break;

                case 3:
                    inf.balance = std::stoi(part);

                default:
                    break;
                }
                ++idx;
            }
            clients[name] = inf;
        }

        file.close();
    } else {
        throw std::runtime_error("Registration file failed to open.\n");
    }
}

void Bank::run(){
    std::string line;
    timeStamp currplaceTS{0,0,0,0,0,0};
    while(std::getline(std::cin, line)){
        if(line[0] == '#') continue;
        if(line[0] == '$') break;
        std::istringstream iss(line);
        std::string word;
        iss >> word;
        if(word[0] == 'l'){
            std::string usrName;
            int usrPin;
            iss >> usrName;
            iss >> word;
            usrPin = stoi(word);
            if(clients.count(usrName) == 0 || clients[usrName].pin != usrPin){
                if(verb) std::cout<<"Failed to log in "<<usrName<<".\n";
            }else{
                if(verb) std::cout<<"User "<<usrName<<" logged in.\n";
                iss >> word;
                clientIPs[usrName].insert(word);
            }
        }
        else if(word[0] == 'o'){
            std::string usrName;
            std::string usrIP;
            iss >> usrName;
            iss >> usrIP;
            if(clientIPs.count(usrName) == 0 || clientIPs[usrName].count(usrIP) == 0){
                if(verb) std::cout<<"Failed to log out "<<usrName<<".\n";
            }else{
                if(verb) std::cout<<"User "<<usrName<<" logged out.\n";
                clientIPs[usrName].erase(usrIP);
                if(clientIPs[usrName].empty()) clientIPs.erase(usrName);
            }
        }
        else if(word[0] == 'p'){
            timeStamp placeTS;
            iss >> word;
            str2TS(word, placeTS);
            if(!cmpTS(currplaceTS, placeTS)) throw std::runtime_error("Invalid decreasing timestamp in 'place' command.");
            trans newTransaction;
            newTransaction.place = placeTS;
            std::string senderIP;
            iss >> senderIP;
            iss >> newTransaction.sender;
            iss >> newTransaction.receiver;
            iss >> word;
            newTransaction.amount = stoi(word);
            iss >> word;
            str2TS(word, newTransaction.execute);
            if(!cmpTS(placeTS, newTransaction.execute)) throw std::runtime_error("You cannot have an execution date before the current timestamp.");
            iss >> newTransaction.type;
            if(!checkThreeDays(newTransaction.place, newTransaction.execute)){
                if(verb) std::cout<<"Select a time less than three days in the future.\n";
                continue;
            }
            if(clients.count(newTransaction.sender) == 0){
                if(verb) std::cout<<"Sender "<<newTransaction.sender<<" does not exist.\n";
                continue;
            }
            if(clients.count(newTransaction.receiver) == 0){
                if(verb) std::cout<<"Recipient "<<newTransaction.receiver<<" does not exist.\n";
                continue;
            }
            if(cmpTS(newTransaction.execute, clients[newTransaction.sender].tS) || cmpTS(newTransaction.execute, clients[newTransaction.receiver].tS)){
                if(verb) std::cout<<"At the time of execution, sender and/or recipient have not registered.\n";
                continue;
            }
            if(clientIPs.count(newTransaction.sender) == 0){
                if(verb) std::cout<<"Sender "<<newTransaction.sender<<" is not logged in.\n";
            }
            if(clientIPs[newTransaction.sender].count(senderIP) == 0){
                if(verb) std::cout<<"Fraudulent transaction detected, aborting request.\n";
                continue;
            }
            currplaceTS = placeTS;
            while(!queuedTrans.empty() && cmpTS(queuedTrans.top().execute, currplaceTS)){
                transaction();
            }
            newTransaction.id = trTot;
            ++trTot;
            queuedTrans.push(newTransaction);
            if(verb){
                std::cout<<"Transaction placed at "<<ts2Int(newTransaction.place)<<": $"<<newTransaction.amount<<" from "<<newTransaction.sender<<" to "<<newTransaction.receiver<<" at "<<ts2Int(newTransaction.execute)<<".\n";
            }
        }
    }
    while(!queuedTrans.empty()){
        transaction();
    }
}

void Bank::query(){
    std::string line;
    while(std::getline(std::cin, line)){
        std::istringstream iss(line);
        std::string word;
        iss >> word;
        if(word[0] == 'l'){
            timeStamp start;
            timeStamp end;
            iss >> word;
            str2TS(word, start);
            iss >> word;
            str2TS(word, end);
            std::vector<trans>::iterator st = std::lower_bound(transactions.begin(), transactions.end(), start, CompareByTS());
            std::vector<trans>::iterator ed = std::lower_bound(transactions.begin(), transactions.end(), end, CompareByTS());
            for(std::vector<trans>::iterator i = st; i != ed; ++i){
                if(i->amount == 1) {std::cout<<i->id<<": "<<i->sender<<" sent 1 dollar to "<<i->receiver<<" at "<<ts2Int(i->execute)<<".\n";}
                else std::cout<<i->id<<": "<<i->sender<<" sent "<<i->amount<<" dollars to "<<i->receiver<<" at "<<ts2Int(i->execute)<<".\n";
            }
            if((ed - st) == 1) {std::cout<<"There was 1 transaction that was placed between time "<<ts2Int(start)<<" to "<<ts2Int(end)<<".\n";}
            else {std::cout<<"There were "<<ed-st<<" transactions that were placed between time "<<ts2Int(start)<<" to "<<ts2Int(end)<<".\n";}
        }
        else if(word[0] == 'r'){
            timeStamp start;
            timeStamp end;
            iss >> word;
            str2TS(word, start);
            iss >> word;
            str2TS(word, end);
            std::vector<trans>::iterator st = std::lower_bound(transactions.begin(), transactions.end(), start, CompareByTS());
            std::vector<trans>::iterator ed = std::lower_bound(transactions.begin(), transactions.end(), end, CompareByTS());
            uint32_t total = 0;
            for(std::vector<trans>::iterator i = st; i != ed; ++i){
               total += (i->senderFee + i->receiverFee);
            }
            std::cout<<"281Bank has collected "<<total<<" dollars in fees over";
            uint64_t duration = ts2Int(end) - ts2Int(start);
            if(duration / 10000000000 > 1) std::cout<<" "<<duration / 10000000000<<" years";
            else if(duration / 10000000000 == 1) std::cout<<" 1 year";
            duration = duration % 10000000000;
            if(duration / 100000000 > 1) std::cout<<" "<<duration / 100000000<<" months";
            else if(duration / 100000000 == 1) std::cout<<" 1 month";
            duration = duration % 100000000;
            if(duration / 1000000 > 1) std::cout<<" "<<duration / 1000000<<" days";
            else if(duration / 1000000 == 1) std::cout<<" 1 day";
            duration = duration % 1000000;
            if(duration / 10000 > 1) std::cout<<" "<<duration / 10000<<" hours";
            else if(duration / 10000 == 1) std::cout<<" 1 hour";
            duration = duration % 10000;
            if(duration / 100 > 1) std::cout<<" "<<duration / 100<<" minutes";
            else if(duration / 100 == 1) std::cout<<" 1 minute";
            duration = duration % 100;
            if(duration > 1) std::cout<<" "<<duration<<" seconds";
            else if(duration == 1) std::cout<<" 1 second";
            std::cout<<".\n";
        }
        else if(word[0] == 'h'){
            std::string usrName;
            iss >> usrName;
            std::cout<<"Customer "<<usrName<<" account summary:\n";
            std::cout<<"Balance: $"<<clients[usrName].balance<<"\n";
            std::cout<<"Total # of transactions: "<<clients[usrName].incoming.size() + clients[usrName].outgoing.size()<<"\n";
            std::cout<<"Incoming "<<clients[usrName].incoming.size()<<":\n";
            for(auto i : clients[usrName].incoming){
                trans& curr = transactions[i];
                if(curr.amount == 1) {std::cout<<curr.id<<": "<<curr.sender<<" sent 1 dollar to "<<curr.receiver<<" at "<<ts2Int(curr.execute)<<".\n";}
                else {std::cout<<curr.id<<": "<<curr.sender<<" sent "<<curr.amount<<" dollars to "<<curr.receiver<<" at "<<ts2Int(curr.execute)<<".\n";}
            }
            std::cout<<"Outgoing "<<clients[usrName].outgoing.size()<<":\n";
            for(auto i : clients[usrName].outgoing){
                trans& curr = transactions[i];
                if(curr.amount == 1) {std::cout<<curr.id<<": "<<curr.sender<<" sent 1 dollar to "<<curr.receiver<<" at "<<ts2Int(curr.execute)<<".\n";}
                else {std::cout<<curr.id<<": "<<curr.sender<<" sent "<<curr.amount<<" dollars to "<<curr.receiver<<" at "<<ts2Int(curr.execute)<<".\n";}
            }
        }
        else if(word[0] == 's'){
            timeStamp start;
            timeStamp end;
            iss >> word;
            str2TS(word, start);
            start.hh = 0;
            start.mi = 0;
            start.ss = 0;
            end = start;
            end.dd += 1;
            std::cout<<"Summary of ["<<ts2Int(start)<<", "<<ts2Int(end)<<"):\n";
            std::vector<trans>::iterator st = std::lower_bound(transactions.begin(), transactions.end(), start, CompareByTS());
            std::vector<trans>::iterator ed = std::lower_bound(transactions.begin(), transactions.end(), end, CompareByTS());

            uint32_t total = 0;
            for(std::vector<trans>::iterator i = st; i != ed; ++i){
                if(i->amount == 1) {std::cout<<i->id<<": "<<i->sender<<" sent 1 dollar to "<<i->receiver<<" at "<<ts2Int(i->execute)<<".\n";}
                else {std::cout<<i->id<<": "<<i->sender<<" sent "<<i->amount<<" dollars to "<<i->receiver<<" at "<<ts2Int(i->execute)<<".\n";}
                total += (i->senderFee + i->receiverFee);
            }
            if((ed - st) == 1) std::cout<<"There was a total of 1 transaction, 281Bank has collected "<<total<<" dollars in fees.\n";
            else std::cout<<"There were a total of "<<ed-st<<" transactions, 281Bank has collected "<<total<<" dollars in fees.\n";
        }
    }
}
