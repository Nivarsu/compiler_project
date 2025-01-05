#include "LR1.h"

int main(){
    string path = "grammer.txt";
    LR1 lr1(path);
    lr1.readGrammar();
    lr1.extendGrammer();
    lr1.DFA();
    lr1.printDFA();
    lr1.judgeLR1();
    lr1.parseTable();
    lr1.printParseTable();
    lr1.inputContent("input.txt");
    lr1.parseInput();

    return 0;
}