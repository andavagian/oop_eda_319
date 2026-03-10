#include <iostream>
#include <string>
using namespace std;

class Parser{
    string s;
    int pos=0;

public:
    Parser(string str):s(str){}

    double parse(){
        return expression();
    }

private:
    double expression(){
        double v=term();
        while(pos<s.size() && (s[pos]=='+'||s[pos]=='-')){
            char op=s[pos++];
            double t=term();
            if(op=='+') v+=t;
            else v-=t;
        }
        return v;
    }

    double term(){
        double v=factor();
        while(pos<s.size() && (s[pos]=='*'||s[pos]=='/')){
            char op=s[pos++];
            double t=factor();
            if(op=='*') v*=t;
            else v/=t;
        }
        return v;
    }

    double factor(){
        if(s[pos]=='('){
            pos++;
            double v=expression();
            pos++;
            return v;
        }

        double num=0;
        while(pos<s.size() && isdigit(s[pos])){
            num=num*10+(s[pos]-'0');
            pos++;
        }
        return num;
    }
};

int main(){
    Parser p("(5+7)/2");
    cout<<p.parse();
}