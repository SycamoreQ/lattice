#include <string>
#include <variant>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <vector>


enum class LiteralType {
    LONG,
    DOUBLE,
    STRING,
    IDENTIFIER
};

struct Literal {
    static bool isNumberStart(char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) || ch == '.';
    }

    static bool isIdentifierStart(char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) || ch == '`';
    }

    static bool isIdentifierPart(char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) ||
               std::isdigit(static_cast<unsigned char>(ch)) ||
               ch == '_';
    }

    static bool isCharsStart(char ch) {
        return ch == '\'' || ch == '"';
    }

    static std::string toString(LiteralType type) {
        switch (type) {
            case LiteralType::LONG: return "LONG";
            case LiteralType::DOUBLE: return "DOUBLE";
            case LiteralType::STRING: return "STRING";
            case LiteralType::IDENTIFIER: return "IDENTIFIER";
            default: return "UNKNOWN";
        }
    }
};

enum class Keyword {
    // common
    SCHEMA, DATABASE, TABLE, COLUMN, VIEW, INDEX, TRIGGER, PROCEDURE, TABLESPACE, FUNCTION, SEQUENCE, CURSOR,
    FROM, TO, OF, IF, ON, FOR, WHILE, DO, NO, BY, WITH, WITHOUT, TRUE, FALSE, TEMPORARY, TEMP, COMMENT,
    // create
    CREATE, REPLACE, BEFORE, AFTER, INSTEAD, EACH, ROW, STATEMENT, EXECUTE, BITMAP, NOSORT, REVERSE, COMPILE,
    // alter
    ALTER, ADD, MODIFY, RENAME, ENABLE, DISABLE, VALIDATE, USER, IDENTIFIED,
    // truncate, drop, insert, update, delete
    TRUNCATE, DROP, CASCADE, INSERT, INTO, VALUES, UPDATE, SET, DELETE,
    // select
    SELECT, DISTINCT, LIMIT, AS, CASE, WHEN, ELSE, THEN, END, LEFT, RIGHT, FULL, INNER, OUTER, CROSS, JOIN,
    USE, USING, NATURAL, WHERE, ORDER, ASC, DESC, GROUP, HAVING, UNION,
    // others
    DECLARE, GRANT, FETCH, REVOKE, CLOSE, CAST, NEW, ESCAPE, LOCK, SOME, LEAVE, ITERATE, REPEAT, UNTIL,
    OPEN, OUT, INOUT, OVER, ADVISE, SIBLINGS, LOOP, EXPLAIN, DEFAULT, EXCEPT, INTERSECT, MINUS, PASSWORD,
    LOCAL, GLOBAL, STORAGE, DATA, COALESCE,
    // Types
    CHAR, CHARACTER, VARYING, VARCHAR, VARCHAR2, INTEGER, INT, SMALLINT, DECIMAL, DEC, NUMERIC, FLOAT, REAL,
    DOUBLE, PRECISION, DATE, TIME, INTERVAL, BOOLEAN, BLOB,
    // Conditionals
    AND, OR, XOR, IS, NOT, NULL_KW, IN, BETWEEN, LIKE, ANY, ALL, EXISTS,
    // Functions
    AVG, MAX, MIN, SUM, COUNT, GREATEST, LEAST, ROUND, TRUNC, POSITION, EXTRACT, LENGTH, CHAR_LENGTH, SUBSTRING,
    SUBSTR, INSTR, INITCAP, UPPER, LOWER, TRIM, LTRIM, RTRIM, BOTH, LEADING, TRAILING, TRANSLATE, CONVERT,
    LPAD, RPAD, DECODE, NVL,
    CONSTRAINT, UNIQUE, PRIMARY, FOREIGN, KEY, CHECK, REFERENCES
};

class KeywordUtils {
private:
    static const std::unordered_map<std::string, Keyword>& getMap() {
        static const std::unordered_map<std::string, Keyword> map = {
            // common
            {"SCHEMA", Keyword::SCHEMA}, {"DATABASE", Keyword::DATABASE}, {"TABLE", Keyword::TABLE},
            {"COLUMN", Keyword::COLUMN}, {"VIEW", Keyword::VIEW}, {"INDEX", Keyword::INDEX},
            {"TRIGGER", Keyword::TRIGGER}, {"PROCEDURE", Keyword::PROCEDURE}, {"TABLESPACE", Keyword::TABLESPACE},
            {"FUNCTION", Keyword::FUNCTION}, {"SEQUENCE", Keyword::SEQUENCE}, {"CURSOR", Keyword::CURSOR},
            {"FROM", Keyword::FROM}, {"TO", Keyword::TO}, {"OF", Keyword::OF}, {"IF", Keyword::IF},
            {"ON", Keyword::ON}, {"FOR", Keyword::FOR}, {"WHILE", Keyword::WHILE}, {"DO", Keyword::DO},
            {"NO", Keyword::NO}, {"BY", Keyword::BY}, {"WITH", Keyword::WITH}, {"WITHOUT", Keyword::WITHOUT},
            {"TRUE", Keyword::TRUE}, {"FALSE", Keyword::FALSE}, {"TEMPORARY", Keyword::TEMPORARY},
            {"TEMP", Keyword::TEMP}, {"COMMENT", Keyword::COMMENT},

            // create
            {"CREATE", Keyword::CREATE}, {"REPLACE", Keyword::REPLACE}, {"BEFORE", Keyword::BEFORE},
            {"AFTER", Keyword::AFTER}, {"INSTEAD", Keyword::INSTEAD}, {"EACH", Keyword::EACH},
            {"ROW", Keyword::ROW}, {"STATEMENT", Keyword::STATEMENT}, {"EXECUTE", Keyword::EXECUTE},
            {"BITMAP", Keyword::BITMAP}, {"NOSORT", Keyword::NOSORT}, {"REVERSE", Keyword::REVERSE},
            {"COMPILE", Keyword::COMPILE},

            // alter
            {"ALTER", Keyword::ALTER}, {"ADD", Keyword::ADD}, {"MODIFY", Keyword::MODIFY},
            {"RENAME", Keyword::RENAME}, {"ENABLE", Keyword::ENABLE}, {"DISABLE", Keyword::DISABLE},
            {"VALIDATE", Keyword::VALIDATE}, {"USER", Keyword::USER}, {"IDENTIFIED", Keyword::IDENTIFIED},

            // truncate, drop, insert, update, delete
            {"TRUNCATE", Keyword::TRUNCATE},
            {"DROP", Keyword::DROP}, {"CASCADE", Keyword::CASCADE},
            {"INSERT", Keyword::INSERT}, {"INTO", Keyword::INTO}, {"VALUES", Keyword::VALUES},
            {"UPDATE", Keyword::UPDATE}, {"SET", Keyword::SET},
            {"DELETE", Keyword::DELETE},

            // select
            {"SELECT", Keyword::SELECT}, {"DISTINCT", Keyword::DISTINCT}, {"LIMIT", Keyword::LIMIT},
            {"AS", Keyword::AS}, {"CASE", Keyword::CASE}, {"WHEN", Keyword::WHEN}, {"ELSE", Keyword::ELSE},
            {"THEN", Keyword::THEN}, {"END", Keyword::END}, {"LEFT", Keyword::LEFT}, {"RIGHT", Keyword::RIGHT},
            {"FULL", Keyword::FULL}, {"INNER", Keyword::INNER}, {"OUTER", Keyword::OUTER},
            {"CROSS", Keyword::CROSS}, {"JOIN", Keyword::JOIN}, {"USE", Keyword::USE},
            {"USING", Keyword::USING}, {"NATURAL", Keyword::NATURAL}, {"WHERE", Keyword::WHERE},
            {"ORDER", Keyword::ORDER}, {"ASC", Keyword::ASC}, {"DESC", Keyword::DESC},
            {"GROUP", Keyword::GROUP}, {"HAVING", Keyword::HAVING}, {"UNION", Keyword::UNION},

            // others
            {"DECLARE", Keyword::DECLARE}, {"GRANT", Keyword::GRANT}, {"FETCH", Keyword::FETCH},
            {"REVOKE", Keyword::REVOKE}, {"CLOSE", Keyword::CLOSE}, {"CAST", Keyword::CAST},
            {"NEW", Keyword::NEW}, {"ESCAPE", Keyword::ESCAPE}, {"LOCK", Keyword::LOCK},
            {"SOME", Keyword::SOME}, {"LEAVE", Keyword::LEAVE}, {"ITERATE", Keyword::ITERATE},
            {"REPEAT", Keyword::REPEAT}, {"UNTIL", Keyword::UNTIL}, {"OPEN", Keyword::OPEN},
            {"OUT", Keyword::OUT}, {"INOUT", Keyword::INOUT}, {"OVER", Keyword::OVER},
            {"ADVISE", Keyword::ADVISE}, {"SIBLINGS", Keyword::SIBLINGS}, {"LOOP", Keyword::LOOP},
            {"EXPLAIN", Keyword::EXPLAIN}, {"DEFAULT", Keyword::DEFAULT}, {"EXCEPT", Keyword::EXCEPT},
            {"INTERSECT", Keyword::INTERSECT}, {"MINUS", Keyword::MINUS}, {"PASSWORD", Keyword::PASSWORD},
            {"LOCAL", Keyword::LOCAL}, {"GLOBAL", Keyword::GLOBAL}, {"STORAGE", Keyword::STORAGE},
            {"DATA", Keyword::DATA}, {"COALESCE", Keyword::COALESCE},

            // Types
            {"CHAR", Keyword::CHAR}, {"CHARACTER", Keyword::CHARACTER}, {"VARYING", Keyword::VARYING},
            {"VARCHAR", Keyword::VARCHAR}, {"VARCHAR2", Keyword::VARCHAR2}, {"INTEGER", Keyword::INTEGER},
            {"INT", Keyword::INT}, {"SMALLINT", Keyword::SMALLINT}, {"DECIMAL", Keyword::DECIMAL},
            {"DEC", Keyword::DEC}, {"NUMERIC", Keyword::NUMERIC}, {"FLOAT", Keyword::FLOAT},
            {"REAL", Keyword::REAL}, {"DOUBLE", Keyword::DOUBLE}, {"PRECISION", Keyword::PRECISION},
            {"DATE", Keyword::DATE}, {"TIME", Keyword::TIME}, {"INTERVAL", Keyword::INTERVAL},
            {"BOOLEAN", Keyword::BOOLEAN}, {"BLOB", Keyword::BLOB},

            // Conditionals
            {"AND", Keyword::AND}, {"OR", Keyword::OR}, {"XOR", Keyword::XOR}, {"IS", Keyword::IS},
            {"NOT", Keyword::NOT}, {"NULL", Keyword::NULL_KW}, {"IN", Keyword::IN}, {"BETWEEN", Keyword::BETWEEN},
            {"LIKE", Keyword::LIKE}, {"ANY", Keyword::ANY}, {"ALL", Keyword::ALL}, {"EXISTS", Keyword::EXISTS},

            // Functions
            {"AVG", Keyword::AVG}, {"MAX", Keyword::MAX}, {"MIN", Keyword::MIN}, {"SUM", Keyword::SUM},
            {"COUNT", Keyword::COUNT}, {"GREATEST", Keyword::GREATEST}, {"LEAST", Keyword::LEAST},
            {"ROUND", Keyword::ROUND}, {"TRUNC", Keyword::TRUNC}, {"POSITION", Keyword::POSITION},
            {"EXTRACT", Keyword::EXTRACT}, {"LENGTH", Keyword::LENGTH}, {"CHAR_LENGTH", Keyword::CHAR_LENGTH},
            {"SUBSTRING", Keyword::SUBSTRING}, {"SUBSTR", Keyword::SUBSTR}, {"INSTR", Keyword::INSTR},
            {"INITCAP", Keyword::INITCAP}, {"UPPER", Keyword::UPPER}, {"LOWER", Keyword::LOWER},
            {"TRIM", Keyword::TRIM}, {"LTRIM", Keyword::LTRIM}, {"RTRIM", Keyword::RTRIM},
            {"BOTH", Keyword::BOTH}, {"LEADING", Keyword::LEADING}, {"TRAILING", Keyword::TRAILING},
            {"TRANSLATE", Keyword::TRANSLATE}, {"CONVERT", Keyword::CONVERT}, {"LPAD", Keyword::LPAD},
            {"RPAD", Keyword::RPAD}, {"DECODE", Keyword::DECODE}, {"NVL", Keyword::NVL},

            // Constraints
            {"CONSTRAINT", Keyword::CONSTRAINT}, {"UNIQUE", Keyword::UNIQUE}, {"PRIMARY", Keyword::PRIMARY},
            {"FOREIGN", Keyword::FOREIGN}, {"KEY", Keyword::KEY}, {"CHECK", Keyword::CHECK},
            {"REFERENCES", Keyword::REFERENCES}
        };
        return map;
    }

public:
    static std::optional<Keyword> textOf(const std::string& text) {
        std::string upperText = text;
        std::transform(upperText.begin(), upperText.end(), upperText.begin(), ::toupper);

        const auto& map = getMap();
        auto it = map.find(upperText);
        if (it != map.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    static std::string toString(Keyword kw) {
        if (kw == Keyword::NULL_KW) return "NULL";

        for (const auto& pair : getMap()) {
            if (pair.second == kw) {
                return pair.first;
            }
        }
        return "UNKNOWN_KEYWORD";
    }
};

enum class Symbol {
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, LEFT_BRACKET, RIGHT_BRACKET,
    SEMI, COMMA, DOT, DOUBLE_DOT, PLUS, SUB, STAR, SLASH, QUESTION, EQ, GT, LT,
    BANG, TILDE, CARET, PERCENT, COLON, DOUBLE_COLON, COLON_EQ, LT_EQ, GT_EQ,
    LT_EQ_GT, LT_GT, BANG_EQ, BANG_GT, BANG_LT, AMP, BAR, DOUBLE_AMP, DOUBLE_BAR,
    DOUBLE_LT, DOUBLE_GT, AT, POUND
};

class SymbolUtils {
private:
    struct SymbolData {
        std::string text;
        Symbol value;
    };

    static const std::vector<SymbolData>& getSymbols() {
        static const std::vector<SymbolData> symbols = {
            {"(", Symbol::LEFT_PAREN}, {")", Symbol::RIGHT_PAREN}, {"{", Symbol::LEFT_BRACE},
            {"}", Symbol::RIGHT_BRACE}, {"[", Symbol::LEFT_BRACKET}, {"]", Symbol::RIGHT_BRACKET},
            {";", Symbol::SEMI}, {",", Symbol::COMMA}, {".", Symbol::DOT}, {"..", Symbol::DOUBLE_DOT},
            {"+", Symbol::PLUS}, {"-", Symbol::SUB}, {"*", Symbol::STAR}, {"/", Symbol::SLASH},
            {"?", Symbol::QUESTION}, {"=", Symbol::EQ}, {">", Symbol::GT}, {"<", Symbol::LT},
            {"!", Symbol::BANG}, {"~", Symbol::TILDE}, {"^", Symbol::CARET}, {"%", Symbol::PERCENT},
            {":", Symbol::COLON}, {"::", Symbol::DOUBLE_COLON}, {":=", Symbol::COLON_EQ},
            {"<=", Symbol::LT_EQ}, {">=", Symbol::GT_EQ}, {"<=>", Symbol::LT_EQ_GT}, {"<>", Symbol::LT_GT},
            {"!=", Symbol::BANG_EQ}, {"!>", Symbol::BANG_GT}, {"!<", Symbol::BANG_LT},
            {"&", Symbol::AMP}, {"|", Symbol::BAR}, {"&&", Symbol::DOUBLE_AMP}, {"||", Symbol::DOUBLE_BAR},
            {"<<", Symbol::DOUBLE_LT}, {">>", Symbol::DOUBLE_GT}, {"@", Symbol::AT}, {"#", Symbol::POUND}
        };
        return symbols;
    }

    static const std::unordered_set<char>& getSymbolStartSet() {
        static std::unordered_set<char> charSet;
        if (charSet.empty()) {
            for (const auto& sym : getSymbols()) {
                for (char ch : sym.text) {
                    charSet.insert(ch);
                }
            }
        }
        return charSet;
    }

public:
    static std::optional<Symbol> textOf(const std::string& text) {
        for (const auto& sym : getSymbols()) {
            if (sym.text == text) {
                return sym.value;
            }
        }
        return std::nullopt;
    }

    static std::string getText(Symbol s) {
        for (const auto& sym : getSymbols()) {
            if (sym.value == s) {
                return sym.text;
            }
        }
        return "";
    }

    static bool isSymbol(char ch) {
        const auto& charSet = getSymbolStartSet();
        return charSet.find(ch) != charSet.end();
    }

    static bool isSymbolStart(char ch) {
        return isSymbol(ch);
    }
};

using TokenType = std::variant<LiteralType, Keyword, Symbol>;

struct Token {
    std::string text;
    TokenType type;
    int endOffset;

    std::string toString() const {
        std::stringstream ss;
        ss << "Token(\"" << text << "\", ";

        // std::visit is used to branch based on which type the variant currently holds
        std::visit([&ss](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, Keyword>) {
                ss << "Keyword." << KeywordUtils::toString(arg);
            }
            else if constexpr (std::is_same_v<T, Symbol>) {
                ss << "Symbol." << SymbolUtils::getText(arg); // or Enum name depending on preference
            }
            else if constexpr (std::is_same_v<T, LiteralType>) {
                ss << "Literal." << Literal::toString(arg);
            }
        }, type);

        ss << ", " << endOffset << ")";
        return ss.str();
    }
};
