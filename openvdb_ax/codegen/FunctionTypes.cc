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
argumentsFromStack(std::stack<llvm::Value*>& values,
                   const size_t count,
                   std::vector<llvm::Value*>& arguments)
{
    arguments.resize(count);
    for (auto r = arguments.rbegin(); r != arguments.rend(); ++r) {
        *r = values.top();
        values.pop();
    }
}

bool
parseDefaultArgumentState(std::vector<llvm::Value*>& arguments,
                          llvm::IRBuilder<>& builder,
                          const bool loadScalarArguments)
{
    bool changed = false;
    for (auto& arg : arguments) {
        llvm::Type* type = arg->getType();
        if (!type->isPointerTy()) continue;
        type = type->getContainedType(0);
        if (loadScalarArguments && isScalarType(type) &&
            !isCharType(type, builder.getContext())) {
            arg = builder.CreateLoad(arg);
            changed |= true;
        }
    }

    return changed;
}

inline void
printTypes(llvm::raw_os_ostream& stream,
           const std::vector<llvm::Type*>& types,
           const std::string sep = "; ")
{
    if (types.empty()) return;
    auto typeIter = types.cbegin();
    for (; typeIter != types.cend() - 1; ++typeIter) {
        (*typeIter)->print(stream);
        stream << sep;
    }
    (*typeIter)->print(stream);
}

void
printSignature(const llvm::Type* returnType,
               const std::vector<llvm::Type*>& signature,
               const std::string& functionName,
               std::ostream& os)
{
    llvm::raw_os_ostream stream(os);

    if (returnType) returnType->print(stream);
    stream << " " << functionName << "(";
    printTypes(stream, signature);
    stream << ")";
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

FunctionSignatureBase::FunctionSignatureBase(void* const function,
                      const std::string& symbol,
                      const size_t outArgs)
    : mFunction(function)
    , mSymbolName(symbol)
    , mOutputArguments(outArgs) {}

FunctionSignatureBase::SignatureMatch
FunctionSignatureBase::match(const std::vector<llvm::Type*>& types,
                             llvm::LLVMContext& C) const
{
    if (!this->sizeMatch(types.size())) return SignatureMatch::None;

    std::vector<llvm::Type*> current;
    this->toLLVMTypes(C, &current);

    if (this->explicitMatch(types, current)) {
        return SignatureMatch::Explicit;
    }

    if (this->implicitMatch(types, current)) {
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
FunctionSignatureBase::explicitMatch(const std::vector<llvm::Type*>& input, llvm::LLVMContext& C) const
{
    if (!this->sizeMatch(input.size())) return false;

    std::vector<llvm::Type*> current;
    this->toLLVMTypes(C, &current);
    return explicitMatch(input, current);
}

bool
FunctionSignatureBase::implicitMatch(std::vector<llvm::Type*>& input, llvm::LLVMContext& C) const
{
    if (!this->sizeMatch(input.size())) return false;

    std::vector<llvm::Type*> current;
    this->toLLVMTypes(C, &current);
    return implicitMatch(input, current);
}

bool
FunctionSignatureBase::hasReturnValue(llvm::LLVMContext& C) const
{
    return this->toLLVMTypes(C) != LLVMType<void>::get(C);
}

void
FunctionSignatureBase::appendOutputArguments(std::vector<llvm::Value*>& values,
                  llvm::IRBuilder<>& builder) const
{
    if (mOutputArguments == 0) return;

    std::vector<llvm::Type*> current;
    this->toLLVMTypes(builder.getContext(), &current);

    const size_t inputSize = current.size() - mOutputArguments;
    for (size_t i = inputSize; i < current.size(); ++i) {

        llvm::Type* type = current[i];

        // @todo - move this check to the constructor or relax this restriction

        if (!type->isPointerTy()) {
            OPENVDB_THROW(LLVMFunctionError, "Function object with symbol name \"" +
                this->symbolName() + "\" has an output argument which is a non pointer "
                "type. Argument " + std::to_string(i) + " must be a pointer type.");
        }

        type = type->getContainedType(0);
        values.emplace_back(builder.CreateAlloca(type));
    }
}

void
FunctionSignatureBase::appendOutputTypes(std::vector<llvm::Type*>& types,
                  llvm::LLVMContext& C) const
{
    if (mOutputArguments == 0) return;

    std::vector<llvm::Type*> current;
    this->toLLVMTypes(C, &current);

    const size_t inputSize = current.size() - mOutputArguments;
    for (size_t i = inputSize; i < current.size(); ++i) {
        types.emplace_back(current[i]);
    }
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
FunctionSignatureBase::print(llvm::LLVMContext& C, const std::string& name, std::ostream& os) const
{
    llvm::raw_os_ostream stream(os);

    std::vector<llvm::Type*> current;
    llvm::Type* returnT = this->toLLVMTypes(C, &current);

    returnT->print(stream);

    if (!name.empty()) stream << " ";
    stream << name << "(";

    if (!current.empty()) {
        auto typeIter = current.cbegin();
        for (; typeIter != current.cend() - 1; ++typeIter) {
            (*typeIter)->print(stream);
            stream << "; ";
        }
        (*typeIter)->print(stream);
    }

    stream << ")";
}

bool
FunctionSignatureBase::explicitMatch(const std::vector<llvm::Type*>& input,
                                     const std::vector<llvm::Type*>& signature) const
{
    assert(input.size() == signature.size());
    return signature == input;
}

bool
FunctionSignatureBase::implicitMatch(const std::vector<llvm::Type*>& input,
                                     const std::vector<llvm::Type*>& signature) const
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

            // if the pointers are different, see if they are arrays and  check
            // types and size - only one layer of array casting indirection is supported
            // as arrays are expected to be in a pointer when passed in as arguments

            llvm::ArrayType* array1 = llvm::dyn_cast<llvm::ArrayType>(contained1);
            if (!array1) return false;

            llvm::ArrayType* array2 = llvm::dyn_cast<llvm::ArrayType>(contained2);
            if (array2 && array1->getNumElements() == array2->getNumElements()) {
                continue;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    return true;
}

FunctionBase::FunctionMatch
FunctionBase::match(const std::vector<llvm::Type*>& types,
      llvm::LLVMContext& C,
      const bool addOutputArguments) const
{
    FunctionSignatureBase::SignatureMatch match =
        FunctionSignatureBase::SignatureMatch::None;
    FunctionSignatureBase::Ptr targetFunction;

    // Match against the best function signature. If addOutputArguments is true
    // loop through the function list twice - the first time prioritise functions
    // with output arguments and return if an explicit or implicit function call
    // is found. Otherwise (and if addOutputArguments is false) find the best
    // match given the input types

    if (addOutputArguments) {
        for (const auto& function : mFunctionList) {

            if (!function->hasOutputArguments()) continue;

            std::vector<llvm::Type*> typesWithReturn(types);
            function->appendOutputTypes(typesWithReturn, C);

            const FunctionSignatureBase::SignatureMatch matchType =
                function->match(typesWithReturn, C);

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

        const FunctionSignatureBase::SignatureMatch matchType = function->match(types, C);

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
            llvm::IRBuilder<>& builder,
            llvm::Module& M,
            std::vector<llvm::Value*>* results,
            const bool addOutputArguments) const
{
    std::vector<llvm::Type*> types;
    valuesToTypes(args, types);

    llvm::LLVMContext& C = M.getContext();

    const FunctionBase::FunctionMatch functionMatch =
        match(types, C, addOutputArguments);

    const FunctionSignatureBase::Ptr targetFunction = functionMatch.first;
    const FunctionSignatureBase::SignatureMatch& match = functionMatch.second;

    std::vector<llvm::Value*> input(args);

    if (match == FunctionSignatureBase::SignatureMatch::Implicit) {

        std::vector<llvm::Type*> targetTypes;
        targetFunction->toLLVMTypes(C, &targetTypes);

        for (size_t i = 0; i < input.size(); ++i) {
            if (isScalarType(input[i]->getType())) {
                input[i] = arithmeticConversion(input[i], targetTypes[i], builder);
            }
            else if (isArrayType(input[i]->getType()->getContainedType(0))) {
                llvm::Type* arrayType = getBaseContainedType(targetTypes[i]);
                input[i] = arrayCast(input[i], arrayType->getArrayElementType(), builder);
            }
        }
    }

    if (match == FunctionSignatureBase::SignatureMatch::Implicit ||
        match == FunctionSignatureBase::SignatureMatch::Explicit) {
        assert(targetFunction);

        if (addOutputArguments) {
            targetFunction->appendOutputArguments(input, builder);
        }

        llvm::Value* result = nullptr;
        if (targetFunction->functionPointer()) {
            result = builder.CreateCall(targetFunction->toLLVMFunction(M), input);
        }
        else {
            result = this->generate(input, globals, builder, M);
        }

        if (!result) {
            OPENVDB_THROW(LLVMFunctionError, "Function \"" + this->identifier() +
                "\" has been invoked with no valid llvm Value return.");
        }

        if (results) {
            for (size_t i = args.size(); i < input.size(); ++i) {
                results->emplace_back(input[i]);
            }
        }

        // @todo  To implicit cast wrong return types?
        if (result->getType() != targetFunction->toLLVMTypes(C)) {
            std::string type, expected;
            llvmTypeToString(result->getType(), type);
            llvmTypeToString(targetFunction->toLLVMTypes(C), expected);

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
        printSignature(LLVMType<void>::get(C), types, this->identifier(), os);
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

}
}
}
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
