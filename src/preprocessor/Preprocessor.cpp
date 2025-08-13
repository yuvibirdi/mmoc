#include "preprocessor/Preprocessor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>
#include <functional>
#include <cctype>

namespace preprocessor {

std::string Preprocessor::preprocess(const std::string &inputFile, const std::string &outputFile) {
    log("Preprocessing " + inputFile);

    // initialize macros from raw definitions (once per preprocess)
    macros_.clear();
    for (const auto &spec : macroDefinitions_) {
        defineMacroFromSpec(spec);
    }

    std::string result = preprocessFileInternal(inputFile);

    if (!outputFile.empty()) {
        std::ofstream file(outputFile);
        if (!file) {
            throw std::runtime_error("Cannot write to output file: " + outputFile);
        }
        file << result;
        file.close();
        log("Preprocessed output written to " + outputFile);
    }
    return result;
}

void Preprocessor::addIncludeDirectory(const std::string &dir) {
    includeDirs_.push_back(dir);
    log("Added include directory: " + dir);
}

void Preprocessor::addMacroDefinition(const std::string &macro) {
    macroDefinitions_.push_back(macro);
    log("Added macro definition: " + macro);
}

// --- Core processing ---
std::string Preprocessor::preprocessFileInternal(const std::string &filePath) {
    std::string content = readFileToString(filePath);
    std::string dir = std::filesystem::path(filePath).parent_path().string();
    return preprocessStringInternal(content, dir);
}

std::string Preprocessor::preprocessStringInternal(const std::string &source, const std::string &currentFileDir) {
    std::istringstream in(source);
    std::ostringstream out;

    ifStack_.clear();

    std::string line;
    while (std::getline(in, line)) {
        // Handle CRLF
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::string t = trim(line);
        if (t.rfind('#', 0) == 0) {
            // Preprocessor directive
            if (handleDirective(t, currentFileDir, out, isCurrentlyActive())) {
                continue;
            }
            continue;
        }

        if (!isCurrentlyActive()) {
            continue; // skip inactive regions
        }

        // Expand macros in non-directive lines
        std::string expanded = expandMacros(line);
        out << expanded << '\n';
    }

    if (!ifStack_.empty()) {
        throw std::runtime_error("Unterminated #if/#ifdef block");
    }

    return out.str();
}

std::string Preprocessor::resolveInclude(const std::string &target, bool isSystem, const std::string &currentFileDir) {
    namespace fs = std::filesystem;

    auto tryPaths = [&](const std::vector<std::string> &dirs) -> std::string {
        for (const auto &d : dirs) {
            fs::path p = fs::path(d) / target;
            std::error_code ec;
            if (fs::exists(p, ec) && fs::is_regular_file(p, ec)) {
                return p.string();
            }
        }
        return {};
    };

    if (!isSystem) {
        // current file dir first for "..."
        if (!currentFileDir.empty()) {
            std::string p = tryPaths({currentFileDir});
            if (!p.empty()) return p;
        }
    }

    // then user-provided include dirs
    if (!includeDirs_.empty()) {
        std::string p = tryPaths(includeDirs_);
        if (!p.empty()) return p;
    }

    // finally, try current working directory as last resort
    std::string p = tryPaths({std::filesystem::current_path().string()});
    return p;
}

std::string Preprocessor::readFileToString(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool Preprocessor::handleDirective(const std::string &line, const std::string &currentFileDir, std::ostringstream &out, bool isActive) {
    // line starts with '#'
    size_t i = 1;
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;

    // Get directive keyword
    size_t start = i;
    while (i < line.size() && std::isalpha(static_cast<unsigned char>(line[i]))) ++i;
    std::string keyword = line.substr(start, i - start);

    // Rest of the line
    std::string rest = trim(line.substr(i));

    if (keyword == "include") {
        return handleInclude(rest, currentFileDir, out, isCurrentlyActive());
    } else if (keyword == "define") {
        if (isActive) handleDefine(rest);
        return true;
    } else if (keyword == "undef") {
        if (isActive) handleUndef(rest);
        return true;
    } else if (keyword == "ifdef") {
        bool cond = macros_.find(rest) != macros_.end();
        pushIf(cond);
        return true;
    } else if (keyword == "ifndef") {
        bool cond = macros_.find(rest) == macros_.end();
        pushIf(cond);
        return true;
    } else if (keyword == "if") {
        bool cond = evalExpr(rest);
        pushIf(cond);
        return true;
    } else if (keyword == "elif") {
        handleElif(rest);
        return true;
    } else if (keyword == "else") {
        handleElse();
        return true;
    } else if (keyword == "endif") {
        popIf();
        return true;
    } else if (keyword == "pragma" || keyword == "line" || keyword == "error" || keyword == "warning") {
        // Ignore pragmas and diagnostics for now
        return true;
    }

    // Unknown directive: ignore
    return true;
}

void Preprocessor::handleDefine(const std::string &rest) {
    // Parse: NAME[ (params) ] [replacement]
    size_t i = 0;
    while (i < rest.size() && std::isspace(static_cast<unsigned char>(rest[i]))) ++i;
    size_t start = i;
    while (i < rest.size() && isIdentChar(rest[i])) ++i;
    std::string name = rest.substr(start, i - start);

    Macro m;

    // Function-like?
    if (i < rest.size() && rest[i] == '(') {
        m.functionLike = true;
        ++i; // skip '('
        std::string params;
        int depth = 1;
        while (i < rest.size() && depth > 0) {
            if (rest[i] == '(') depth++;
            else if (rest[i] == ')') depth--;
            if (depth > 0) params.push_back(rest[i]);
            ++i;
        }
        splitCommaArgs(params, m.params);
        // skip spaces
        while (i < rest.size() && std::isspace(static_cast<unsigned char>(rest[i]))) ++i;
    } else {
        // skip spaces after name
        while (i < rest.size() && std::isspace(static_cast<unsigned char>(rest[i]))) ++i;
    }

    m.body = rest.substr(i);
    // trim trailing spaces
    m.body = trim(m.body);

    macros_[name] = std::move(m);
}

void Preprocessor::handleUndef(const std::string &rest) {
    std::string name = trim(rest);
    macros_.erase(name);
}

bool Preprocessor::handleInclude(const std::string &rest, const std::string &currentFileDir, std::ostringstream &out, bool isActive) {
    if (!isActive) return true; // ignore include in inactive blocks

    std::string r = trim(rest);
    if (r.size() < 2) return true;

    bool system = false;
    std::string target;
    if (r.front() == '"' && r.back() == '"') {
        target = r.substr(1, r.size() - 2);
    } else if (r.front() == '<' && r.back() == '>') {
        system = true;
        target = r.substr(1, r.size() - 2);
    } else {
        // Could be macro-expanded include; try expansion then parse quotes/angles.
        std::string expanded = expandMacros(r);
        expanded = trim(expanded);
        if (expanded.size() >= 2 && ((expanded.front() == '"' && expanded.back() == '"') || (expanded.front() == '<' && expanded.back() == '>'))) {
            system = (expanded.front() == '<');
            target = expanded.substr(1, expanded.size() - 2);
        } else {
            return true; // ignore invalid include
        }
    }

    std::string path = resolveInclude(target, system, currentFileDir);
    if (path.empty()) {
        throw std::runtime_error("Include not found: " + target);
    }

    // Recursively preprocess included file
    std::string included = preprocessFileInternal(path);

    // Append to output
    out << included;
    return true;
}

bool Preprocessor::isCurrentlyActive() const {
    bool active = true;
    for (const auto &f : ifStack_) {
        active = active && f.parentActive && f.thisActive;
        if (!active) break;
    }
    return active;
}

void Preprocessor::pushIf(bool cond) {
    bool parentActive = isCurrentlyActive();
    IfFrame frame{parentActive, parentActive && cond, cond && parentActive};
    ifStack_.push_back(frame);
}

void Preprocessor::handleElse() {
    if (ifStack_.empty()) throw std::runtime_error("#else without matching #if");
    auto &f = ifStack_.back();
    if (!f.parentActive) { f.thisActive = false; return; }
    if (f.anyTrue) { f.thisActive = false; }
    else { f.thisActive = true; f.anyTrue = true; }
}

void Preprocessor::handleElif(const std::string &expr) {
    if (ifStack_.empty()) throw std::runtime_error("#elif without matching #if");
    auto &f = ifStack_.back();
    if (!f.parentActive) { f.thisActive = false; return; }
    if (f.anyTrue) { f.thisActive = false; return; }
    bool cond = evalExpr(expr);
    f.thisActive = cond; f.anyTrue = cond || f.anyTrue;
}

void Preprocessor::popIf() {
    if (ifStack_.empty()) throw std::runtime_error("#endif without matching #if");
    ifStack_.pop_back();
}

bool Preprocessor::evalExpr(const std::string &expr) {
    // Very small evaluator: handles defined(NAME), !, decimal integers, identifiers -> 1 if defined else 0
    // Also supports || and && and parentheses minimally.

    struct Tok { enum T{ID, NUM, LP, RP, NOT, AND, OR, DEFINED, END} t; std::string v; };
    std::vector<Tok> toks;
    size_t i = 0; auto s = trim(expr);
    auto push = [&](Tok::T t, std::string v=""){ toks.push_back(Tok{t,std::move(v)}); };
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (std::isspace(c)) { ++i; continue; }
        if (std::isalpha(c) || c == '_') {
            size_t j = i+1; while (j < s.size() && (std::isalnum(static_cast<unsigned char>(s[j])) || s[j]=='_')) ++j;
            std::string id = s.substr(i, j-i);
            if (id == "defined") push(Tok::DEFINED);
            else push(Tok::ID, id);
            i = j; continue;
        }
        if (std::isdigit(c)) { size_t j=i+1; while (j<s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) ++j; push(Tok::NUM, s.substr(i, j-i)); i=j; continue; }
        if (s.compare(i,2,"&&")==0) { push(Tok::AND); i+=2; continue; }
        if (s.compare(i,2,"||")==0) { push(Tok::OR); i+=2; continue; }
        if (s[i]=='!') { push(Tok::NOT); ++i; continue; }
        if (s[i]=='(') { push(Tok::LP); ++i; continue; }
        if (s[i]==')') { push(Tok::RP); ++i; continue; }
        ++i;
    }
    push(Tok::END);

    size_t p = 0;
    std::function<int()> parseF, parseT, parseE;
    parseF = [&]() -> int {
        auto tok = toks[p];
        if (tok.t == Tok::NOT) { ++p; return !parseF(); }
        if (tok.t == Tok::LP) { ++p; int v = parseE(); if (toks[p].t==Tok::RP) ++p; return v; }
        if (tok.t == Tok::DEFINED) {
            if (toks[p+1].t == Tok::LP) { p+=2; std::string id = toks[p].v; if (toks[p].t==Tok::ID) ++p; if (toks[p].t==Tok::RP) ++p; return macros_.count(id) ? 1:0; }
            else { ++p; std::string id = toks[p].v; if (toks[p].t==Tok::ID) ++p; return macros_.count(id)?1:0; }
        }
        if (tok.t == Tok::ID) { ++p; return macros_.count(tok.v)?1:0; }
        if (tok.t == Tok::NUM) { ++p; return std::stoi(tok.v); }
        return 0;
    };
    parseT = [&]() -> int { int v = parseF(); while (toks[p].t == Tok::AND) { ++p; v = (v && parseF()) ? 1:0; } return v; };
    parseE = [&]() -> int { int v = parseT(); while (toks[p].t == Tok::OR) { ++p; v = (v || parseT()) ? 1:0; } return v; };

    int v = parseE();
    return v != 0;
}

void Preprocessor::defineMacroFromSpec(const std::string &spec) {
    auto eq = spec.find('=');
    if (eq == std::string::npos) {
        macros_[trim(spec)] = Macro{false, {}, "1"};
    } else {
        std::string name = spec.substr(0, eq);
        std::string value = spec.substr(eq + 1);
        macros_[trim(name)] = Macro{false, {}, trim(value)};
    }
}

bool Preprocessor::isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool Preprocessor::isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

std::string Preprocessor::expandMacros(const std::string &line) {
    std::string out;
    out.reserve(line.size());

    for (size_t i = 0; i < line.size(); ) {
        char c = line[i];
        if (isIdentStart(c)) {
            size_t j = i+1; while (j < line.size() && isIdentChar(line[j])) ++j;
            std::string name = line.substr(i, j-i);
            auto it = macros_.find(name);
            if (it == macros_.end()) {
                out.append(name);
                i = j; continue;
            }

            const Macro &m = it->second;
            if (!m.functionLike) {
                out.append(m.body);
                i = j; continue;
            }
            size_t k = j; while (k < line.size() && std::isspace(static_cast<unsigned char>(line[k]))) ++k;
            if (k >= line.size() || line[k] != '(') {
                out.append(name);
                i = j; continue;
            }
            ++k;
            int depth = 1; std::string argsStr;
            while (k < line.size() && depth > 0) {
                if (line[k] == '(') depth++;
                else if (line[k] == ')') depth--;
                if (depth > 0) argsStr.push_back(line[k]);
                ++k;
            }
            std::vector<std::string> args; splitCommaArgs(argsStr, args);
            std::string rep = m.body;
            for (size_t idx = 0; idx < m.params.size() && idx < args.size(); ++idx) {
                std::string param = m.params[idx];
                std::string tmp; tmp.reserve(rep.size());
                for (size_t a = 0; a < rep.size(); ) {
                    if (rep.compare(a, param.size(), param) == 0 &&
                        (a==0 || !isIdentChar(rep[a-1])) &&
                        (a+param.size()>=rep.size() || !isIdentChar(rep[a+param.size()]))) {
                        tmp.append(trim(args[idx]));
                        a += param.size();
                    } else {
                        tmp.push_back(rep[a]);
                        ++a;
                    }
                }
                rep.swap(tmp);
            }
            out.append(rep);
            i = k; continue;
        } else {
            out.push_back(c); ++i;
        }
    }

    return out;
}

std::string Preprocessor::trim(const std::string &s) {
    size_t a = 0; while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    size_t b = s.size(); while (b > a && std::isspace(static_cast<unsigned char>(s[b-1]))) --b;
    return s.substr(a, b-a);
}

void Preprocessor::splitCommaArgs(const std::string &s, std::vector<std::string> &out) {
    out.clear();
    std::string cur; int depth = 0; bool inStr=false; char strCh=0;
    for (size_t i=0;i<s.size();++i){
        char c = s[i];
        if (inStr) {
            cur.push_back(c);
            if (c == strCh && (i==0 || s[i-1] != '\\')) inStr=false;
            continue;
        }
        if (c=='"' || c=='\'') { inStr=true; strCh=c; cur.push_back(c); continue; }
        if (c=='(') { depth++; cur.push_back(c); continue; }
        if (c==')') { depth--; cur.push_back(c); continue; }
        if (c==',' && depth==0) { out.push_back(trim(cur)); cur.clear(); continue; }
        cur.push_back(c);
    }
    if (!cur.empty() || !s.empty()) out.push_back(trim(cur));
}

void Preprocessor::log(const std::string &message) {
    if (verbose_) {
        std::cerr << "[Preprocessor] " << message << std::endl;
    }
}

} // namespace preprocessor
