#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cctype>
#include <algorithm>
#include <stdexcept>

using namespace std;

// БАЗОВЫЕ СТРУКТУРЫ ДЕРЕВА

struct Node {
    virtual ~Node() = default;
    virtual double evaluate(const map<string, double>& vars) const = 0;
    virtual shared_ptr<Node> derivative(const string& var) const = 0;
    virtual string toString() const = 0;
    virtual bool isBinary() const { return false; }
};

// ПРЕДВАРИТЕЛЬНЫЕ ОБЪЯВЛЕНИЯ

struct BinaryOpNode;
struct FunctionNode;
struct UnaryOpNode;

// РЕАЛИЗАЦИЯ УЗЛОВ

struct NumberNode : Node {
    double val;
    NumberNode(double v) : val(v) {}
    double evaluate(const map<string, double>&) const override { return val; }
    shared_ptr<Node> derivative(const string&) const override { return make_shared<NumberNode>(0.0); }
    string toString() const override {
        if (val == floor(val) && abs(val) < 1e15) {
            ostringstream oss; oss << static_cast<long long>(val); return oss.str();
        }
        ostringstream oss; oss << fixed << setprecision(6) << val;
        string s = oss.str();
        if (s.find('.') != string::npos) {
            while (s.back() == '0') s.pop_back();
            if (s.back() == '.') s.pop_back();
        }
        return s;
    }
};

struct VariableNode : Node {
    string name;
    VariableNode(const string& n) : name(n) {}
    double evaluate(const map<string, double>& vars) const override {
        auto it = vars.find(name);
        if (it == vars.end()) throw runtime_error("ERROR Unknown variable: " + name);
        return it->second;
    }
    shared_ptr<Node> derivative(const string& var) const override {
        return make_shared<NumberNode>(name == var ? 1.0 : 0.0);
    }
    string toString() const override { return name; }
};

struct BinaryOpNode : Node {
    char op;
    shared_ptr<Node> left, right;
    BinaryOpNode(char o, shared_ptr<Node> l, shared_ptr<Node> r) : op(o), left(l), right(r) {}
    double evaluate(const map<string, double>& vars) const override;
    shared_ptr<Node> derivative(const string& var) const override;
    string toString() const override;
    bool isBinary() const override { return true; }
};

struct UnaryOpNode : Node {
    char op;
    shared_ptr<Node> child;
    UnaryOpNode(char o, shared_ptr<Node> c) : op(o), child(c) {}
    double evaluate(const map<string, double>& vars) const override;
    shared_ptr<Node> derivative(const string& var) const override;
    string toString() const override;
};

struct FunctionNode : Node {
    string name;
    shared_ptr<Node> arg;
    FunctionNode(const string& n, shared_ptr<Node> a) : name(n), arg(a) {}
    double evaluate(const map<string, double>& vars) const override;
    shared_ptr<Node> derivative(const string& var) const override;
    string toString() const override;
};

// РЕАЛИЗАЦИЯ МЕТОДОВ

double UnaryOpNode::evaluate(const map<string, double>& vars) const {
    double v = child->evaluate(vars);
    return op == '+' ? v : -v;
}

shared_ptr<Node> UnaryOpNode::derivative(const string& var) const {
    return static_pointer_cast<Node>(make_shared<UnaryOpNode>(op, child->derivative(var)));
}

string UnaryOpNode::toString() const {
    string c = child->toString();
    if (child->isBinary()) c = "(" + c + ")";
    return string(1, op) + c;
}

double FunctionNode::evaluate(const map<string, double>& vars) const {
    double v = arg->evaluate(vars);
    if (name == "sin") return sin(v);
    if (name == "cos") return cos(v);
    if (name == "tan") return tan(v);
    if (name == "asin") { if (v < -1.0 || v > 1.0) throw runtime_error("ERROR Domain error: asin(" + to_string(v) + ")"); return asin(v); }
    if (name == "acos") { if (v < -1.0 || v > 1.0) throw runtime_error("ERROR Domain error: acos(" + to_string(v) + ")"); return acos(v); }
    if (name == "atan") return atan(v);
    if (name == "exp") return exp(v);
    if (name == "log") { 
        if (v < 0.0) throw runtime_error("ERROR Domain error: log(" + to_string(v) + ")");
        return log(v);
    }
    if (name == "sqrt") { if (v < 0.0) throw runtime_error("ERROR Domain error: sqrt(" + to_string(v) + ")"); return sqrt(v); }
    throw runtime_error("ERROR Unknown function: " + name);
}

shared_ptr<Node> FunctionNode::derivative(const string& var) const {
    shared_ptr<Node> dArg = arg->derivative(var);
    if (name == "sin") 
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', static_pointer_cast<Node>(make_shared<FunctionNode>("cos", arg)), dArg));
    if (name == "cos") 
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', static_pointer_cast<Node>(make_shared<UnaryOpNode>('-', static_pointer_cast<Node>(make_shared<FunctionNode>("sin", arg)))), dArg));
    if (name == "tan") {
        auto cos_u = static_pointer_cast<Node>(make_shared<FunctionNode>("cos", arg));
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('/', dArg, static_pointer_cast<Node>(make_shared<BinaryOpNode>('^', cos_u, make_shared<NumberNode>(2)))));
    }
    if (name == "asin") {
        auto inner = static_pointer_cast<Node>(make_shared<BinaryOpNode>('-', make_shared<NumberNode>(1), static_pointer_cast<Node>(make_shared<BinaryOpNode>('^', arg, make_shared<NumberNode>(2)))));
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('/', dArg, static_pointer_cast<Node>(make_shared<FunctionNode>("sqrt", inner))));
    }
    if (name == "acos") {
        auto inner = static_pointer_cast<Node>(make_shared<BinaryOpNode>('-', make_shared<NumberNode>(1), static_pointer_cast<Node>(make_shared<BinaryOpNode>('^', arg, make_shared<NumberNode>(2)))));
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('/', static_pointer_cast<Node>(make_shared<UnaryOpNode>('-', dArg)), static_pointer_cast<Node>(make_shared<FunctionNode>("sqrt", inner))));
    }
    if (name == "atan") {
        auto inner = static_pointer_cast<Node>(make_shared<BinaryOpNode>('+', make_shared<NumberNode>(1), static_pointer_cast<Node>(make_shared<BinaryOpNode>('^', arg, make_shared<NumberNode>(2)))));
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('/', dArg, inner));
    }
    if (name == "exp") 
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', static_pointer_cast<Node>(make_shared<FunctionNode>("exp", arg)), dArg));
    if (name == "log") 
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('/', dArg, arg));
    if (name == "sqrt") 
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('/', dArg, static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', make_shared<NumberNode>(2), static_pointer_cast<Node>(make_shared<FunctionNode>("sqrt", arg))))));
    return make_shared<NumberNode>(0);
}

string FunctionNode::toString() const { return name + "(" + arg->toString() + ")"; }

double BinaryOpNode::evaluate(const map<string, double>& vars) const {
    double l = left->evaluate(vars), r = right->evaluate(vars);
    if (op == '+') return l + r;
    if (op == '-') return l - r;
    if (op == '*') return l * r;
    if (op == '/') return l / r;
    if (op == '^') return pow(l, r);
    return 0;
}

shared_ptr<Node> BinaryOpNode::derivative(const string& var) const {
    if (op == '+') return static_pointer_cast<Node>(make_shared<BinaryOpNode>('+', left->derivative(var), right->derivative(var)));
    if (op == '-') return static_pointer_cast<Node>(make_shared<BinaryOpNode>('-', left->derivative(var), right->derivative(var)));
    if (op == '*') return static_pointer_cast<Node>(make_shared<BinaryOpNode>('+', 
        static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', left->derivative(var), right)), 
        static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', left, right->derivative(var)))));
    if (op == '/') {
        auto num = static_pointer_cast<Node>(make_shared<BinaryOpNode>('-', 
            static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', left->derivative(var), right)), 
            static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', left, right->derivative(var)))));
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('/', num, static_pointer_cast<Node>(make_shared<BinaryOpNode>('^', right, make_shared<NumberNode>(2)))));
    }
    if (op == '^') {
        // Случай: производная от x^n (n — константа): (x^n)' = n * x^(n-1)
        if (dynamic_cast<NumberNode*>(right.get())) {
            auto n = dynamic_pointer_cast<NumberNode>(right);
            auto newExp = make_shared<BinaryOpNode>('-', n, make_shared<NumberNode>(1));
            return static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', 
                static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', n, 
                    static_pointer_cast<Node>(make_shared<BinaryOpNode>('^', left, newExp)))),
                left->derivative(var)));
        }
        // Общий случай: производная от u^v
        auto term1 = static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', right->derivative(var), static_pointer_cast<Node>(make_shared<FunctionNode>("log", left))));
        auto term2 = static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', right, static_pointer_cast<Node>(make_shared<BinaryOpNode>('/', left->derivative(var), left))));
        return static_pointer_cast<Node>(make_shared<BinaryOpNode>('*', 
            static_pointer_cast<Node>(make_shared<BinaryOpNode>('^', left, right)), 
            static_pointer_cast<Node>(make_shared<BinaryOpNode>('+', term1, term2))));
    }
    return make_shared<NumberNode>(0);
}

string BinaryOpNode::toString() const {
    string l = left->toString(), r = right->toString();
    
    // Расстановка скобок для левой части с учётом приоритетов
    if (left->isBinary()) {
        auto lBin = static_cast<BinaryOpNode*>(left.get());
        if ((op == '*' || op == '/' || op == '^') && (lBin->op == '+' || lBin->op == '-')) 
            l = "(" + l + ")";
        if ((op == '/' || op == '-') && (lBin->op == '*' || lBin->op == '/' || lBin->op == '+' || lBin->op == '-'))
            l = "(" + l + ")";
    }
    
    // Расстановка скобок для правой части с учётом приоритетов
    if (right->isBinary()) {
        auto rBin = static_cast<BinaryOpNode*>(right.get());
        if ((op == '*' || op == '/' || op == '^') && (rBin->op == '+' || rBin->op == '-')) 
            r = "(" + r + ")";
        if (op == '/' && rBin->op != '+' && rBin->op != '-') 
            r = "(" + r + ")";
        if (op == '-' && (rBin->op == '+' || rBin->op == '-'))
            r = "(" + r + ")";
        if (op == '^' && !dynamic_cast<NumberNode*>(right.get()))
            r = "(" + r + ")";
    }
    
    return l + " " + op + " " + r;
}

// ПАРСЕР

class Parser {
    vector<string> tokens;
    int pos;
public:
    Parser(const vector<string>& t) : tokens(t), pos(0) {}
    shared_ptr<Node> parse() {
        if (tokens.empty()) throw runtime_error("ERROR Empty expression");
        shared_ptr<Node> res = parseExpr();
        if (pos < tokens.size()) throw runtime_error("ERROR Invalid syntax");
        return res;
    }
private:
    string current() { return pos < tokens.size() ? tokens[pos] : ""; }
    void next() { pos++; }
    
    shared_ptr<Node> parseExpr() {
        shared_ptr<Node> left = parseTerm();
        while (current() == "+" || current() == "-") { 
            char op = current()[0]; next(); 
            left = static_pointer_cast<Node>(make_shared<BinaryOpNode>(op, left, parseTerm())); 
        }
        return left;
    }
    
    shared_ptr<Node> parseTerm() {
        shared_ptr<Node> left = parseUnary();
        while (current() == "*" || current() == "/") { 
            char op = current()[0]; next(); 
            left = static_pointer_cast<Node>(make_shared<BinaryOpNode>(op, left, parseUnary())); 
        }
        return left;
    }
    
    // Обработка цепочек унарных операторов: ++2, +-2, ---2
    shared_ptr<Node> parseUnary() {
        if (current() == "+" || current() == "-") {
            char op = current()[0]; next();
            return static_pointer_cast<Node>(make_shared<UnaryOpNode>(op, parseUnary()));
        }
        return parsePower();
    }
    
    shared_ptr<Node> parsePower() {
        shared_ptr<Node> base = parsePrimary();
        if (current() == "^") { 
            next(); 
            return static_pointer_cast<Node>(make_shared<BinaryOpNode>('^', base, parseUnary())); 
        }
        return base;
    }
    
    shared_ptr<Node> parsePrimary() {
        string tok = current();
        if (tok.empty()) throw runtime_error("ERROR Unexpected end of expression");
        
        if (isdigit(tok[0]) || (tok[0] == '.' && tok.size() > 1 && isdigit(tok[1]))) {
            try { double val = stod(tok); next(); return make_shared<NumberNode>(val); } 
            catch (...) { throw runtime_error("ERROR Invalid number format"); }
        }
        
        if (isalpha(tok[0]) || tok[0] == '_') {
            next();
            string name = tok; transform(name.begin(), name.end(), name.begin(), ::tolower);
            vector<string> funcs = {"sin","cos","tan","asin","acos","atan","exp","log","sqrt"};
            bool isFunc = find(funcs.begin(), funcs.end(), name) != funcs.end();
            if (isFunc) {
                if (current() != "(") throw runtime_error("ERROR Expected '(' after function " + name);
                next(); 
                shared_ptr<Node> arg = parseExpr();
                if (current() != ")") throw runtime_error("ERROR Expected ')'");
                next(); 
                return static_pointer_cast<Node>(make_shared<FunctionNode>(name, arg));
            }
            return make_shared<VariableNode>(name);
        }
        
        if (tok == "(") {
            next(); 
            shared_ptr<Node> node = parseExpr();
            if (current() != ")") throw runtime_error("ERROR Expected ')'");
            next(); 
            return node;
        }
        
        throw runtime_error("ERROR Unexpected token: " + tok);
    }
};

// ЛЕКСЕР

// Проверка корректности числа: запрещены ведущие нули, точки без цифр, некорректная научная нотация
bool isValidNumber(const string& s) {
    if (s.empty()) return false;
    if (s[0] == '0') {
        if (s.length() == 1) return true;
        if (s[1] != '.' && s[1] != 'e' && s[1] != 'E') return false;
    }
    bool hasDigit = false, hasDot = false, hasE = false, digitsAfterDot = false;
    size_t i = 0, len = s.length();
    while (i < len) {
        char c = s[i];
        if (isdigit(c)) { hasDigit = true; if (hasDot) digitsAfterDot = true; i++; }
        else if (c == '.') { if (hasDot || hasE) return false; hasDot = true; i++; }
        else if (c == 'e' || c == 'E') {
            if (hasE || !hasDigit) return false;
            hasE = true; i++;
            if (i < len && (s[i] == '+' || s[i] == '-')) i++;
            bool foundDigits = false;
            while (i < len && isdigit(s[i])) { foundDigits = true; i++; }
            if (!foundDigits) return false;
            if (i < len) return false;
            return true;
        }
        else return false;
    }
    if (hasDot && !digitsAfterDot) return false;
    return hasDigit;
}

// Разбиение строки на токены: числа, переменные, операторы, функции
vector<string> tokenize(const string& s) {
    vector<string> tokens; string cur;
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if (isspace(c)) {
            if (!cur.empty()) {
                if (isdigit(cur[0]) || (cur[0] == '.' && cur.length() > 1 && isdigit(cur[1])))
                    if (!isValidNumber(cur)) throw runtime_error("ERROR");
                tokens.push_back(cur); cur.clear();
            } continue;
        }
        if ((c == 'e' || c == 'E') && !cur.empty() && isdigit(cur.back())) { cur += c; continue; }
        if ((c == '+' || c == '-') && !cur.empty() && (cur.back() == 'e' || cur.back() == 'E')) { cur += c; continue; }
        if (isdigit(c) || (c == '.' && !cur.empty() && isdigit(cur.back()))) { cur += c; continue; }
        if (isalpha(c) || c == '_') {
            if (!cur.empty() && isdigit(cur[0])) throw runtime_error("ERROR");
            cur += c; continue;
        }
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '(' || c == ')') {
            if (!cur.empty()) {
                if (isdigit(cur[0]) || (cur[0] == '.' && cur.length() > 1 && isdigit(cur[1])))
                    if (!isValidNumber(cur)) throw runtime_error("ERROR");
                tokens.push_back(cur); cur.clear();
            }
            tokens.push_back(string(1, c)); continue;
        }
        throw runtime_error("ERROR");
    }
    if (!cur.empty()) {
        if (isdigit(cur[0]) || (cur[0] == '.' && cur.length() > 1 && isdigit(cur[1])))
            if (!isValidNumber(cur)) throw runtime_error("ERROR");
        tokens.push_back(cur);
    }
    for (string& tok : tokens) transform(tok.begin(), tok.end(), tok.begin(), ::tolower);
    return tokens;
}

// ВСПОМОГАТЕЛЬНЫЙ ВЫВОД

// Форматирование результата: 6 значащих цифр, удаление нулей, обработка inf/nan/-0
void printResult(double val) {
    if (isinf(val)) { 
        if (val > 0) cout << "inf\n"; 
        else cout << "-inf\n"; 
        return; 
    }
    if (isnan(val)) { cout << "nan\n"; return; }
    
    // Сохранение знака нуля: -0 выводится как "-0"
    if (val == 0.0) {
        if (signbit(val)) {
            cout << "-0\n";
        } else {
            cout << "0\n";
        }
        return;
    }
    
    // Целые числа выводятся без десятичной части
    if (val == floor(val) && abs(val) < 1e15) { 
        cout << static_cast<long long>(val) << "\n"; 
        return; 
    }
    
    // Вычисление точности: 6 значащих цифр, но не более 6 знаков после запятой
    int precision;
    if (abs(val) < 1.0) {
        double temp = abs(val);
        int leadingZeros = 0;
        while (temp < 1.0 && temp > 1e-15) {
            temp *= 10;
            leadingZeros++;
        }
        precision = min(6, 6 + leadingZeros);
    } else {
        int intDigits = (int)log10(abs(val)) + 1;
        precision = max(1, 6 - intDigits);
        precision = min(precision, 6);
    }
    
    double multiplier = pow(10.0, precision);
    val = round(val * multiplier) / multiplier;
    
    ostringstream oss;
    oss << fixed << setprecision(precision) << val;
    string s = oss.str();
    
    // Удаление конечных нулей после запятой
    if (s.find('.') != string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    cout << s << "\n";
}

// MAIN

int main() {
    ios_base::sync_with_stdio(false); cin.tie(NULL);
    string cmd;
    while (cin >> cmd) {
        try {
            int n; cin >> n;
            vector<string> varNames(n); for (int i=0;i<n;++i) cin >> varNames[i];
            vector<double> varValues(n); for (int i=0;i<n;++i) cin >> varValues[i];
            
            string expression; 
            getline(cin, expression);
            bool only_spaces = true;
            for (char c : expression) if (!isspace(c)) { only_spaces = false; break; }
            if (only_spaces) getline(cin, expression);
            
            map<string, double> vars;
            for (int i=0;i<n;++i) {
                string low = varNames[i]; transform(low.begin(), low.end(), low.begin(), ::tolower);
                vars[low] = varValues[i];
            }
            
            Parser parser(tokenize(expression));
            shared_ptr<Node> root = parser.parse();
            
            if (cmd == "evaluate") { 
                printResult(root->evaluate(vars)); 
            }
            else if (cmd == "derivative") {
                string dv = n>0 ? varNames[0] : "x"; 
                transform(dv.begin(), dv.end(), dv.begin(), ::tolower);
                cout << root->derivative(dv)->toString() << "\n";
            }
            else if (cmd == "evaluate_derivative") {
                string dv = n>0 ? varNames[0] : "x"; 
                transform(dv.begin(), dv.end(), dv.begin(), ::tolower);
                printResult(root->derivative(dv)->evaluate(vars));
            }
            else throw runtime_error("ERROR Unknown command");
            
        } catch (const exception& e) {
            string m = e.what(); 
            if (m.find("ERROR")!=0) m = "ERROR " + m;
            cout << m << "\n";
        }
    }
    return 0;
}