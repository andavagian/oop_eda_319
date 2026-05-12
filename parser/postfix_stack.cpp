#include <iostream>
#include <stack>
#include <sstream>
#include <vector>
#include <cctype>
using namespace std;

int prec(char op){
    if(op=='+'||op=='-') return 1;
    if(op=='*'||op=='/') return 2;
    return 0;
}

vector<string> infixToPostfix(string s){
    stack<char> ops;
    vector<string> out;

    for(int i=0;i<s.size();){
        if(isdigit(s[i])){
            string num;
            while(i<s.size() && isdigit(s[i])) num+=s[i++];
            out.push_back(num);
        }
        else if(s[i]=='('){
            ops.push(s[i++]);
        }
        else if(s[i]==')'){
            while(ops.top()!='('){
                out.push_back(string(1,ops.top()));
                ops.pop();
            }
            ops.pop(); i++;
        }
        else{
            while(!ops.empty() && prec(ops.top())>=prec(s[i])){
                out.push_back(string(1,ops.top()));
                ops.pop();
            }
            ops.push(s[i++]);
        }
    }

    while(!ops.empty()){
        out.push_back(string(1,ops.top()));
        ops.pop();
    }

    return out;
}

double evalPostfix(vector<string> p){
    stack<double> st;

    for(auto &t : p){
        if(isdigit(t[0])){
            st.push(stod(t));
        }else{
            double b=st.top(); st.pop();
            double a=st.top(); st.pop();

            if(t=="+") st.push(a+b);
            if(t=="-") st.push(a-b);
            if(t=="*") st.push(a*b);
            if(t=="/") st.push(a/b);
        }
    }
    return st.top();
}

int main(){
    string s="(5+7)/2";
    auto p=infixToPostfix(s);
    cout<<evalPostfix(p);
}