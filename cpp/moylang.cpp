#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <stdexcept>

struct Value {
    bool isString;
    long long i;
    std::string s;
    Value(long long v = 0) : isString(false), i(v) {}
    Value(const std::string& str) : isString(true), i(0), s(str) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& src) : src(src), pos(0) {}
    enum Type { ID, NUM, STR, OP, LPAREN, RPAREN, LBRACE, RBRACE, SEMI, END };
    struct Token { Type type; std::string text; };
    Token next() {
        skipWs();
        if (pos >= src.size()) return {END, ""};
        char c = src[pos];
        if (std::isalpha(c) || c=='_') return ident();
        if (std::isdigit(c)) return number();
        if (c=='\"') return string();
        switch (c) {
            case '(': ++pos; return {LPAREN, "("};
            case ')': ++pos; return {RPAREN, ")"};
            case '{': ++pos; return {LBRACE, "{"};
            case '}': ++pos; return {RBRACE, "}"};
            case ';': ++pos; return {SEMI, ";"};
            case '#':
                while (pos < src.size() && src[pos] != '\n') ++pos;
                return next();
        }
        ++pos;
        return {OP, std::string(1,c)};
    }
private:
    Token ident() {
        size_t start = pos;
        while (pos < src.size() && (std::isalnum(src[pos]) || src[pos]=='_')) ++pos;
        return {ID, src.substr(start, pos-start)};
    }
    Token number() {
        size_t start = pos;
        while (pos < src.size() && std::isdigit(src[pos])) ++pos;
        return {NUM, src.substr(start, pos-start)};
    }
    Token string() {
        ++pos; size_t start = pos;
        while (pos < src.size() && src[pos] != '\"') ++pos;
        std::string res = src.substr(start, pos-start);
        if (pos < src.size()) ++pos; // consume quote
        return {STR, std::string("\"") + res + "\""};
    }
    void skipWs() {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) ++pos;
    }
    const std::string& src; size_t pos;
};

class Parser {
public:
    explicit Parser(const std::string& src) : lex(src) { curr = lex.next(); }
    struct Stmt { enum {VAR, PRINT, ASSIGN, IF, WHILE, BLOCK} kind; std::string name; std::vector<Stmt> block; std::vector<Stmt> elseBlock; std::vector<std::string> expr; };
    std::vector<Stmt> parse() {
        std::vector<Stmt> res;
        while (curr.type != Lexer::END) res.push_back(statement());
        return res;
    }
private:
    Lexer lex; Lexer::Token curr;
    void advance() { curr = lex.next(); }
    bool match(Lexer::Type t) { if (curr.type==t) { advance(); return true; } return false; }
    void expect(Lexer::Type t) { if (!match(t)) throw std::runtime_error("syntax error"); }
    std::string expression() {
        std::string out;
        int depth=0;
        while (curr.type!=Lexer::SEMI && !(curr.type==Lexer::RPAREN && depth==0) && curr.type!=Lexer::END) {
            if (curr.type==Lexer::LPAREN) depth++;
            if (curr.type==Lexer::RPAREN) depth--;
            out += curr.text + ' ';
            advance();
        }
        return out;
    }
    Stmt block() {
        Stmt s; s.kind = Stmt::BLOCK;
        expect(Lexer::LBRACE);
        while (!match(Lexer::RBRACE)) s.block.push_back(statement());
        return s;
    }
    Stmt statement() {
        if (curr.type==Lexer::ID && curr.text=="var") {
            advance();
            Stmt s; s.kind=Stmt::VAR; s.name=curr.text; expect(Lexer::ID); expect(Lexer::OP); s.expr.push_back(expression()); expect(Lexer::SEMI); return s;
        }
        if (curr.type==Lexer::ID && curr.text=="print") {
            advance(); expect(Lexer::LPAREN); Stmt s; s.kind=Stmt::PRINT; s.expr.push_back(expression()); expect(Lexer::RPAREN); expect(Lexer::SEMI); return s;
        }
        if (curr.type==Lexer::ID && curr.text=="if") {
            advance(); expect(Lexer::LPAREN); Stmt s; s.kind=Stmt::IF; s.expr.push_back(expression()); expect(Lexer::RPAREN); s.block.push_back(block());
            if (curr.type==Lexer::ID && curr.text=="else") { advance(); s.elseBlock.push_back(block()); }
            return s;
        }
        if (curr.type==Lexer::ID && curr.text=="while") {
            advance(); expect(Lexer::LPAREN); Stmt s; s.kind=Stmt::WHILE; s.expr.push_back(expression()); expect(Lexer::RPAREN); s.block.push_back(block()); return s;
        }
        Stmt s; s.kind=Stmt::ASSIGN; s.name=curr.text; expect(Lexer::ID); expect(Lexer::OP); s.expr.push_back(expression()); expect(Lexer::SEMI); return s;
    }
};

class Interpreter {
public:
    void exec(const std::vector<Parser::Stmt>& stmts) { for (const auto& s : stmts) run(s); }
private:
    std::unordered_map<std::string, Value> vars;
    static void skipSpaces(const std::string& e, size_t& i){ while(i<e.size() && std::isspace(static_cast<unsigned char>(e[i]))) ++i; }
    Value parseValue(const std::string& e, size_t& i){
        skipSpaces(e,i);
        if(i>=e.size()) throw std::runtime_error("unexpected end");
        if(e[i]=='\"'){
            ++i; size_t start=i; while(i<e.size()&&e[i]!='\"') ++i; std::string res=e.substr(start,i-start); if(i<e.size()) ++i; return Value(res);
        }
        if(std::isdigit(e[i])){
            size_t start=i; while(i<e.size()&&std::isdigit(e[i])) ++i; return Value(std::stoll(e.substr(start,i-start)));
        }
        if(std::isalpha(e[i])||e[i]=='_'){
            size_t start=i; while(i<e.size()&&(std::isalnum(e[i])||e[i]=='_')) ++i; std::string name=e.substr(start,i-start); if(!vars.count(name)) throw std::runtime_error("undefined var"); return vars[name];
        }
        if(e[i]=='('){ ++i; Value v=parseExpr(e,i); skipSpaces(e,i); if(i<e.size()&&e[i]==')') ++i; return v; }
        throw std::runtime_error("bad value");
    }
    Value parseExpr(const std::string& e, size_t& i){
        Value left=parseValue(e,i); skipSpaces(e,i);
        while(i<e.size() && (e[i]=='+'||e[i]=='-')){
            char op=e[i++]; Value right=parseValue(e,i); skipSpaces(e,i);
            if(op=='+') left=add(left,right); else left=sub(left,right);
        }
        return left;
    }
    Value evalExpr(const std::string& expr){ size_t i=0; return parseExpr(expr,i); }
    static Value add(const Value&a,const Value&b){ if(a.isString||b.isString) return Value(toString(a)+toString(b)); return Value(a.i+b.i); }
    static Value sub(const Value&a,const Value&b){ return Value(a.i-b.i); }
    static std::string toString(const Value&v){ return v.isString? v.s : std::to_string(v.i); }
    bool evalCond(const std::string& expr){ Value v=evalExpr(expr); return v.isString? !v.s.empty() : v.i!=0; }
    void run(const Parser::Stmt& st){
        switch(st.kind){
            case Parser::Stmt::VAR:
                vars[st.name]=evalExpr(st.expr[0]);
                break;
            case Parser::Stmt::ASSIGN: if(vars.count(st.name)) vars[st.name]=evalExpr(st.expr[0]); else throw std::runtime_error("undefined var"); break;
            case Parser::Stmt::PRINT:
                std::cout << toString(evalExpr(st.expr[0])) << std::endl;
                break;
            case Parser::Stmt::IF: if(evalCond(st.expr[0])) for(const auto&x:st.block[0].block) run(x); else if(!st.elseBlock.empty()) for(const auto&x:st.elseBlock[0].block) run(x); break;
            case Parser::Stmt::WHILE: while(evalCond(st.expr[0])) for(const auto&x:st.block[0].block) run(x); break;
            case Parser::Stmt::BLOCK: for(const auto&x:st.block) run(x); break;
        }
    }
};

int main(int argc, char** argv){
    if(argc!=2){ std::cerr<<"Usage: ./moylang <file.moy>"<<std::endl; return 1; }
    std::ifstream f(argv[1]); if(!f){ std::cerr<<"Cannot open file"<<std::endl; return 1; }
    std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    Parser p(src); auto stmts=p.parse(); Interpreter in; in.exec(stmts); return 0;
}

