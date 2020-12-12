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

// typedef SymbolTable<Symbol, Symbol> ObjectEnvironment; // name, type
typedef SymbolTable<Symbol, CallDecl> CallEnvironment; // call environment
typedef SymbolTable<Symbol, VariableDecl> VarEnvironment; // variable environment

static CallEnvironment callEnv;
static VarEnvironment varEnv;

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

static bool sameName(Symbol name1, Symbol name2) {
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
    // Since call declaractions can only put at the global scope,
    // there should always exists one scope for call declaraction.
    callEnv.enterscope();

    for(int i = decls->first(); decls->more(i); i = decls->next(i))//遍历decl的方法，从dumptype中拷贝
    {
        Symbol type = decls->nth(i)->getType();
        Symbol name = decls->nth(i)->getName();

        if(decls->nth(i)->isCallDecl())
        {
            if (!sameType(type, Int) && !sameType(type, Void)
                && !sameType(type, String) && !sameType(type, Float)
                && !sameType(type, Bool)) {//返回类型
                semant_error(decls->nth(i)) << "The returnType must be int or string or void or float or bool." << std::endl;
                continue;
            }

            if(callEnv.probe(name) != NULL){//current scope 有重名
                semant_error(decls->nth(i))<<"Function "<< name <<" has been defined."<<std::endl;
                continue;
            }
            
            if(!isValidCallName(name)){//非法
                semant_error(decls->nth(i))<<"The function can‘t name ’printf‘."<<std::endl;
                continue;
            }

            callEnv.addid(name, 
                new CallDecl((CallDecl)(decls->nth(i))));
        }    
    }
}

static void install_globalVars(Decls decls) {
    // Since variable declaractions can appear in any stmtBlocks,
    // there may exist some variable declaractions with the same name
    // but defined in different scope
    varEnv.enterscope();

    for(int i = decls->first(); decls->more(i); i = decls->next(i))//遍历decl的方法，从dumptype中拷贝
        if (!decls->nth(i)->isCallDecl())
            decls->nth(i)->check();
}

static void check_calls(Decls decls) {
    for (int i=decls->first(); decls->more(i); i=decls->next(i)) {
        if (decls->nth(i)->isCallDecl()) {
            decls->nth(i)->check();
        }
    }
}

static void check_main() {//直接看没有main报错就行
    CallDecl callDecl;
    Symbol returnType;
    Variables paras;

    if(!callEnv.probe(Main)) {
        semant_error()<<"main function not found"<<std::endl;
    } else {
        callDecl = *callEnv.probe(Main);
        paras = callDecl->getVariables();
        returnType = callDecl->getType();

        if (!sameType(returnType, Void))
            semant_error(callDecl) << "main function shouold have return type Void." << std::endl;
        if (paras->len() != 0)
            semant_error(callDecl) << "main function should have empty parameters." << std::endl;
    }
}

void VariableDecl_class::check() {//变量不可为Void类型
    Symbol type = this->getType();
    Symbol name = this->getName();

    if (sameType(type, Void)) {
        semant_error(this) << "The variable must be int or string or void or float or bool." << std::endl;
    }
    if (name == print) {
        semant_error(this) << "The variable can‘t name ’printf‘." << std::endl;
    }
    if (varEnv.probe(name) != NULL) {
        semant_error(this) <<"the global variable "<< name <<" has been defined."<<std::endl;
    }
    varEnv.addid(name, 
        new VariableDecl(static_cast<VariableDecl>(this)));
}

void CallDecl_class::check() {
    //这里就要考虑到在范围的scope里的变量名了
    //需要考虑的有0、是否有main（已经判断了）1、main参数为空；2、main的返回值是oid
    //检查同一个域里有没有同名声明（不用），形参不能相同,类型不能为void
    //调用函数检查关键字和变量
    Symbol returnType = this->getType();
    StmtBlock body = this->getBody();
    Symbol name = this->getName();

    varEnv.enterscope();
    //看参数的问题
    checkPara();

    //检查返回类型
    body->check(returnType);
    if (!body->isReturn()) {
        semant_error(this) << "Function " << name << " must have an overall return statement." << std::endl;
    }

    body->checkBreakContinue();
    varEnv.exitscope();
}


void StmtBlock_class::check(Symbol type) {
    Stmts stmts = this->getStmts(); 
    VariableDecls vars = this->getVariableDecls();

    // 关键字和变量检查
    for (int i=vars->first(); vars->more(i); i=vars->next(i)) {
        vars->nth(i)->check();
    }
    for (int j=stmts->first(); stmts->more(j); j=stmts->next(j)) {
        stmts->nth(j)->check(type);
    }
}

void IfStmt_class::check(Symbol type) {//if的check考虑else和then存在与否的情况，可能的报错是if中的条件语句返回值不是bool
    Expr condition = this->getCondition();
    StmtBlock thenStmtBlock = this->getThen();
    StmtBlock elseStmtBlock = this->getElse();

    Symbol conditionType = condition->checkType();

    if (!sameType(conditionType, Bool)) 
        semant_error(this) << "Condition's type must be Bool"<< std::endl;
    
    varEnv.enterscope();
    thenStmtBlock->check(type);
    varEnv.exitscope();

    varEnv.enterscope();
    elseStmtBlock->check(type);
    varEnv.exitscope();
}

void WhileStmt_class::check(Symbol type) {//报错与if类似
    Expr condition = this->getCondition();
    StmtBlock body = this->getBody();

    Symbol conditionType = condition->checkType();
    
    if (!sameType(conditionType, Bool)) 
        semant_error(this) << "Condition's type must be Bool"<< std::endl;
    
    varEnv.enterscope();
    body->check(type);
    varEnv.exitscope();
}

void ForStmt_class::check(Symbol type) {//for循环中间条件语句的报错情况与if类似，
    Expr init = this->getInit();
    Expr condition = this->getCondition();
    Expr loop = this->getLoop();
    StmtBlock body = this->getBody();

    init->checkType();
    Symbol conditionType = condition->checkType();
    if (!condition->is_empty_Expr() && !sameType(conditionType, Bool)) {
        semant_error(this) << "Condition's type must be Bool"<< std::endl;
    }
    loop->checkType();

    varEnv.enterscope();
    body->check(type);
    varEnv.exitscope();
}

void StmtBlock_class::checkBreakContinue() {
    Stmts stmts = this->getStmts();

    for (int i=stmts->first(); stmts->more(i); i=stmts->next(i)) {
        stmts->nth(i)->checkBreakContinue();
    }
}

void IfStmt_class::checkBreakContinue() {
    this->getThen()->checkBreakContinue();
    this->getElse()->checkBreakContinue();
}

void BreakStmt_class::checkBreakContinue() {
    semant_error(this) << "break must be used in a loop sentence" << std::endl;
}

void ContinueStmt_class::checkBreakContinue() {
    semant_error(this) << "continue must be used in a loop sentence" << std::endl;
}

void ContinueStmt_class::check(Symbol type) {
    // empty
}

void BreakStmt_class::check(Symbol type) {
    // empty
}

void CallDecl_class::checkPara() {
    Symbol callName = this->getName();
    Variables paras = this->getVariables();
    for (int i=paras->first(); paras->more(i); i=paras->next(i))
    {
        Symbol paraName = paras->nth(i)->getName();
        Symbol paraType = paras->nth(i)->getType();

        if(varEnv.probe(paraName) != NULL)
            semant_error(this)<< "This name: "<< paraName << " have been used in same function."<< std::endl;
        if(sameType(paraType, Void))
            semant_error(this)<<"Type of the parameter: "<< paraName <<" can not be Void."<< std::endl;
        varEnv.addid(paraName, new VariableDecl(variableDecl(paras->nth(i))));
    }
}

void ReturnStmt_class::check(Symbol type) {//return的类型必须和声明类型一致
    Expr value = this->getValue();
    Symbol valueType;

    if (value->is_empty_Expr()) {
        if (!sameType(type, Void)) 
            semant_error(this) << "Returns Void, but need "<< type <<std::endl;
    } else {
        valueType = value->checkType();
        if (!sameType(type, valueType)) 
            semant_error(this) <<  "Returns "<< valueType << ", but need "<< type <<std::endl;
    }
}

Symbol Call_class::checkType(){
    //这个函数要考虑，printf至少有一个参数，输出参数是字符串
    Symbol callName = this->getName();
    Actuals actuals = this->getActuals();
    
    if (sameName(callName, print)) {
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

    // case for non-printf call expr
    if (!callEnv.lookup(callName)) {
        semant_error(this) << "Function " << callName << " has not been defined" << endl;
        
        this->setType(Void);
        return this->type;
    }

    CallDecl callDecl = *callEnv.lookup(callName);
    if (actuals->len() > 0) {
        if (actuals->len() != callDecl->getVariables()->len()) {
            semant_error(this) << "Wrong number of paras" << endl;
        }
        for (int i=actuals->first(), j = 0; 
            actuals->more(i) && j <  callDecl->getVariables()->len(); 
            i=actuals->next(i), j++) {
            
            Symbol sym = actuals->nth(i)->checkType();
            // check function call's paras fit funcdecl's paras
            if (!sameType(sym, callDecl->getVariables()->nth(j)->getType())) {
                semant_error(this) << "Function " << callName << ", type " << sym 
                    << " of parameters " << "does not conform to declared type " << callDecl->getVariables()->nth(j)->getType()->get_string() << endl;
            }  
        }
    }
    
    this->setType(callDecl->getType());
    return this->type;
}

Symbol Actual_class::checkType(){
    Symbol Type = this->expr->checkType();
    this->setType(Type);
    return this->type;
}

Symbol Assign_class::checkType(){

    if (varEnv.lookup(this->lvalue) == NULL) {
        semant_error(this) << "object " << this->lvalue->get_string() << " has not been defined" << endl;
    }

    Symbol lvalueType = (*varEnv.lookup(this->lvalue))->getType();
    Symbol valueType = this->value->checkType();
    if (!sameType(lvalueType, valueType)) {
        semant_error(this) << "Type should be same!" << std::endl;
    }
    
    this->setType(valueType);
    return this->type;
}


Symbol Add_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    
    if (sameType(Type1, Type2)) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'+' 's operands must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        } else {
            this->setType(Type1);
            return this->type;
        }
    } else {
        if ((Type1 == Float && Type2 == Int) || (Type1 == Int && Type2 == Float)) {
            this->setType(Float);
            return type;
        } else {
            semant_error(this) << "'+' 's operands should be same,or paras could exchanged." << std::endl;
            this->setType(Float);
            return type;
        }
    }
}


Symbol Minus_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();


    if (sameType(Type1, Type2)) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'-' 's operands must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        } else {
            this->setType(Type1);
            return this->type;
        }
    } else {
        if ((Type1 == Float && Type2 == Int) || (Type1 == Int && Type2 == Float)) {
            this->setType(Float);
            return type;
        } else {
            semant_error(this) << "'-' 's operands  should be same,or paras could exchanged." << std::endl;
            this->setType(Float);
            return type;
        }
    }
}

Symbol Multi_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();

    
    if (sameType(Type1, Type2)) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'*' 's operands  must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        } else {
            this->setType(Type1);
            return this->type;
        }
    } else {
        if ((Type1 == Float && Type2 == Int) || (Type1 == Int && Type2 == Float)) {
            this->setType(Float);
            return type;
        } else {
            semant_error(this) << "'*' 's operands  should be same,or paras could exchanged." << std::endl;
            this->setType(Float);
            return type;
        }
    }
}

Symbol Divide_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();

    
    if (sameType(Type1, Type2)) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'/' 's operands must be int or float type , not" << Type1 << std::endl;
            this->setType(Float);
            return this->type;
        } else {
            this->setType(Type1);
            return this->type;
        }
    } else {
        if ((Type1 == Float && Type2 == Int) || (Type1 == Int && Type2 == Float)) {
            this->setType(Float);
            return type;
        } else {
            semant_error(this) << "'/' 's operands  should be same,or paras could exchanged." << std::endl;
            this->setType(Float);
            return type;
        }
    }
}

Symbol Mod_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();

    
    if (sameType(Type1, Type2)) {
        if ( Type1 != Int ) {
            semant_error(this) << "'%''s operands  must be int type , not" << Type1 << std::endl;
            this->setType(Int);
            return this->type;
        } else {
            this->setType(Type1);
            return this->type;
        }
    } else {
        semant_error(this) << "'%' 's operands  should be same" << std::endl;
        this->setType(Int);
        return type;
    }

}

Symbol Neg_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    if (Type1 != Int && Type1 != Float) {
        semant_error(this)<<" '-' 's operands  should be int or float."<<endl;
    }
    this->setType(Type1);
    return this->type;
}

Symbol Lt_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float ) {
            semant_error(this) << "'<' 's operands  must be int or float type , not" << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << " '<' 's operands  should be same,or paras could exchanged." << std::endl;
    }

    this->setType(Bool);
    return this->type;
}

Symbol Le_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'<=' 's operands must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << " '<=' 's operands should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Equ_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'==' 's operands  must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << "'==' 's operands  should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;
}

Symbol Neq_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'!=' 's operands must be int and float and bool type and Bool type, not " << Type1 << std::endl;
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
            semant_error(this) << "'>=' 's operands  must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << "'>=' 's operands  should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Gt_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 == Type2) {
        if ( Type1 != Int && Type1 != Float && Type1 != Bool) {
            semant_error(this) << "'>' 's operands  must be int and float and bool type and Bool type, not " << Type1 << std::endl;
        }
    } else if (!(Type1 == Float && Type2 == Int) && !(Type1 == Int && Type2 == Float)) {
        semant_error(this) << "'>' 's operands  should be same,or paras could exchanged." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol And_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 != Bool || Type2 != Bool) {
        semant_error(this) << "'And' 's operands should have both bool type." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Or_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 != Bool || Type2 != Bool) {
        semant_error(this) << "'Or' 's operands should have both bool type." << std::endl;
    }
    this->setType(Bool);
    return this->type;

}

Symbol Xor_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if ((Type1 != Type2) || (Type1 != Int)) 
    {
        semant_error(this) << "'^' 's operands should have int type." << std::endl;
    }
    this->setType(Int);
    return this->type;

}

Symbol Not_class::checkType(){
    Symbol Type1 = e1->checkType();
    if (Type1 != Bool) {
        semant_error(this) << "'!' 's operands should have Bool type" << endl;
    }

    this->setType(Bool);
    return this->type;

}

Symbol Bitand_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 != Int || Type2 != Int) {
        semant_error(this) << "& expr should have both Int type." << std::endl;
    }
    this->setType(Int);
    return this->type;

}

Symbol Bitor_class::checkType(){
    Symbol Type1 = this->e1->checkType();
    Symbol Type2 = this->e2->checkType();
    if (Type1 != Int || Type2 != Int) {
        semant_error(this) << "| expr should have both Int type." << std::endl;
    }
    this->setType(Int);
    return this->type;

}

Symbol Bitnot_class::checkType(){
    Symbol Type1 = e1->checkType();
    if (Type1 != Int) {
        semant_error(this) << "~ expr should have Int type" << endl;
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
    VariableDecl *varDecl = varEnv.lookup(this->var);
    
    if (varDecl == NULL) {
        semant_error(this) << "object "<< this->var->get_string() <<" has not been defined." << endl;
        this->setType(Void);
        return this->type;
    }
    Symbol varType = (*varDecl)->getType();
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



