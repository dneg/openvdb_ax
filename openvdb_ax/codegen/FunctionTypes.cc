///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2019 DNEG
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DNEG nor the names
// of its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

/// @authors Nick Avramoussis
///
/// @brief
///

#include "FunctionTypes.h"

#include "Types.h"
#include "Utils.h"

#include <openvdb_ax/Exceptions.h>

#include <openvdb/util/Name.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_os_ostream.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

void
stackValuesForFunction(std::vector<llvm::Value*>& arguments,
                   std::stack<llvm::Value*>& values,
                   const size_t count,
                   llvm::IRBuilder<>& builder)
{
    // initialize arguments. scalars are always passed by value, arrays
    // always by pointer, strings by a pointer to the first char
    llvm::Type* strType =
        LLVMType<std::string>::get(builder.getContext());

    arguments.resize(count);
    for (auto r = arguments.rbegin(); r != arguments.rend(); ++r) {
        llvm::Value* arg = values.top(); values.pop();
        llvm::Type* type = arg->getType();
        if (type->isPointerTy()) {
            type = type->getPointerElementType();
            if (type->isIntegerTy() || type->isFloatingPointTy()) {
                // pass by value
                arg = builder.CreateLoad(arg);
            }
            else if (type->isArrayTy()) {/*pass by pointer*/}
            else if (type == strType) {
                // get the string pointer
                arg = builder.CreateStructGEP(strType, arg, 0); // char**
                arg = builder.CreateLoad(arg); // char*
            }
        }
        else {
            // arrays should never be loaded
            assert(!type->isArrayTy());
            if (type->isIntegerTy() || type->isFloatingPointTy()) {
                /*pass by value*/
            }
            else if (type == LLVMType<std::string>::get(builder.getContext())) {
                llvm::Constant* zero =
                    llvm::cast<llvm::Constant>(LLVMType<int32_t>::get(builder.getContext(), 0));
                arg = llvm::cast<llvm::Constant>(arg)->getAggregateElement(zero); // char*
            }
        }
        *r = arg;
    }
}

inline void
printType(const llvm::Type* type, llvm::raw_os_ostream& stream, const bool axTypes)
{
    const ast::tokens::CoreType token =
        axTypes ? tokenFromLLVMType(type) : ast::tokens::UNKNOWN;
    if (token == ast::tokens::UNKNOWN) type->print(stream);
    else stream << ast::tokens::typeStringFromToken(token);
}

inline void
printTypes(llvm::raw_os_ostream& stream,
           const std::vector<llvm::Type*>& types,
           const std::string sep = "; ",
           const bool axTypes = false)
{
    if (types.empty()) return;
    auto typeIter = types.cbegin();
    for (; typeIter != types.cend() - 1; ++typeIter) {
        printType(*typeIter, stream, axTypes);
        stream << sep;
    }
    printType(*typeIter, stream, axTypes);
}

void
printSignature(const llvm::Type* returnType,
               const std::vector<llvm::Type*>& signature,
               const std::string& functionName,
               std::ostream& os,
               const bool axTypes = false)
{
    llvm::raw_os_ostream stream(os);

    if (returnType) printType(returnType, stream, axTypes);
    if (!functionName.empty()) {
        stream << " " << functionName;
    }
    stream << '(';
    printTypes(stream, signature, "; ", axTypes);
    stream << ')';
}

/// @brief  Returns true if an explicit match exists between two llvm type vectors
///
/// @param input      The first vector of llvm types
/// @param signature  The second vector of llvm types
///
bool explicitMatchTypes(const std::vector<llvm::Type*>& input,
                        const std::vector<llvm::Type*>& signature)
{
    assert(input.size() == signature.size());
    return signature == input;
}

/// @brief  Returns true if an implicit match exists between two llvm type vectors
///
/// @param input      The first vector of llvm types
/// @param signature  The second vector of llvm types
///
bool implicitMatchTypes(const std::vector<llvm::Type*>& input,
                        const std::vector<llvm::Type*>& signature)
{
    assert(input.size() == signature.size());

    for (size_t i = 0; i < signature.size(); ++i) {
        llvm::Type* type1 = signature[i];
        llvm::Type* type2 = input[i];

        if (type1 == type2) {
            continue;
        }
        else if (isScalarType(type1) && isScalarType(type2)) {
            continue;
        }
        else if (type1->isPointerTy() && type2->isPointerTy()) {
            // check contained ptr type
            llvm::Type* contained1 = type1->getContainedType(0);
            llvm::Type* contained2 = type2->getContainedType(0);
            if (contained1 == contained2) continue;

            // if the pointers are different, see if they are arrays and check
            // types and size - only one layer of array casting indirection is supported
            // as arrays are expected to be in a pointer when passed in as arguments
            if (!contained1->isArrayTy()) return false;
            if (!contained2->isArrayTy()) return false;
            if (contained1->getArrayNumElements() != contained2->getArrayNumElements()) {
                return false;
            }
        }
        else {
            return false;
        }
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


FunctionSignatureBase::FunctionSignatureBase(VoidFPtr function,
                      const std::string& symbol,
                      const bool lastArgIsReturn)
    : mFunction(function)
    , mSymbolName(symbol)
    , mLastArgumentReturn(lastArgIsReturn) {}

FunctionSignatureBase::SignatureMatch
FunctionSignatureBase::match(const std::vector<llvm::Type*>& types) const
{
    if (!this->sizeMatch(types.size())) return SignatureMatch::None;
    if (types.empty()) return SignatureMatch::Explicit;

    llvm::LLVMContext& C = types.front()->getContext();

    std::vector<llvm::Type*> current;
    this->toLLVMTypes(C, &current);

    if (explicitMatchTypes(types, current)) {
        return SignatureMatch::Explicit;
    }

    if (implicitMatchTypes(types, current)) {
        return SignatureMatch::Implicit;
    }
    return SignatureMatch::Size;
}

bool
FunctionSignatureBase::sizeMatch(const size_t size) const
{
    return (size == this->size());
}

bool
FunctionSignatureBase::explicitMatch(const std::vector<llvm::Type*>& input) const
{
    if (!this->sizeMatch(input.size())) return false;
    if (input.empty()) return true;

    llvm::LLVMContext& C = input.front()->getContext();

    std::vector<llvm::Type*> current;
    this->toLLVMTypes(C, &current);
    return explicitMatchTypes(input, current);
}

bool
FunctionSignatureBase::implicitMatch(const std::vector<llvm::Type*>& input) const
{
    if (!this->sizeMatch(input.size())) return false;
    if (input.empty()) return true;

    llvm::LLVMContext& C = input.front()->getContext();

    std::vector<llvm::Type*> current;
    this->toLLVMTypes(C, &current);
    return implicitMatchTypes(input, current);
}

bool
FunctionSignatureBase::hasReturnValue(llvm::LLVMContext& C) const
{
    return this->toLLVMTypes(C) != LLVMType<void>::get(C);
}

llvm::Value*
FunctionSignatureBase::getOutputArgument(llvm::IRBuilder<>& builder) const
{
    llvm::Type* type = this->getOutputType(builder.getContext());
    if (!type) return nullptr;
    // Outputs must be pointers - guaranteed by the FunctionSignature classes
    assert(type->isPointerTy());
    type = type->getContainedType(0);
    return builder.CreateAlloca(type);
}

llvm::Type*
FunctionSignatureBase::getOutputType(llvm::LLVMContext& C) const
{
    if (!mLastArgumentReturn) return nullptr;
    std::vector<llvm::Type*> current;
    this->toLLVMTypes(C, &current);
    return current.back();
}

llvm::Function*
FunctionSignatureBase::toLLVMFunction(llvm::Module& M) const
{
    std::vector<llvm::Type*> current;
    llvm::Type* returnT = this->toLLVMTypes(M.getContext(), &current);

    llvm::FunctionType* functionType =
        llvm::FunctionType::get(/*Result=*/returnT,
            /*Params=*/llvm::ArrayRef<llvm::Type*>(current),
            /*isVarArg=*/false);

    const std::string& name = this->symbolName();
    llvm::Function* llvmFunction = M.getFunction(llvm::StringRef(name));

    if (!llvmFunction) {

        // Create the function with external linkage
        // @todo - expose the linkage type as an option on the signature

        llvmFunction =
            llvm::Function::Create(functionType,
                llvm::Function::ExternalLinkage, name, &M);
    }
    else if (functionType != llvmFunction->getFunctionType()) {
        OPENVDB_THROW(LLVMFunctionError, "Unable to create a LLVM Function with "
            "symbol \"" + name + "\" as it already exists with a different signature.");
    }

    return llvmFunction;
}

void
FunctionSignatureBase::print(llvm::LLVMContext& C,
    const std::string& name,
    std::ostream& os,
    const bool axTypes) const
{
    std::vector<llvm::Type*> current;
    llvm::Type* returnT = this->toLLVMTypes(C, &current);
    printSignature(returnT, current, name, os, axTypes);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


void FunctionBase::getDependencies(std::vector<std::string>&) const {}

void FunctionBase::getDocumentation(std::string&) const {}

FunctionBase::FunctionMatch
FunctionBase::match(const std::vector<llvm::Type*>& types,
      llvm::LLVMContext& C,
      const bool addOutputArgument) const
{
    FunctionSignatureBase::SignatureMatch match =
        FunctionSignatureBase::SignatureMatch::None;
    FunctionSignatureBase::Ptr targetFunction;

    // Match against the best function signature. If addOutputArgument is true
    // loop through the function list twice - the first time prioritise functions
    // with output arguments and return if an explicit or implicit function call
    // is found. Otherwise (and if addOutputArgument is false) find the best
    // match given the input types

    if (addOutputArgument) {
        for (const auto& function : mFunctionList) {

            if (!function->hasOutputArgument()) continue;

            std::vector<llvm::Type*> typesWithReturn(types);
            typesWithReturn.emplace_back(function->getOutputType(C));

            const FunctionSignatureBase::SignatureMatch matchType =
                function->match(typesWithReturn);

            // for any explicit match, immediately return, otherwise choose the highest
            // match type. Note that the first implicit match is used if the highest
            // match type is implicit

            if (matchType == FunctionSignatureBase::SignatureMatch::Explicit) {
                return FunctionBase::FunctionMatch(function, matchType);
            }
            else if (matchType > match) {
                targetFunction = function;
                match = matchType;
            }
        }

        // return if implicit match (explicit will have already returned)

        if (match == FunctionSignatureBase::SignatureMatch::Implicit) {
            return FunctionBase::FunctionMatch(targetFunction, match);
        }
    }

    for (const auto& function : mFunctionList) {

        // get the match type

        const FunctionSignatureBase::SignatureMatch matchType = function->match(types);

        // for any explicit match, immediately return, otherwise choose the highest
        // match type. Note that the first implicit match is used if the highest
        // match type is implicit

        if (matchType == FunctionSignatureBase::SignatureMatch::Explicit) {
            return FunctionBase::FunctionMatch(function, matchType);
        }
        else if (matchType > match) {
            targetFunction = function;
            match = matchType;
        }
    }

    return FunctionBase::FunctionMatch(targetFunction, match);
}

llvm::Value*
FunctionBase::execute(const std::vector<llvm::Value*>& args,
            const std::unordered_map<std::string, llvm::Value*>& globals,
            llvm::IRBuilder<>& B,
            std::vector<llvm::Value*>* results,
            const bool addOutputArgument) const
{
    std::vector<llvm::Type*> types;
    valuesToTypes(args, types);

    llvm::LLVMContext& C = B.getContext();

    const FunctionBase::FunctionMatch functionMatch =
        match(types, C, addOutputArgument);

    const FunctionSignatureBase::Ptr targetFunction = functionMatch.first;
    const FunctionSignatureBase::SignatureMatch& match = functionMatch.second;

    std::vector<llvm::Value*> input(args);

    if (match == FunctionSignatureBase::SignatureMatch::Implicit) {

        std::vector<llvm::Type*> targetTypes;
        targetFunction->toLLVMTypes(C, &targetTypes);

        for (size_t i = 0; i < input.size(); ++i) {
            if (isScalarType(input[i]->getType())) {
                input[i] = arithmeticConversion(input[i], targetTypes[i], B);
            }
            else if (isArrayType(input[i]->getType()->getContainedType(0))) {
                llvm::Type* arrayType = getBaseContainedType(targetTypes[i]);
                input[i] = arrayCast(input[i], arrayType->getArrayElementType(), B);
            }
        }
    }

    if (match == FunctionSignatureBase::SignatureMatch::Implicit ||
        match == FunctionSignatureBase::SignatureMatch::Explicit) {
        assert(targetFunction);

        if (addOutputArgument && targetFunction->hasOutputArgument()) {
            input.emplace_back(targetFunction->getOutputArgument(B));
        }

        llvm::Value* result = nullptr;
        if (targetFunction->functionPointer()) {
            llvm::Module* M = B.GetInsertBlock()->getParent()->getParent();
            result = B.CreateCall(targetFunction->toLLVMFunction(*M), input);
        }
        else {
            result = this->generate(input, globals, B);
        }

        // only error if result is a nullptr if the return it not void
        llvm::Type* returnType = targetFunction->toLLVMTypes(C);
        if (!returnType->isVoidTy() && !result) {
            OPENVDB_THROW(LLVMFunctionError, "Function \"" + this->identifier() +
                "\" has been invoked with no valid llvm Value return.");
        }

        if (results) {
            for (size_t i = args.size(); i < input.size(); ++i) {
                results->emplace_back(input[i]);
            }
        }

        // @todo  To implicit cast wrong return types?
        if (result && result->getType() != returnType) {
            std::string type, expected;
            llvmTypeToString(result->getType(), type);
            llvmTypeToString(returnType, expected);

            OPENVDB_THROW(LLVMFunctionError, "Function \"" + this->identifier() +
                "\" has been invoked with a mismatching return type. Expected: \"" +
                expected + "\", got \"" + type + "\".");
        }

        return result;
    }

    std::ostringstream os;
    if (match == FunctionSignatureBase::SignatureMatch::None) {
        os << "Wrong number of arguments. \"" << this->identifier() << "\""
           << " was called with: (";
        llvm::raw_os_ostream stream(os);
        printTypes(stream, types);
        stream << ")";
    }
    else {
        // match == FunctionSignatureBase::SignatureMatch::Size
        os << "No matching function for ";
        printSignature(LLVMType<void>::get(C), types, this->identifier(), os, true);
        os << ".";
    }

    os << " Candidates are: ";
    for (const auto& function : mFunctionList) {
        os << std::endl;
        function->print(C, this->identifier(), os);
        os << ", ";
    }

    OPENVDB_THROW(LLVMFunctionError, os.str());
}

llvm::Value*
FunctionBase::generate(const std::vector<llvm::Value*>&,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>&) const {
    return nullptr;
}

}
}
}
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
