#include<iostream>
#include<fstream>
#include<map>
#include<vector>
#include<sstream>
#include<queue>
#include<unordered_set>
#include<algorithm>
#include<set>
#include <iomanip>

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
    unordered_set<string> Action;
    unordered_set<string> Goto;
    vector<map<string, string>> parse;
    string input;
    

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
        //LR1Item startItem = {head, headProd, 0, "$"};
        LR1Item startItem = {head, headProd, 0, "#"};
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
        bool judge = false;
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
                    shaftLookAhead.insert(states[i][index].right[states[i][index].dotPosition]);
                }
                for(int index : reductionIndex){
                    reductionLookAhead.insert(states[i][index].lookAhead);
                }
                vector<string> interSection;
                set_intersection(shaftLookAhead.begin(), shaftLookAhead.end(), reductionLookAhead.begin(), reductionLookAhead.end(), back_inserter(interSection));
                if(interSection.size() != 0){
                    judge = true;
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
                            judge = true;
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
        if(judge){
            cout << "该文法不是LR1文法";
            exit(1);
        }
    }
    void parseTable(){
        for(const auto &pair : grammer){
            if(pair.first != head){
                Goto.insert(pair.first);
            }
        }
        for(const auto &pair : grammer){
            if(pair.first == head){
                continue;
            }
            vector<vector<string>> prodTotal = pair.second;
            for(const vector<string> &prod : prodTotal){
                for(const string temp : prod){
                    if(!Goto.count(temp)){
                        Action.insert(temp);
                    }
                }
            }
        }
        Action.insert("#");
        parse.resize(states.size());
        for(const auto &transition : transitions){
            int stateIndex = transition.first.first;
            string symbol = transition.first.second;
            int nextState = transition.second;
            if(grammer.count(symbol)){
                parse[stateIndex][symbol] = to_string(nextState);
            }else{
                parse[stateIndex][symbol] = "s" + to_string(nextState);
            }
        }
        for(int i = 0; i < states.size(); i++){
            for(const LR1Item &item : states[i]){
                //归约项目
                if(item.left == head && item.right == headProd && item.dotPosition == 1){
                    parse[i][item.lookAhead] = "acc";
                    continue;
                }
                if(item.dotPosition == item.right.size()){
                    parse[i][item.lookAhead] = item.left + " -> ";
                    int prodIndex;
                    for(int index = 0; index < grammer[item.left].size(); index++){
                        vector<string> prod = grammer[item.left][index];
                        if(prod == item.right){
                            prodIndex = index;
                            break;
                        }
                    }
                    parse[i][item.lookAhead] += to_string(prodIndex);
                }
            }
        }
    }
    void printParseTable(){
        cout << "LR1分析表\n";
        cout << "     ";
        cout << setw(Action.size() * 10) << "Action";
        cout << " | ";
        cout << setw(Goto.size() * 10) << "Goto";
        cout << "\n";
        cout << "     ";
        for(const auto &item : Action){
            cout << setw(10) << item;
        }
        cout << " | ";
        for(const auto &item : Goto){
            cout << setw(10) << item;
        }
        cout << "\n";
        for(int i = 0; i < states.size(); i++){
            cout << "I" << i << "   "; 
            for(string ac : Action){
                // if(ac == "#"){
                //     ac = "$";
                // }
                if(parse[i].find(ac) != parse[i].end()){
                    cout << setw(10) << parse[i][ac];
                }else{
                    cout << setw(10) << " ";
                }
            }
            cout << " | ";
            for(string ac : Goto){
                if(parse[i].find(ac) != parse[i].end()){
                    cout << setw(10) << parse[i][ac];
                }else{
                    cout << setw(10) << " ";
                }
            }
            cout << "\n";
        }
    }
    void inputContent(string inputPath){
        ifstream in(inputPath);
        if(!in.is_open()){
            cerr << "文件未正常打开";
            return;
        }
        
        getline(in, input);
        cout << "输入句子为：" << input << "\n";

    }
    void parseInput(){
        string inputTemp = input;
        inputTemp.push_back('#');
        vector<string> parseState;
        vector<string> parseSymbol;
        parseState.push_back("0");
        parseSymbol.push_back("#");
        cout << std::left << setw(20) << "StateStake" 
        << std::left << setw(20) << "SymbolStake" 
        << setw(20) << "Input" 
        << std::left << setw(20) << "Action" 
        << std::left << setw(20) << "Goto";
        cout << "\n";
        string parseAction;
        string parseGoto;
        bool flag = false;
        while(parseAction != "acc"){
            cout << std::left << setw(20) << vectorToString(parseState);
            cout << std::left << setw(20) << vectorToString(parseSymbol);
            cout << setw(20) << inputTemp;

            //获取状态栈顶
            string stateTop = parseState.back();
            //获取输入串当前头部
            string inputPoint;
            inputPoint.push_back(inputTemp[0]);
            
            
            //if(parse[stateTop[0] - '0'].count(inputPoint)){
            if(parse[stringToInt(stateTop)].count(inputPoint)){
                //string nextAction = parse[stateTop[0] - '0'][inputPoint];
                string nextAction = parse[stringToInt(stateTop)][inputPoint];
                if(nextAction == "acc"){
                    parseAction = "acc";
                    cout << std::left << setw(20) << parseAction;
                    cout << "\n";
                    flag = true;
                    break;
                }else if(nextAction.size() > 2 && nextAction != "acc"){
                    //归约
                    istringstream iss(nextAction);
                    string left, right, arrow;
                    iss >> left >> arrow >> right;
                    //int length = grammer[left][right[0] - '0'].size();
                    int length = grammer[left][stringToInt(right)].size();
                    while(length > 0){
                        parseState.pop_back();
                        parseSymbol.pop_back();
                        length--;
                    }
                    parseSymbol.push_back(left);
                    parseAction = nextAction;
                    stateTop = parseState.back();
                    //string nextGoto = parse[stateTop[0] - '0'][left];
                    int num = stringToInt(stateTop);
                    string nextGoto = parse[num][left];
                    parseGoto = nextGoto;
                    parseState.push_back(nextGoto);

                }else{
                    //移进
                    parseSymbol.push_back(inputPoint);
                    inputTemp.erase(0, 1);

                    parseAction = nextAction;
                    parseGoto = " ";
                    nextAction.erase(0, 1);
                    parseState.push_back(nextAction);
                }
                cout << std::left << setw(20) << parseAction 
                    << std::left << setw(20) << parseGoto;
                cout << "\n";
            }else{
                cout << "\n分析表无法找到当前状态的Action\n";
                cout << "错误类型：遇到未期望的字符" << inputTemp[0];
                cout << ", 位置在" << input.size() - inputTemp.size() + 1 << "\n";
                stateTop = parseState.back();
                string expectedSymbol;
                //for(const auto &pair : parse[stateTop[0] - '0']){
                for(const auto &pair : parse[stringToInt(stateTop)]){
                    if(!grammer.count(pair.first)){
                        if(expectedSymbol.empty()){
                            expectedSymbol = pair.first;
                        }else{
                            expectedSymbol += "," + pair.first;
                        }
                    }
                }
                cout << "修改建议：期待的字符为" << expectedSymbol;
                break;
            }

        }
        if(flag){
            cout << "合法输入,该输入内容符合LR1文法";
        }

    }
    string vectorToString(vector<string> content){
        ostringstream oss;
        for(const string &str : content){
            oss << str;
        }
        return oss.str();
    }
    int stringToInt(string str){
        int num = 0;
        int index = 1;
        for(int i = 0; i < str.size(); i++){
            num += index * (str[i] - '0');
            index *= 10;
        }
        return num;
    }
};