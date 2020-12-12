#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"

extern int semant_debug;
extern char *curr_filename;

static ostream& error_stream = cerr;
static int semant_errors = 0;
static Decl curr_decl = 0;

typedef SymbolTable<Symbol, Symbol> ObjectEnvironment; // name, type

typedef std::map<Symbol, Symbol> Map;
Map callMap;
Map gvMap;
Map lvMap;



ObjectEnvironment objectEnv;
ObjectEnvironment CallEnv;//call
ObjectEnvironment GvEnv;//globalvar
ObjectEnvironment LvEnv;//localvar


///////////////////////////////////////////////
// helper func
///////////////////////////////////////////////


static ostream& semant_error() {
    semant_errors++;
    return error_stream;
}

static ostream& semant_error(tree_node *t) {
    error_stream << t->get_line_number() << ": ";
    return semant_error();
}

static ostream& internal_error(int lineno) {
    error_stream << "FATAL:" << lineno << ": ";
    return error_stream;
}

typedef std::vector<Symbol> FuncParameter;
typedef std::map<Symbol, FuncParameter> Map2;
Map2 funcParaMap;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////

static Symbol 
    Int,
    Float,
    String,
    Bool,
    Void,
    Main,
    print
    ;

bool isValidCallName(Symbol type) {
    return type != (Symbol)print;
}

bool isValidTypeName(Symbol type) {
    return type != Void;
}

//
// Initializing the predefined symbols.
//

static void initialize_constants(void) {
    // 4 basic types and Void type
    Bool        = idtable.add_string("Bool");
    Int         = idtable.add_string("Int");
    String      = idtable.add_string("String");
    Float       = idtable.add_string("Float");
    Void        = idtable.add_string("Void");  
    // Main function
    Main        = idtable.add_string("main");

    // classical function to print things, so defined here for call.
    print        = idtable.add_string("printf");
}

/*
    TODO :
    you should fill the following function defines, so that semant() can realize a semantic 
    analysis in a recursive way. 
    Of course, you can add any other functions to help.
*/

static bool sameType(Symbol name1, Symbol name2) {
    return strcmp(name1->get_string(), name2->get_string()) == 0;
}
    //CALL一开始是空的，只有一个空指针。所以先要创建一个开始的scope，才能enterscope
    //遇到新scope要先enterscope
    //遇到call：
        //1、先对比当前scope是否已经重复
        //2、检查是否和‘printf’冲定义
        //3、检查call的类型仅限于`Int` `Void` `String` `Float` 和`Bool`
        //都没问题的话就添加到当前所在的scope里。
    //遇到scope结束标志要exitscope
    
    //

static void install_calls(Decls decls) {//这个函数与scope无关，把所有符合条件的都装进去
    
    for(int i = decls->first(); decls->more(i); i = decls->next(i))//遍历decl的方法，从dumptype中拷贝
    {
        Symbol type = decls->nth(i)->getType();
        Symbol name = decls->nth(i)->getName();

        if(decls->nth(i)->isCallDecl())
        {
            if (type != Int && type != String && type != Void && type != Float && type != Bool) {//返回类型
                semant_error(decls->nth(i)) << "The returnType must be int or string or void or float or bool." << std::endl;
            }

            if(callMap[name]!=NULL){//current scope 有重名
                semant_error(decls->nth(i))<<"Function "<< name <<" has been defined."<<std::endl;
            }
            
            if(!isValidCallName(name)){//非法
                semant_error(decls->nth(i))<<"The function can‘t name ’printf‘."<<std::endl;
            }
            callMap[name]=type;
            // std::cout<<name<<type<<std::endl;
            // decls->nth(i)->checkPara();
        }    
    }
}

static void install_globalVars(Decls decls) {
    for(int i = decls->first(); decls->more(i); i = decls->next(i))//遍历decl的方法，从dumptype中拷贝
    {
        Symbol type = decls->nth(i)->getType();
        Symbol name = decls->nth(i)->getName();

       if (!decls->nth(i)->isCallDecl()) {
           std::cout << "fuck!" << std::endl; // --------------------------------------
           if (type == Void) {
                semant_error(decls->nth(i)) << "The variable must be int or string or void or float or bool." << std::endl;
            } 
            if (name == print) {
                semant_error(decls->nth(i)) << "The variable can‘t name ’printf‘." << std::endl;
            }
            if (gvMap[name] != NULL) {
                semant_error(decls->nth(i)) <<"the global variable "<<name<<" has been defined."<<std::endl;
            }
            gvMap[name] = type;
        }
    }
}

static void check_calls(Decls decls) {
    for (int i=decls->first(); decls->more(i); i=decls->next(i)) {
        if (decls->nth(i)->isCallDecl()) {
            decls->nth(i)->check();
            decls->nth(i)->checkPara();
            // lvMap.clear();
        }
    }
}

static void check_main() {//直接看没有main报错就行
    if(callMap[Main]==nullptr)
        semant_error()<<"ERROR:No Main!"<<std::endl;
}

void VariableDecl_class::check() {//变量不可为Void类型
    Symbol type = this->getType();
    Symbol name = this->getName();

    if(type == Void)
        semant_error()<<"This variable "<<name<<" can't be void."<<std::endl;
    
    lvMap[name]=type;
    objectEnv.addid(name,&type);
}

void CallDecl_class::check() {
    //这里就要考虑到在范围的scope里的变量名了
    //需要考虑的有0、是否有main（已经判断了）1、main参数为空；2、main的返回值是oid
    //检查同一个域里有没有同名声明（不用），形参不能相同,类型不能为void
    //调用函数检查关键字和变量
    Symbol returnType = this->getType();
    StmtBlock body = this->getBody();
    Symbol callName = this->getName();
    Variables paras = this->getVariables();
    objectEnv.enterscope();

    //先看main是不是符合条件
    if (callName == Main) {
        if (paras->len() != 0) 
            semant_error(this) << "Main function can't have parameter(s)." << std::endl;
        if (callMap[Main] != Void) 
            semant_error(this) << "Main function must return Void type." << std::endl;
    } 
    //看参数的问题
    for (int i=paras->first(); paras->more(i); i=paras->next(i))
    {
        Symbol Name = paras->nth(i)->getName();
        Symbol Type = paras->nth(i)->getType();

        if(objectEnv.lookup(Name)!=NULL)
            semant_error(this)<< "This name: "<<Name<< " have been used in same function."<<std::endl;
        if(Type==Void)
            semant_error(this)<<"Type of the parameter: "<<Name<<" can not be Void."<<std::endl;
        objectEnv.addid(Name,new Symbol(Type));
        lvMap[Name]=Type;
    }
    //检查返回类型
    body->check(returnType);
    if (!body->isReturn()) {
        semant_error(this) << "Function " << name << " must have an overall return statement." << std::endl;
    }

    
    

    // body->checkBreakContinue();
    objectEnv.exitscope();
}

// correct
void StmtBlock_class::check(Symbol type) {
    Stmts stmts = this->getStmts(); 
    VariableDecls vars = this->getVariableDecls();

    for (int j=stmts->first(); stmts->more(j); j=stmts->next(j)) {
        stmts->nth(j)->check(type);
    }
    // 关键字和变量检查
    for (int i=vars->first(); vars->more(i); i=vars->next(i)) {
        vars->nth(i)->check();
    }
}

void IfStmt_class::check(Symbol type) {//if的check考虑else和then存在与否的情况，可能的报错是if中的条件语句返回值不是bool
    Expr condition = this->getCondition();
    StmtBlock thenStmtBlock = this->getThen();
    StmtBlock elseStmtBlock = this->getElse();
    Symbol conditionType = condition->checkType();
    if (conditionType != Bool) 
        semant_error(this) << "Condition's type must be Bool"<< std::endl;
    thenStmtBlock->check(type);
    elseStmtBlock->check(type);
}

void WhileStmt_class::check(Symbol type) {//报错与if类似
    Expr condition = this->getCondition();
    StmtBlock body = this->getBody();
    Symbol conditionType = condition->checkType();
    if (conditionType != Bool) 
        semant_error(this) << "Condition's type must be Bool"<< std::endl;
    body->check(type);
    body->checkBreakContinue();
}

void ForStmt_class::check(Symbol type) {//for循环中间条件语句的报错情况与if类似，
    Expr init = this->getInit();
    Expr condition = this->getCondition();
    Expr loop = this->getLoop();
    StmtBlock body = this->getBody();
    init->checkType();
    Symbol conditionType = condition->checkType();
    if (conditionType != Bool) {
        semant_error(this) << "Condition's type must be Bool"<< std::endl;
    }
    loop->checkType();
    body->check(type);  
}
////////////////////////////////////////
void StmtBlock_class::checkBreakContinue() {
    Stmts stmts = this->getStmts();
    for (int i=stmts->first(); stmts->more(i); i=stmts->next(i)) {
        if (stmts->nth(i)->isSafe()) {
            continue;
        } else  {
            stmts->nth(i)->checkBreakContinue();
        } 
    }
}

void IfStmt_class::checkBreakContinue() {
    this->thenexpr->checkBreakContinue();
    this->elseexpr->checkBreakContinue();
}

void BreakStmt_class::checkBreakContinue() {
    semant_error(this) << "break must be used in a loop sentence" << std::endl;
}

void ContinueStmt_class::checkBreakContinue() {
    semant_error(this) << "continue must be used in a loop sentence" << std::endl;
}
void ContinueStmt_class::check(Symbol type) {

}

void BreakStmt_class::check(Symbol type) {

}

void CallDecl_class::checkPara() {
    // Symbol callName = this->getName();
    // Variables paras = this->getVariables();
    // Symbol returnType = this->getType();
    // StmtBlock body = this->getBody();

    // FuncParameter funcParameter;
    // for (int i=paras->first(); paras->more(i); i=paras->next(i)) {
    //     Symbol paraName = paras->nth(i)->getName();
    //     Symbol paraType = paras->nth(i)->getType();
    //     funcParameter.push_back(paraType);
    // }
    // funcParaMap[callName] = funcParameter;
}

void ReturnStmt_class::check(Symbol type) {//return的类型必须和声明类型一致
    Expr value = this->getValue();
    Symbol valueType = value->checkType();
    if (value->is_empty_Expr())
    {
        if (type != Void) 
            semant_error(this) << "You should return "<< type<<"not void."<<std::endl;
    }
    else 
        if (type != valueType) 
            semant_error(this) <<  "You should return "<< type<<"not "<<valueType <<std::endl;
}

Symbol Call_class::checkType(){
    //这个函数要考虑，printf至少有一个参数，输出参数是字符串
    Symbol callName = this->getName();
    Actuals actuals = this->getActuals();
    unsigned int j = 0;
    
    if (callName == print) {
        if (actuals->len() == 0) {
            semant_error(this) << "printf function must has at last one parameter of type String." << endl;
            this->setType(Void);
            return this->type;
        }
        Symbol sym = actuals->nth(actuals->first())->checkType();
        if (sym != String) {
            semant_error(this) << "printf()'s first parameter must be of type String." << endl;
        }
        this->setType(Void);
        return this->type;
    }

    if (actuals->len() > 0) {
        if (actuals->len() != int(funcParaMap[callName].size())) {
            semant_error(this) << "Wrong number of paras" << endl;
        }
        for (int i=actuals->first(); actuals->more(i) && j<funcParaMap[callName].size(); i=actuals->next(i)) {
            Symbol sym = actuals->nth(i)->checkType();
            // check function call's paras fit funcdecl's paras
            if (sym != funcParaMap[callName][j]) {
                semant_error(this) << "Function " << callName << ", type " << sym << " of parameter a does not conform to declared type " << funcParaMap[callName][j] << endl;
            }
            ++j;      
        }
    }
    
    if (callMap[callName] == NULL) {
        semant_error(this) << "Object " << callName << " has not been defined" << endl;
        this->setType(Void);
        return this->type;
    } 
    this->setType(callMap[callName]);
    return this->type;
}

Symbol Actual_class::checkType(){
    Symbol Type = this->expr->checkType();
    this->setType(Type);
    return this->type;
}

Symbol Assign_class::checkType(){

    if (objectEnv.lookup(this->lvalue) == NULL && gvMap[this->lvalue] == NULL) {
        semant_error(this) << "value no defined" << endl;
    } 
    Symbol lvalueType = lvMap[this->lvalue];
    Symbol valueType = this->value->checkType();
    if (lvalueType != valueType) {
        semant_error(this) << "Type should be same!" << std::endl;
    }
    
    
    this->setType(valueType);
    return this->type;
}

Symbol Add_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'===' must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        }
    }
    

    else if ((Type1 == Float && Type2 == Int)||(Type1 == Int && Type2 == Float)) {
        this->setType(Float);
        return type;
    } 

    else {
        semant_error(this) << "'+' 's paras should be same,or paras could exchanged." << std::endl;
        this->setType(Float);
        return type;
    }
    this->setType(Type1);
    return this->type;
}


Symbol Minus_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();

    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float ) {
           semant_error(this) << "'-' must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        }
    } 

    else if ((Type1 == Float && Type2 == Int)||(Type1 == Int && Type2 == Float)) {
        this->setType(Float);
        return type;
    }
    
     else {
        semant_error(this) << "'-' 's paras should be same,or paras could exchanged." << std::endl;
        this->setType(Float);
        return type;
    }
    this->setType(Type1);
    return this->type;
 
}

Symbol Multi_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();

    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'*' must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        }
    } else if ((Type1 == Float && Type2 == Int)||(Type1 == Int && Type2 == Float)) {
        this->setType(Float);
        return type;
    } else {
        semant_error(this) << "'*' 's paras should be same,or paras could exchanged." << std::endl;
        this->setType(Float);
        return type;
    }
    this->setType(Type1);
    return this->type;
 
}

Symbol Divide_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();

    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'/' must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        }
    } else if ((Type1 == Float && Type2 == Int)||(Type1 == Int && Type2 == Float)) {
        this->setType(Float);
        return type;
    } else {
        semant_error(this) << "'/' 's paras should be same,or paras could exchanged." << std::endl;
        this->setType(Float);
        return type;
    }
    this->setType(Type1);
    return this->type;

}

Symbol Mod_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();

    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'%' must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        }
    } else if ((Type1 == Float && Type2 == Int)||(Type1 == Int && Type2 == Float)) {
        this->setType(Float);
        return type;
    } else {
        semant_error(this) << " '%' 's paras should be same,or paras could exchanged." << std::endl;
        this->setType(Float);
        return type;
    }
    this->setType(Type1);
    return this->type;

}

Symbol Neg_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    if (Type1 != Int && Type1 != Float) {
        semant_error(this)<<" '-''s paras should be int or float."<<endl;
    }
    this->setType(Type1);
    return this->type;
}

Symbol Lt_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'<' must be int or float type , not" << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << " '<' 's paras should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;
}

Symbol Le_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'<=' expr value must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << " '<=' 's paras should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Equ_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'=' expr value must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << "'=' 's paras should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;
}

Symbol Neq_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'!=' expr value must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << "'!=' 's paras should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Ge_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'>=' expr value must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << "'>=' 's paras should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Gt_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'>' expr value must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << "'>' 's paras should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol And_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 != Bool || Type2 != Bool) {
        semant_error(this) << " 'And' should have both bool type." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Or_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 != Bool || Type2 != Bool) {
        semant_error(this) << " 'Or' should have both bool type." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Xor_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if ((Type1 != Type2)||!((Type1 == Bool)||(Type1 == Int))) 
    {
        semant_error(this) << " 'And' should have both bool or int type." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Not_class::checkType(){
    Symbol Type1 = e1->checkType();
    if (Type1 != Bool) {
        semant_error(this) << "'!' should have Bool type" << endl;
    }

    this->setType(Bool);
    return this->type;

}

Symbol Bitand_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 != Int || Type2 != Int) {
        semant_error(this) << "Bitand expr should have both Int type." << std::endl;
    }
    this->setType(Int);
    return this->type;

}

Symbol Bitor_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 != Int || Type2 != Int) {
        semant_error(this) << "Bitor expr should have both Int type." << std::endl;
    }
    this->setType(Int);
    return this->type;

}

Symbol Bitnot_class::checkType(){
    Symbol Type1 = e1->checkType();
    if (Type1 != Int) {
        semant_error(this) << "Bitnot expr should have Int type" << endl;
    }

    this->setType(Int);
    return this->type;

}

Symbol Const_int_class::checkType(){
    setType(Int);
    return type;
}

Symbol Const_string_class::checkType(){
    setType(String);
    return type;
}

Symbol Const_float_class::checkType(){
    setType(Float);
    return type;
}

Symbol Const_bool_class::checkType(){
    setType(Bool);
    return type;
}

Symbol Object_class::checkType(){
    if (objectEnv.lookup(this->var) == NULL) {
        semant_error(this) << "Object '"<< this->var <<"' has not been defined." << endl;
        this->setType(Void);
        return this->type;
    }
    Symbol varType = lvMap[this->var];
    this->setType(varType);
    return this->type;

}

Symbol No_expr_class::checkType(){
    setType(Void);
    return getType();
}

void Program_class::semant() {
    initialize_constants();
    install_calls(decls);
    check_main();
    install_globalVars(decls);
    check_calls(decls);
    
    if (semant_errors > 0) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}



