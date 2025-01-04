#include<iostream>
#include<fstream>
#include<map>
#include<vector>
#include<sstream>
#include<queue>
#include<unordered_set>
#include<algorithm>
#include<set>

using namespace std;

struct LR1Item{
    string left;
    vector<string> right;
    int dotPosition;
    string lookAhead;

    bool operator==(const LR1Item &other) const{
        return left == other.left && right == other.right && dotPosition == other.dotPosition && lookAhead == other.lookAhead;
    }

    bool operator<(const LR1Item& other) const {
        if (left != other.left) return left < other.left;
        if (right != other.right) return right < other.right;
        if (dotPosition != other.dotPosition) return dotPosition < other.dotPosition;
        return lookAhead < other.lookAhead;
    }
};

class LR1{
private: 
    string filename;
    map<string, vector<vector<string>>> grammer;
    string head; 
    vector<string> headProd;
    //状态记录
    vector<vector<LR1Item>> states;
    //状态转换记录
    map<pair<int, string>, int> transitions;

public:
    //读取文法到map
    LR1(string Filename):filename(Filename) {}
    LR1(string Filename, string Head):filename(Filename), head(Head) {}
    //LR1(string Head):head(Head) {}

    void readGrammar(){
        ifstream input(filename);
        if (!input.is_open())
        {
            cerr << "未正确打开" << filename;
            return;
        }
        string line;
        while(getline(input, line)){
            istringstream iss(line);
            string vn;
            string arrow;
            iss >> vn >> arrow;

            string symbol;
            vector<string> prod;
            while(iss >> symbol){
                prod.push_back(symbol);
            }
            grammer[vn].push_back(prod);
        }

        input.close(); 
    }

    //拓广文法
    void extendGrammer(){
        auto it = grammer.begin();
        string headSymbol = it->first;
        if(!head.empty()){
            headSymbol = head;
        }

        string extend = headSymbol + "'";
        grammer[extend] = {{headSymbol}};

        head = extend;
        headProd = {headSymbol};
    }
    void DFA(){
        LR1Item startItem = {head, headProd, 0, "$"};
        vector<LR1Item> startState = computeClosure({startItem});
        states.push_back(startState);
        queue<int> stateQueue;
        stateQueue.push(0);

        while (!stateQueue.empty())
        {
            int stateIndex = stateQueue.front();
            stateQueue.pop();

            set<string> symbols;
            for(const LR1Item item : states[stateIndex]){
                if(item.dotPosition < item.right.size()){
                    symbols.insert(item.right[item.dotPosition]);
                }
            }
            for(const string &symbol : symbols){
                vector<LR1Item> newState = goToNext(symbol, states[stateIndex]);
                if(newState.empty()){
                    continue;
                }

                int newStateIndex;
                auto it = find(states.begin(), states.end(), newState);
                if(it == states.end()){
                    states.push_back(newState);
                    newStateIndex = states.size() - 1;
                    stateQueue.push(newStateIndex);
                }else{
                    newStateIndex = it - states.begin();
                }
                transitions[{stateIndex, symbol}] = newStateIndex;
            }
        }
    }

    void printDFA(){
        for(int i = 0; i < states.size(); i++){
            cout << "I" << i << ":\n";
            for(const LR1Item &item : states[i]){
                string prodLeft = item.left + " -> ";
                ostringstream oss;
                for (const auto& str : item.right) {
                    oss << str;
                }
                string prodRight = oss.str();
                prodRight.insert(prodRight.begin() + item.dotPosition, '.');
                cout << prodLeft << prodRight << " ,"<< item.lookAhead << "\n";
            }
            cout << "\n";
        }
    }
    vector<LR1Item> computeClosure(vector<LR1Item> items){
        vector<LR1Item> clousureItems = items;
        queue<LR1Item> workQueue;
        for(const auto &item : items){
            workQueue.push(item);
        }

        while (!workQueue.empty())
        {
            LR1Item itemTemp = workQueue.front();
            workQueue.pop();

            //归约不计算闭包
            if(itemTemp.dotPosition < itemTemp.right.size()){
                string symbol = itemTemp.right[itemTemp.dotPosition];

                //判断当前位置的标识符是否为非终结符
                if(grammer.count(symbol)){
                    vector<string> beta = {itemTemp.right.begin() + itemTemp.dotPosition + 1, itemTemp.right.end()};
                    beta.push_back(itemTemp.lookAhead);
                    auto firstSet = computeFirst(beta);

                    for(const vector<string> &vn : grammer[symbol]){
                        for(const string &first : firstSet){
                            LR1Item vnTemp = {symbol, vn, 0, first};
                            if(find(clousureItems.begin(), clousureItems.end(), vnTemp) == clousureItems.end()){
                                clousureItems.push_back(vnTemp);
                                workQueue.push(vnTemp);
                            }

                        }
                    }
                }
            }
        }
        return clousureItems;
    }
    unordered_set<string> computeFirst(const vector<string> &symbols){
        unordered_set<string> firstSet;
        for(auto &symbol : symbols){
            if(grammer.find(symbol) == grammer.end()){
                //终结符
                firstSet.insert(symbol);
                break;
            }else{
                //非终结符
                bool hasEpsilon = false;
                for(const auto &prod : grammer[symbol]){
                    auto first = computeFirst(prod);
                    firstSet.insert(first.begin(), first.end());
                    if(first.count("ε")){
                        hasEpsilon = true;
                        firstSet.erase("ε");
                    }
                }

                if(!hasEpsilon){
                    break;
                }
            }
        }
        return firstSet;
    }
    vector<LR1Item> goToNext(string symbol, vector<LR1Item> oldState){
        vector<LR1Item> newState;
        for(const LR1Item &item : oldState){
            if(item.dotPosition < item.right.size() && item.right[item.dotPosition] == symbol){
                LR1Item newItem = item;
                newItem.dotPosition++;
                newState.push_back(newItem);
            }
        }
        return computeClosure(newState);

    }

    //判断lr1文法
    void judgeLR1(){
        for(int i = 0; i < states.size(); i++){
            bool hasShaft = false;
            bool hasReduction = false;
            vector<int> shaftIndex;
            vector<int> reductionIndex;
            for(int j = 0; j < states[i].size(); j++){
                LR1Item item = states[i][j];
                if(item.dotPosition == item.right.size()){
                    hasReduction = true;
                    reductionIndex.push_back(j);
                }else if(item.dotPosition < item.right.size()){
                    hasShaft = true;
                    shaftIndex.push_back(j);
                }
            }
            //当前状态同时存在移进-归约
            if(hasShaft && hasReduction){
                set<string> shaftLookAhead;
                set<string> reductionLookAhead;
                for(int index : shaftIndex){
                    shaftLookAhead.insert(states[i][index].lookAhead);
                }
                for(int index : reductionIndex){
                    reductionLookAhead.insert(states[i][index].lookAhead);
                }
                vector<string> interSection;
                set_intersection(shaftLookAhead.begin(), shaftLookAhead.end(), reductionLookAhead.begin(), reductionLookAhead.end(), back_inserter(interSection));
                if(interSection.size() != 0){
                    cout << "状态I" << i << "存在移进-归约冲突\n";
                    cout << "冲突向前搜索符为" << interSection[0];
                    for(int index = 1; index < interSection.size(); index++){
                        cout << ", " << interSection[index];
                    }
                    cout << "\n";
                }
            }
            if(hasReduction){
                map<LR1Item, set<string>> reductionItem;
                for(int index = 0; index < reductionIndex.size(); index++){
                    LR1Item item = states[i][index];
                    item.lookAhead = "";
                    item.dotPosition = 0;
                    reductionItem[item].insert(states[i][index].lookAhead);
                }
                for(const auto &pairi : reductionItem){
                    for(const auto &pairj : reductionItem){
                        if(pairi == pairj){
                            continue;
                        }
                        vector<string> interSection;
                        set_intersection(pairi.second.begin(), pairi.second.end(), pairj.second.begin(), pairj.second.end(), back_inserter(interSection));
                        if(interSection.size() != 0){
                            cout << "状态I" << i << " 存在归约-归约冲突项目\n";

                            cout << pairi.first.left << " -> ";
                            ostringstream ossi;
                            for (const auto& str : pairi.first.right) {
                                ossi << str;
                            }
                            string prodRight = ossi.str();
                            cout << prodRight << "\n";

                            cout << pairj.first.left << " -> ";
                            ostringstream ossj;
                            for (const auto& str : pairj.first.right) {
                                ossj << str;
                            }
                            prodRight = ossj.str();
                            cout << prodRight << "\n";

                            cout << "冲突向前搜索符为" << interSection[0];
                            for(int index = 1; index < interSection.size(); index++){
                                cout << ", " << interSection[index];
                            }
                            cout << "\n";
                        }
                    }
                }
            }
        }
    }
};