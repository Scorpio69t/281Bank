// Project Identifier: 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <iostream>
#include <sstream>

struct timeStamp{
    u_int64_t yy;
    u_int64_t mo;
    u_int64_t dd;
    u_int64_t hh;
    u_int64_t mi;
    u_int64_t ss;
};

struct info{
    int pin;
    uint32_t balance;
    timeStamp tS;
    std::vector<size_t> incoming;
    std::vector<size_t> outgoing;
};

struct trans{
    int id;
    std::string sender;
    std::string receiver;
    timeStamp place;
    uint32_t amount;
    uint32_t senderFee = 0;
    uint32_t receiverFee = 0;
    timeStamp execute;
    std::string type ="\0";
};

uint64_t ts2Int(const timeStamp& ts){
    return ts.ss+100*ts.mi+10000*ts.hh+1000000*ts.dd+100000000*ts.mo+10000000000*ts.yy;
    // std::cout<<ts.yy;
    // if(ts.mo / 10 == 0) std::cout<<"0"<<ts.mo;
    // else std::cout<<ts.mo;
    // if(ts.dd / 10 == 0) std::cout<<"0"<<ts.dd;
    // else std::cout<<ts.dd;
    // if(ts.hh / 10 == 0) std::cout<<"0"<<ts.hh;
    // else std::cout<<ts.hh;
    // if(ts.mi / 10 == 0) std::cout<<"0"<<ts.mi;
    // else std::cout<<ts.mi;
    // if(ts.ss / 10 == 0) std::cout<<"0"<<ts.ss;
    // else std::cout<<ts.ss;
}

bool sameTS(const timeStamp& ts1, const timeStamp& ts2){
    // return (ts1.yy == ts2.yy && ts1.mo == ts2.mo && ts1.dd == ts2.dd && ts1.hh == ts2.hh && ts1.mi == ts2.mi && ts1.ss == ts2.ss);
    return (ts2Int(ts1) == ts2Int(ts2));
}

bool cmpTS(const timeStamp& ts1, const timeStamp& ts2){ //return true if ts1 is earlier than ts2
    // if(ts1.yy == ts2.yy){
    //     if(ts1.mo == ts2.mo){
    //             if(ts1.dd == ts2.dd){
    //                 if(ts1.hh == ts2.hh){
    //                     if(ts1.mi == ts2.mi){
    //                         return (ts1.ss <= ts2.ss);
    //                     }else return (ts1.mi < ts2.mi);
    //                 }else return (ts1.hh < ts2.hh);
    //             }else return (ts1.dd < ts2.dd);
    //     }else return (ts1.mo < ts2.mo);
    // }else return (ts1.yy < ts2.yy);
    return (ts2Int(ts1) <= ts2Int(ts2));
}

struct compTrans{
    bool operator()(const trans& t1, const trans& t2){
        if (&t1 == &t2) {
            return false;
        }
        if(sameTS(t1.execute, t2.execute)){
            return (!(t1.id < t2.id));
        }else return (!cmpTS(t1.execute, t2.execute));
    }
};

struct CompareByTS {
    bool operator()(const trans& lhs, const trans& rhs) const {
        return cmpTS(lhs.execute, rhs.execute);
    }

    bool operator()(const trans& lhs, const timeStamp& val) const {
        return cmpTS(lhs.execute ,val);
    }

    bool operator()(const timeStamp& val, const trans& rhs) const {
        return cmpTS(val, rhs.execute);
    }
};

bool checkLoyalty(const timeStamp& ts1, const timeStamp& ts2){ //return true is ts2 > ts1+5years
    // if(ts2.yy == (ts1.yy+5)){
    //     if(ts1.mo == ts2.mo){
    //             if(ts1.dd == ts2.dd){
    //                 if(ts1.hh == ts2.hh){
    //                     if(ts1.mi == ts2.mi){
    //                         return (ts1.ss < ts2.ss);
    //                     }else return (ts1.mi < ts2.mi);
    //                 }else return (ts1.hh < ts2.hh);
    //             }else return (ts1.dd < ts2.dd);
    //     }else return (ts1.mo < ts2.mo);
    // }else return (ts2.yy > (ts1.yy+5));
    return (ts2Int(ts2) > (ts2Int(ts1) + 50000000000));
}

bool checkThreeDays(const timeStamp& ts1, const timeStamp& ts2){ //retrun true if ts2 is at most 3 days later than ts1
    // if(ts2.yy > ts1.yy) return false;
    // if(ts2.mo > ts1.mo) return false;
    // if(ts2.dd == (ts1.dd + 3)){
    //     if(ts2.hh == ts1.hh){
    //         if(ts2.mi == ts1.mi){
    //             return (ts2.ss < ts1.ss);
    //         }else return (ts2.mi < ts1.mi);
    //     }else return (ts2.hh < ts1.hh);
    // }else return (ts2.dd < (ts1.dd + 3));
    return (ts2Int(ts2) <= (ts2Int(ts1) + 3000000));
}

void str2TS(std::string str, timeStamp& ts){
    std::istringstream iss(str);
    std::string part;
    int idx = 0;
    while(std::getline(iss, part, ':')){
        switch (idx)
        {
        case 0:
            ts.yy = std::stoi(part);
            break;

        case 1:
            ts.mo = std::stoi(part);
            break;

        case 2:
            ts.dd = std::stoi(part);
            break; 

        case 3:
            ts.hh = std::stoi(part);
            break;

        case 4:
            ts.mi = std::stoi(part);
            break;

        case 5:
            ts.ss = std::stoi(part);
            break;
        
        default:
            break;
        }
        ++idx;
    }
}

class Bank{
    private:
        std::unordered_map<std::string,info> clients;
        std::unordered_map<std::string, std::unordered_set<std::string>> clientIPs;
        std::vector<trans> transactions;
        bool verb = false;
        std::string regFileName = "\0";
        std::priority_queue<trans, std::vector<trans>, compTrans> queuedTrans;
        int trTot = 0;
        void transaction();
    public:
        void get_options(int argc, char** argv);
        void read_clients();
        void run();
        void query();
};

void Bank::transaction(){
    if(!queuedTrans.empty()){
        trans currTrans = queuedTrans.top();
        uint32_t fee = currTrans.amount/100;
        fee = fee > 10 ? fee : 10;
        fee = fee > 450 ? 450 : fee;
        if(checkLoyalty(clients[currTrans.sender].tS, currTrans.execute)){
            fee = (fee * 3) / 4;
        }
        if(currTrans.type == "o"){
            if(clients[currTrans.sender].balance < (currTrans.amount + fee)){
                if(verb) std::cout<<"Insufficient funds to process transaction "<<currTrans.id<<".\n";
            }else{
                currTrans.senderFee = fee;
                // std::cout<<"Sender fee "<<currTrans.senderFee<<" Receiver fee "<<currTrans.receiverFee<<"\n";
                clients[currTrans.sender].balance -= (currTrans.amount + fee);
                clients[currTrans.sender].outgoing.push_back(transactions.size());

                clients[currTrans.receiver].balance += currTrans.amount;
                clients[currTrans.receiver].incoming.push_back(transactions.size());

                transactions.push_back(currTrans);
                if(verb){
                    std::cout<<"Transaction executed at "<<ts2Int(currTrans.execute)<<": $"<<currTrans.amount<<" from "<<currTrans.sender<<" to "<<currTrans.receiver<<".\n";
                }
            }
        }
        else{
            currTrans.senderFee = (fee % 2 == 0) ? (fee / 2) : ((fee + 1) / 2);
            currTrans.receiverFee = (fee % 2 == 0) ? (fee / 2) : ((fee - 1) / 2);
            // std::cout<<"Sender fee "<<currTrans.senderFee<<" Receiver fee "<<currTrans.receiverFee<<"\n";
            if(clients[currTrans.sender].balance < (currTrans.senderFee + currTrans.amount) || clients[currTrans.receiver].balance < currTrans.receiverFee){
                if(verb) std::cout<<"Insufficient funds to process transaction "<<currTrans.id<<".\n";
            }else{
                clients[currTrans.sender].balance -= (currTrans.senderFee + currTrans.amount);
                clients[currTrans.sender].outgoing.push_back(transactions.size());

                clients[currTrans.receiver].balance -= currTrans.receiverFee;
                clients[currTrans.receiver].balance += currTrans.amount;
                clients[currTrans.receiver].incoming.push_back(transactions.size());
                
                transactions.push_back(currTrans);
                if(verb){
                    std::cout<<"Transaction executed at "<<ts2Int(currTrans.execute)<<": $"<<currTrans.amount<<" from "<<currTrans.sender<<" to "<<currTrans.receiver<<".\n";
                }
            }
        }
        queuedTrans.pop();
    }
}