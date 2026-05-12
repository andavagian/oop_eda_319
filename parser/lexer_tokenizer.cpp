#include <iostream>
#include <vector>
#include <stack>
#include <cctype>
using namespace std;

enum Type {NUM, OP, LP, RP};

struct Token{
    Type type;
    double value;
    char op;
};

vector<Token> tokenize(string s){
    vector<Token> t;

    for(int i=0;i<s.size();){
        if(isdigit(s[i])){
            double num=0;
            while(i<s.size() && isdigit(s[i])){
                num=num*10+(s[i]-'0');
                i++;
            }
            t.push_back({NUM,num,0});
        }
        else if(s[i]=='('){ t.push_back({LP,0,0}); i++; }
        else if(s[i]==')'){ t.push_back({RP,0,0}); i++; }
        else{
            t.push_back({OP,0,s[i]});
            i++;
        }
    }
    return t;
}

double apply(double a,double b,char op){
    if(op=='+') return a+b;
    if(op=='-') return a-b;
    if(op=='*') return a*b;
    return a/b;
}

double evaluate(vector<Token> t){
    stack<double> vals;
    stack<char> ops;

    auto prec=[](char c){ return (c=='+'||c=='-')?1:2; };

    for(auto &x:t){
        if(x.type==NUM) vals.push(x.value);

        else if(x.type==LP) ops.push('(');

        else if(x.type==RP){
            while(ops.top()!='('){
                double b=vals.top(); vals.pop();
                double a=vals.top(); vals.pop();
                vals.push(apply(a,b,ops.top()));
                ops.pop();
            }
            ops.pop();
        }

        else{
            while(!ops.empty() && ops.top()!='(' && prec(ops.top())>=prec(x.op)){
                double b=vals.top(); vals.pop();
                double a=vals.top(); vals.pop();
                vals.push(apply(a,b,ops.top()));
                ops.pop();
            }
            ops.push(x.op);
        }
    }

    while(!ops.empty()){
        double b=vals.top(); vals.pop();
        double a=vals.top(); vals.pop();
        vals.push(apply(a,b,ops.top()));
        ops.pop();
    }

    return vals.top();
}

int main(){
    string s="(5+7)/2";
    auto tokens=tokenize(s);
    cout<<evaluate(tokens);
}