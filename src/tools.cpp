#include "../inc/tools.hpp"
#include "../inc/errors.hpp"
#include "../inc/runner.hpp"
#include "../inc/expressions.hpp"

MEOWSCRIPT_SOURCE_FILE

argument_list MeowScript::tools::parse_argument_list(GeneralTypeToken context) {
    return parse_argument_list(context.source);
}

argument_list MeowScript::tools::parse_argument_list(Token context) {
    argument_list ret;
    if(context.content == "" || context.in_quotes || !brace_check(context,'(',')')) {
        return argument_list();
    }
    context.content.erase(context.content.begin());
    context.content.erase(context.content.begin()+context.content.size()-1);
    if(context.content.size() == 0) {
        return argument_list();
    }

    auto lexed = lex_text(context);
    std::vector<Token> line;
    for(auto i : lexed)
        for(auto j : i.source) 
            line.push_back(j);
    
    GeneralTypeToken last = general_null;
    bool found_sl = true;

    for(auto i : line) {
        if(i.content == ",") {
            if(last == general_null) {
                return argument_list();
            }
            ret.push_back(last);
            last = general_null;
            found_sl = true;
        }
        else if(found_sl) {
            last = i;
            found_sl = false;
        }
        else {
            return argument_list();
        }
    }
    if(last != general_null) {
        if(!found_sl) {
            ret.push_back(last);
        }
        else {
            return argument_list();
        }
    }

    return ret;
}

const std::vector<std::pair<GeneralTypeToken,GeneralTypeToken>> Dictionary::pairs() const {
    std::vector<std::pair<GeneralTypeToken,GeneralTypeToken>> ret;
    for(size_t i = 0; i < keys().size(); ++i) {
        ret.push_back(std::make_pair(keys()[i],values()[i]));
    }
    return ret;
}
std::vector<std::pair<GeneralTypeToken,GeneralTypeToken>> Dictionary::pairs() {
    std::vector<std::pair<GeneralTypeToken,GeneralTypeToken>> ret;
    for(size_t i = 0; i < keys().size(); ++i) {
        ret.push_back(std::make_pair(keys()[i],values()[i]));
    }
    return ret;
}

const std::vector<GeneralTypeToken>& Dictionary::keys() const {
    return i_keys;
}
const std::vector<GeneralTypeToken>& Dictionary::values() const {
    return i_values;
}
std::vector<GeneralTypeToken>& Dictionary::keys() {
    return i_keys;
}
std::vector<GeneralTypeToken>& Dictionary::values() {
    return i_values;
}

bool Dictionary::has(const GeneralTypeToken gtt) const {
    for(size_t i = 0; i < keys().size(); ++i) {
        if(keys()[i] == gtt) {
            return true;
        }
    }
    return false;
}

GeneralTypeToken& Dictionary::operator[](GeneralTypeToken gtt) {
    for(size_t i = 0; i < keys().size(); ++i) {
        if(keys()[i] == gtt) {
            return values()[i];
        }
    }

    keys().push_back(gtt);
    values().push_back(general_null);
    return values().back();
}

std::vector<Parameter> MeowScript::tools::parse_function_params(Token context) {
    if(brace_check(context,'(',')')) {
        context.content.erase(context.content.begin());
        context.content.erase(context.content.begin()+context.content.size()-1);
    }
    auto lines = lex_text(context.content);
    if(lines.empty()) {
        return {};
    }
    std::vector<Parameter> ret;
    std::vector<Token> line;
    for(auto i : lines) {
        for(auto j : i.source) {
            line.push_back(j);
        }
    }

    std::vector<std::vector<Token>> arguments;
    if(line.size() != 0)
        arguments.push_back({});
    for(size_t i = 0; i < line.size(); ++i) {
        if(line[i].content == ",") {
            if(arguments.back().empty()) {
                throw errors::MWSMessageException{"Invalid parameter format!",global::get_line()};
            }
            arguments.push_back({});
        }
        else {
            arguments.back().push_back(line[i]);
        }
    }

    for(auto i : arguments) {
        for(size_t j = 0; j < i.size(); ++j) {
            if(i[j].content == "" && !i[j].in_quotes) {
                i.erase(i.begin()+j);
                --j;
            }
        }

        if(i.size() == 1) {
            if(!is_valid_name(i[0]) && !is_literal_value(i[0])) {
                throw errors::MWSMessageException{"Unexpected token: " + i[0].content,global::get_line()};
            }
            Parameter p = Parameter(Variable::Type::UNKNOWN,i[0]);
            if(is_literal_value(i[0])) {
                p.literal_value = make_variable(i[0]);
                p.name = "";
            }
            ret.push_back(p);
        }
        else if(i.size() == 2) {
            if(!is_valid_name(i[0]) && !is_literal_value(i[0])) {
                throw errors::MWSMessageException{"Unexpected token: " + i[0].content,global::get_line()};
            }
            if(!is_ruleset(i[1])) {
                throw errors::MWSMessageException{"Unexpected token: " + i[1].content,global::get_line()};
            }
            Parameter p = Parameter(Variable::Type::UNKNOWN,i[0]);
            if(is_literal_value(i[0])) {
                p.literal_value = make_variable(i[0]);
                p.name = "";
            }

            RuleSet ruleset = construct_ruleset(i[1]);
            p.ruleset = ruleset;

            ret.push_back(p);
        }
        else if(i.size() == 3) {
            if(!is_valid_name(i[0]) && !is_literal_value(i[0])) {
                throw errors::MWSMessageException{"Unexpected token: " + i[0].content,global::get_line()};
            }
            if(i[1].content != "::" && i[1].content != ":") {
                throw errors::MWSMessageException{"Invalid parameter format!",global::get_line()};
            }
            if(!is_struct(i[2]) && !is_valid_var_t(i[2])) {
                throw errors::MWSMessageException{i[2].content + " is not a known VariableType or struct!",global::get_line()};
            }
            if(is_struct(i[2])) {
                ret.push_back(Parameter(Variable::Type::Object,i[0],i[2]));
            }
            else {
                Parameter p = Parameter(token2var_t(i[2]),i[0]);
                if(is_literal_value(i[0])) {
                    p.literal_value = make_variable(i[0]);
                    p.name = "";
                }
                ret.push_back(p);
            }
        }
        else if(i.size() == 4) {
            if(!is_valid_name(i[0]) && !is_literal_value(i[0])) {
                throw errors::MWSMessageException{"Unexpected token: " + i[0].content,global::get_line()};
            }
            if(!is_ruleset(i[1])) {
                throw errors::MWSMessageException{"Unexpected token: " + i[1].content,global::get_line()};
            }
            if(i[2].content != "::" && i[2].content != ":") {
                throw errors::MWSMessageException{"Invalid parameter format!",global::get_line()};
            }
            if(!is_struct(i[3]) && !is_valid_var_t(i[3])) {
                throw errors::MWSMessageException{i[3].content + " is not a known VariableType or struct!",global::get_line()};
            }
            Parameter p;
            RuleSet ruleset = construct_ruleset(i[1]);
            p.ruleset = ruleset;

            if(is_struct(i[2])) {
                p.type = Variable::Type::Object;
                p.name = i[0];
                p.struct_name = i[3];
                ret.push_back(p);
            }
            else {
                p.type = token2var_t(i[3]);
                p.name = i[0];
                if(is_literal_value(i[0])) {
                    p.literal_value = make_variable(i[0]);
                    p.name = "";
                }
                ret.push_back(p);
            }
        }
        else {
            throw errors::MWSMessageException{"Invalid parameter format!",global::get_line()};
        }
    }
    return ret;
}

GeneralTypeToken MeowScript::tools::check4var(GeneralTypeToken token) {
    if(token.type == General_type::NAME) {
        Variable* var = get_variable(token.source);
        if(var != nullptr) {
            return GeneralTypeToken(*var);
        }
    }
    return token;
}

GeneralTypeToken MeowScript::tools::check4compound(GeneralTypeToken token) {
    if(token.type == General_type::COMPOUND) {
        token.source.content.erase(token.source.content.begin());
        token.source.content.erase(token.source.content.begin()+token.source.content.size()-1);
        ++global::in_compound;
        int saved_istruct = global::in_struct;
        global::in_struct = 0;
        GeneralTypeToken ret = run_text(token.source,false);
        global::in_struct = saved_istruct;
        --global::in_compound;
        //if(ret.type == General_type::VOID) {
        //    throw errors::MWSMessageException{"Compound did not return anything!",global::get_line()};
        //}
        return ret;
    }
    return token;
}

GeneralTypeToken MeowScript::tools::check4placeholder(GeneralTypeToken token) {
    return check4var(check4compound(check4expression(token)));
}

GeneralTypeToken MeowScript::tools::check4expression(GeneralTypeToken token) {
    if(token.type == General_type::EXPRESSION) {
        Variable ret = parse_expression(token.to_string());
        return GeneralTypeToken(ret);
    }
    return token;
}

GeneralTypeToken MeowScript::tools::check4replace(GeneralTypeToken token) {
    if(token.type == General_type::NAME) {
        if(replaces.count(token.source.content) != 0) {
            return replaces[token.source.content];
        }
        return token;
    }
    return token;
}

Token MeowScript::tools::check4replace(Token token) {
    if(replaces.count(token.content) != 0 && !token.in_quotes) {
        return replaces[token.content].to_string();
    }
    return token;
}

Token MeowScript::tools::remove_unneeded_chars(Token token) {
    if(token.in_quotes) {
        return token;
    }
    size_t i = 0;
    for(i = 0; i < token.content.size(); ++i) {
        if(token.content[i] != ' ' && token.content[i] != '\t' && !is_newline(token.content[i])) {
            break;
        }
    }
    token.content.erase(token.content.begin(),token.content.begin()+i);
    for(i = token.content.size(); i != -1; --i) {
        if(token.content[i] != ' ' && token.content[i] != '\t' && !is_newline(token.content[i])) {
            break;
        }
    }
    token.content.erase(token.content.begin()+i,token.content.end());
    return token;
}


Token MeowScript::tools::remove_uness_decs(Token num, bool to_int) {
    if(num.in_quotes) {
        return num;
    }
    bool has_dot = false;
    bool all_zero = true;
    for(auto i : num.content) {
        //                      vvvvvvvv - just in case
        all_zero &= (i == '0' || i == '.');
        if(i == '.') has_dot = true;
    }
    if(all_zero) {
        return "0";
    }
    if(!has_dot) {
        return num;
    }

    if(to_int) {
        for(size_t i = 0; i < num.content.size(); ++i) {
            if(num.content[i] == '.') {
                num.content.erase(num.content.begin()+i,num.content.end());
                return num;
            }
        }
        return num;
    }

    for(size_t i = num.content.size()-1; i != -1; --i) {
        if(num.content[i] == '.') {
            num.content.erase(num.content.begin()+i);
            break;
        }
        else if(num.content[i] == '0') {
            num.content.erase(num.content.begin()+i);
        }
        else {
            break;
        }
    }

    return num;
}

Token MeowScript::tools::until_newline(std::vector<Token> tks) {
    Token ret;
    for(auto i : tks) {
        for(auto j : i.content) {
            if(j == '\n') {
                return ret;
            }
            ret.content += j;
        }
        ret.content += " ";
    }
    ret.content.pop_back();
    return ret;
}
