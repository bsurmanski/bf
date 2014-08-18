
#include <cstdio>
#include <cassert>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

using namespace llvm;

enum Token
{
    tok_incp,
    tok_decp,
    tok_inc,
    tok_dec,
    tok_read,
    tok_write,
    tok_beginloop,
    tok_endloop,
};

static Module *module;
static IRBuilder<> builder(getGlobalContext());
AllocaInst *tapePtr;

int gettok(FILE *f)
{
    switch(getc(f))
    {
        case '>': return tok_incp;
        case '<': return tok_decp;
        case '+': return tok_inc;
        case '-': return tok_dec;
        case '.': return tok_write;
        case ',': return tok_read;
        case '[': return tok_beginloop;
        case ']': return tok_endloop;
        default: return -1;
    }
}

void codegen_incp()
{
    Value *tapeVal = builder.CreateLoad(tapePtr);
    Value *addVal = builder.CreateGEP(tapeVal, ConstantInt::get(Type::getInt8Ty(getGlobalContext()), 1));
    builder.CreateStore(addVal, tapePtr);
}

void codegen_decp()
{
    Value *tapeVal = builder.CreateLoad(tapePtr);
    Value *subVal = builder.CreateGEP(tapeVal, ConstantInt::get(Type::getInt8Ty(getGlobalContext()), -1));
    builder.CreateStore(subVal, tapePtr);
}

void codegen_inc()
{
    Value *tapeVal = builder.CreateLoad(tapePtr);
    Value *tapeNum = builder.CreateLoad(tapeVal);
    Value *addVal = builder.CreateAdd(tapeNum, ConstantInt::get(Type::getInt8Ty(getGlobalContext()), 1));
    builder.CreateStore(addVal, tapeVal);
}

void codegen_dec()
{
    Value *tapeVal = builder.CreateLoad(tapePtr);
    Value *tapeNum = builder.CreateLoad(tapeVal);
    Value *subVal = builder.CreateSub(tapeNum, ConstantInt::get(Type::getInt8Ty(getGlobalContext()), 1));
    builder.CreateStore(subVal, tapeVal);
}

void codegen_read()
{
    Value *val = builder.CreateCall(module->getFunction("getchar"));
}

void codegen_write()
{
    Value *tapeVal = builder.CreateLoad(tapePtr);
    Value *tapeNum = builder.CreateLoad(tapeVal);
    Value *extNum = builder.CreateZExt(tapeNum, Type::getInt32Ty(getGlobalContext()));
    builder.CreateCall(module->getFunction("putchar"), extNum);
}

FILE *f;
void codegen(int tok);
void codegen_loop()
{
    //set up loop
    BasicBlock *CurBB = builder.GetInsertBlock();
    BasicBlock *whileCondBB = BasicBlock::Create(getGlobalContext(), "whilecond", CurBB->getParent());
    BasicBlock *whileBB = BasicBlock::Create(getGlobalContext(), "while", CurBB->getParent());
    BasicBlock *afterBB = BasicBlock::Create(getGlobalContext(), "afterwhile", CurBB->getParent());
    builder.CreateBr(whileCondBB);
    builder.SetInsertPoint(whileCondBB);
    Value *tapeVal = builder.CreateLoad(tapePtr);
    Value *tapeNum = builder.CreateLoad(tapeVal);
    Value *bval = builder.CreateICmpNE(tapeNum, ConstantInt::get(Type::getInt8Ty(getGlobalContext()), 0));
    builder.CreateCondBr(bval, whileBB, afterBB);
    builder.SetInsertPoint(whileBB);

    // insert loop meat
    while(!feof(f))
    {
        int tok = gettok(f);
        if(tok < 0) continue;
        if(tok == tok_endloop) break;
        codegen(tok);
    }
    
    //wrap up loop
    builder.CreateBr(whileCondBB);
    builder.SetInsertPoint(afterBB);
}

void codegen(int tok)
{
    assert(tok >= 0);

    switch(tok)
    {
        case tok_incp:
            codegen_incp(); break;
        case tok_decp:
            codegen_decp(); break;
        case tok_inc:
            codegen_inc(); break;
        case tok_dec:
            codegen_dec(); break;
        case tok_read:
            codegen_read(); break;
        case tok_write:
            codegen_write(); break;
        case tok_beginloop:
            codegen_loop(); break;
    }
}

void init(int tapesz)
{
    module = new llvm::Module("BFMod", getGlobalContext()); 

    //declare malloc
    std::vector<Type*> mallocargs;
    mallocargs.push_back(Type::getInt64Ty(getGlobalContext()));
    FunctionType *malloctype = FunctionType::get(Type::getInt8Ty(getGlobalContext())->getPointerTo(), mallocargs, false);
    Function *mallocfunc = Function::Create(malloctype, Function::ExternalLinkage, "malloc", module);

    //declare getchar
    FunctionType *getchartype = FunctionType::get(Type::getInt32Ty(getGlobalContext()), std::vector<Type*>(), false);
    Function *getcharfunc = Function::Create(getchartype, Function::ExternalLinkage, "getchar", module);

    //declare putchar
    std::vector<Type*> putcharargs;
    putcharargs.push_back(Type::getInt32Ty(getGlobalContext()));
    FunctionType *putchartype = FunctionType::get(Type::getInt32Ty(getGlobalContext()), putcharargs, false);
    Function *putcharfunc = Function::Create(putchartype, Function::ExternalLinkage, "putchar", module);

    //create main block
    FunctionType *maintype = FunctionType::get(Type::getVoidTy(getGlobalContext()), std::vector<Type*>(), false);
    Function *mainfunc = Function::Create(maintype, Function::ExternalLinkage, "main", module);
    BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", mainfunc);
    builder.SetInsertPoint(BB);

    //create tape variable
    Value *tapeInitVal = builder.CreateCall(module->getFunction("malloc"), ConstantInt::get(Type::getInt64Ty(getGlobalContext()), tapesz));
    tapePtr = builder.CreateAlloca(Type::getInt8Ty(getGlobalContext())->getPointerTo(), 0, "tape");
    builder.CreateStore(tapeInitVal, tapePtr);
}

int main(int argc, char **argv)
{
    f = fopen("test.bf", "r"); 
    init(1024);

    while(!feof(f))
    {
        int tok = gettok(f);
        if(tok < 0) continue;
        codegen(tok);
    }
    builder.CreateRetVoid();

    module->dump();

    return 0;
}
