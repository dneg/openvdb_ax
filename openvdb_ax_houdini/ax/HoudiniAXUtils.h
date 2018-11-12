///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2018 DNEG Visual Effects
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DNEG Visual Effects nor the names of
// its contributors may be used to endorse or promote products derived
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

///
/// @authors Nick Avramoussis
///
/// @brief  Utility methods for OpenVDB AX in Houdini,
///         contains VEX and channel expression conversion methods.
///
///

#ifndef OPENVDB_AX_HOUDINI_AX_UTILS_HAS_BEEN_INCLUDED
#define OPENVDB_AX_HOUDINI_AX_UTILS_HAS_BEEN_INCLUDED

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/ast/Scanners.h>
#include <openvdb_ax/codegen/FunctionTypes.h>
#include <openvdb_ax/codegen/FunctionRegistry.h>
#include <openvdb_ax/codegen/Utils.h>
#include <openvdb_ax/compiler/Compiler.h>
#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/compiler/CompilerOptions.h>

#include <openvdb/Types.h>
#include <openvdb/Metadata.h>
#include <openvdb/Exceptions.h>
#include <openvdb/openvdb.h>

#include <map>
#include <set>
#include <utility>
#include <string>

namespace openvdb_ax_houdini
{

enum class TargetType
{
    POINTS,
    VOLUMES,
    LOCAL
};

using ChannelExpressionPair = std::pair<std::string, std::string>;
using ChannelExpressionSet = std::set<ChannelExpressionPair>;

/// @brief  Find any Houdini channel expressions represented inside the
///         provided Syntax Tree.
///
/// @param  tree              The AST to parse
/// @param  exprSet           The expression set to populate
inline void findChannelExpressions(const openvdb::ax::ast::Tree& tree,
                            ChannelExpressionSet& exprSet);

/// @brief  Find any Houdini $ expressions represented inside the
///         provided Syntax Tree.
///
/// @param  tree              The AST to parse
/// @param  exprSet           The expression set to populate
inline void findDollarExpressions(const openvdb::ax::ast::Tree& tree,
                            ChannelExpressionSet& exprSet);

/// @brief  Converts a Syntax Tree which contains possible representations of
///         Houdini VEX instructions to internally supported instructions
///
/// @param  tree        The AST to convert
/// @param  targetType  The type of primitive being compiled (this can potentially
///                     alter the generated instructions)
inline void convertASTFromVEX(openvdb::ax::ast::Tree& tree,
                       const TargetType targetType);

/// @brief  Converts lookup functions within a Syntax Tree to ExternalVariable nodes
///         if the argument is a string literal. This method is much faster than using
///         the lookup functions but uses $ AX syntax which isn't typical of a Houdini
///         session.
///
/// @param  tree        The AST to convert
/// @param  targetType  The type of primitive being compiled (this can potentially
///                     alter the generated instructions)
inline void convertASTKnownLookups(openvdb::ax::ast::Tree& tree);

/// @brief  Register custom Houdini functions, making them available to the
///         core compiler. These functions generally have specific Houdini only
///         functionality.
/// @param  reg   The function registry to add register the new functions into
/// @param  options The function options
inline void registerCustomHoudiniFunctions(openvdb::ax::codegen::FunctionRegistry& reg,
                                        const openvdb::ax::FunctionOptions& options);


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

/// @brief AST scanner to find channel expressions and them to the expression set
struct FindChannelExpressions : public openvdb::ax::ast::Visitor
{
    FindChannelExpressions(ChannelExpressionSet& expressions)
        : mExpressions(expressions) {}
    virtual ~FindChannelExpressions() = default;

    static const std::string*
    getChannelPath(const openvdb::ax::ast::ExpressionList::Ptr args)
    {
        assert(args);
        const std::vector<openvdb::ax::ast::Expression::Ptr>& inputs = args->mList;
        if (inputs.empty()) return nullptr;

        const openvdb::ax::ast::Expression* const firstInput = inputs.front().get();
        const openvdb::ax::ast::Value<std::string>* const path =
            dynamic_cast<const openvdb::ax::ast::Value<std::string>* const>(firstInput);
        if (!path) return nullptr;
        return &(path->mValue);
    }

    /// @brief  Add channel expression function calls
    /// @param  node The FunctionCall AST node being visited
    void visit(const openvdb::ax::ast::FunctionCall& node) override final
    {
        std::string type;

        if (node.mFunction == "lookupf" ||
            node.mFunction == "ch") {
            type = openvdb::typeNameAsString<float>();
        }
        else if (node.mFunction == "lookupvec3f" ||
                 node.mFunction == "chv") {
            type = openvdb::typeNameAsString<openvdb::Vec3f>();
        }
        else if (node.mFunction == "chramp") {
            type = "ramp";
        }

        if (type.empty()) return;

        // Get channel arguments. If there are incorrect arguments, defer to
        // the compiler code generation function error system to report proper
        // errors later

        const openvdb::ax::ast::ExpressionList::Ptr args = node.mArguments;
        const std::string* path = getChannelPath(args);
        if (!path) return;

        mExpressions.emplace(type, *path);
    }

private:
    ChannelExpressionSet& mExpressions;
};

inline void findChannelExpressions(const openvdb::ax::ast::Tree& tree,
                                   ChannelExpressionSet& exprSet)
{
    FindChannelExpressions op(exprSet);
    tree.accept(op);
}


///////////////////////////////////////////////////////////////////////////


inline void findDollarExpressions(const openvdb::ax::ast::Tree& tree,
                                  ChannelExpressionSet& exprSet)
{
    openvdb::ax::ast::visitNodeType<openvdb::ax::ast::ExternalVariable>(tree,
        [&](const openvdb::ax::ast::ExternalVariable& node) {
            exprSet.emplace(node.mType, node.mName);
        });
}


///////////////////////////////////////////////////////////////////////////

/// @brief AST modifier to convert VEX-like syntax from Houdini to AX.
///        Finds scalar and vector channel expressions and replace with AX custom
///        data lookups. Replaces volume intrinsics @P, @ix, @iy, @iz with AX function
///        calls. In the future this may be used to translate VEX syntax to an AX AST
///        and back to text for in application conversion to AX syntax.
struct ConvertFromVEX : public openvdb::ax::ast::Modifier
{
    ConvertFromVEX(const TargetType targetType)
        : mTargetType(targetType) {}
    virtual ~ConvertFromVEX() = default;

    /// @brief  Convert channel function calls to internally supported functions
    /// @param  node  The FunctionCall AST node being visited
    openvdb::ax::ast::Expression*
    visit(openvdb::ax::ast::FunctionCall& node) override final
    {
        openvdb::ax::ast::FunctionCall::UniquePtr replacement;

        std::string identifier;
        if (node.mFunction == "ch")       identifier = "lookupf";
        else if (node.mFunction == "chv") identifier = "lookupvec3f";

        if (!identifier.empty()) {
            // Copy the arguments
            openvdb::ax::ast::ExpressionList::UniquePtr args(node.mArguments->copy());
            replacement.reset(new openvdb::ax::ast::FunctionCall(identifier, args.release()));
        }

        return replacement.release();
    }

    /// @brief  Convert Houdini instrinsic volume attribute read accesses
    /// @param  node  The AttributeValue AST node being visited
    openvdb::ax::ast::Expression*
    visit(openvdb::ax::ast::AttributeValue& node) override final
    {
        openvdb::ax::ast::FunctionCall::UniquePtr replacement;

        if (mTargetType == TargetType::VOLUMES) {
            if (node.mAttribute->mName == "P") {
                replacement.reset(new openvdb::ax::ast::FunctionCall("getvoxelpws"));
            }
            else if (node.mAttribute->mName == "ix") {
                replacement.reset(new openvdb::ax::ast::FunctionCall("getcoordx"));
            }
            else if (node.mAttribute->mName == "iy") {
                replacement.reset(new openvdb::ax::ast::FunctionCall("getcoordy"));
            }
            else if (node.mAttribute->mName == "iz") {
                replacement.reset(new openvdb::ax::ast::FunctionCall("getcoordz"));
            }
        }

        return replacement.release();
    }

    /// @brief  Check Houdini instrinsic volume attribute write accesses
    /// @param  node The Attribute AST node being visited
    openvdb::ax::ast::Variable*
    visit(openvdb::ax::ast::Attribute& node) override final
    {
        if (mTargetType == TargetType::VOLUMES)
        {
            if (node.mName == "P"  || node.mName == "ix" ||
                node.mName == "iy" || node.mName == "iz") {
                throw std::runtime_error("Unable to write to a volume name \"@" +
                    node.mName + "\". This is a keyword identifier");
            }
        }

        return nullptr;
    }

private:
    const TargetType mTargetType;
};

inline void convertASTFromVEX(openvdb::ax::ast::Tree& tree,
                       const TargetType targetType)
{
    ConvertFromVEX converter(targetType);
    tree.accept(converter);
}


///////////////////////////////////////////////////////////////////////////

/// @brief  Convert any lookup or channel functions to ExternalVariable nodes
///         if the path is a string literal
struct ConvertKnownLookups : public openvdb::ax::ast::Modifier
{
    ConvertKnownLookups() = default;
    virtual ~ConvertKnownLookups() = default;

    /// @brief  Performs the function call to external variable conversion
    /// @param  node  The FunctionCall AST node being visited
    openvdb::ax::ast::Expression*
    visit(openvdb::ax::ast::FunctionCall& node) override final
    {
        const bool isFloatLookup(node.mFunction == "lookupf");
        const bool isStringLookup(false /*node.mFunction == "lookups"*/);
        const bool isVec3fLookup(node.mFunction == "lookupvec3f");

        if (!isFloatLookup && !isStringLookup && !isVec3fLookup) return nullptr;

        openvdb::ax::ast::ExternalVariable::UniquePtr replacement;
        const std::string* path =
            FindChannelExpressions::getChannelPath(node.mArguments);

        // If for any reason we couldn't validate or get the channel path from the
        // first argument, fall back to the internal lookup functions. These will
        // error with the expected function argument style results on invalid arguments
        // and, correctly support string attribute arguments provided by s@attribute
        // (although are much slower)

        if (path) {

            std::string type;
            if (isFloatLookup) type = openvdb::typeNameAsString<float>();
            else if (isStringLookup) type = openvdb::typeNameAsString<std::string>();
            else type = openvdb::typeNameAsString<openvdb::Vec3f>();

            replacement.reset(new openvdb::ax::ast::ExternalVariable(*path, type));
        }

        return replacement.release();
    }
};

inline void convertASTKnownLookups(openvdb::ax::ast::Tree& tree)
{
    ConvertKnownLookups converter;
    tree.accept(converter);
}


///////////////////////////////////////////////////////////////////////////

/// @brief  Custom derived metadata for ramp channel expressions to be used
///         with codegen::ax::CustomData
template <typename ValueType>
struct RampDataCache : public openvdb::Metadata
{
public:
    using RampType = std::map<float, ValueType>;
    using Ptr = openvdb::SharedPtr<RampDataCache<ValueType>>;
    using ConstPtr = openvdb::SharedPtr<const RampDataCache<ValueType>>;

    RampDataCache() : mData() {}
    virtual ~RampDataCache() {}
    virtual openvdb::Name typeName() const { return str(); }
    virtual openvdb::Metadata::Ptr copy() const {
        openvdb::Metadata::Ptr metadata(new RampDataCache<ValueType>());
        metadata->copy(*this);
        return metadata;
    }
    virtual void copy(const openvdb::Metadata& other) {
        const RampDataCache* t = dynamic_cast<const RampDataCache*>(&other);
        if (t == nullptr) OPENVDB_THROW(openvdb::TypeError, "Incompatible type during copy");
        mData = t->mData;
    }
    virtual std::string str() const { return "<compiler ramp data>"; }
    virtual bool asBool() const { return true; }
    virtual openvdb::Index32 size() const { return mData.size(); }

    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////

    inline void insertRampPoint(const float pos, const ValueType& value)
    {
        mData[pos] = value;
    }

    inline void clearRampPoints() { mData.clear(); }

    RampType& value() { return mData; }
    const RampType& value() const { return mData; }

protected:
    virtual void readValue(std::istream&s, openvdb::Index32 numBytes) {
        OPENVDB_THROW(openvdb::TypeError, "Metadata has unknown type");
    }
    virtual void writeValue(std::ostream&) const {
        OPENVDB_THROW(openvdb::TypeError, "Metadata has unknown type");
    }

private:
    RampType mData;
};

/// @brief chramp function to query channel referenced ramps in Houdini
///
struct Chramp : public openvdb::ax::codegen::FunctionBase
{
    using FunctionBase = openvdb::ax::codegen::FunctionBase;

    struct Internal : public FunctionBase
    {
        inline FunctionBase::Context context() const override final {
            return FunctionBase::All;
        }
        inline const std::string identifier() const override final {
            return std::string("internal_chramp");
        }
        inline void getDocumentation(std::string& doc) const override final {
            doc = "Internal function for querying ramp data";
        }

        inline static FunctionBase::Ptr
        create(const openvdb::ax::FunctionOptions&) {
            return FunctionBase::Ptr(new Internal());
        }

        Internal() : FunctionBase({
                DECLARE_FUNCTION_SIGNATURE_OUTPUT(sample_map, 1)
            }) {}

    private:

        static void sample_map(const uint8_t* const name, float position, const void* const data, float (*out)[3])
        {
            using FloatRampCacheType = RampDataCache<float>;
            using FloatRamp = FloatRampCacheType::RampType;
            using VectorRampCacheType = RampDataCache<openvdb::math::Vec3<float>>;
            using VectorRamp = VectorRampCacheType::RampType;

            const openvdb::ax::CustomData* const customData =
                static_cast<const openvdb::ax::CustomData* const>(data);

            // clamp
            position = position > 0.0f ? position < 1.0f ? position : 1.0f : 0.0f;

            const std::string nameString(reinterpret_cast<const char* const>(name));

            const FloatRampCacheType* const floatRampData =
                customData->getData<FloatRampCacheType>(nameString);

            if (floatRampData) {
                const FloatRamp& ramp = floatRampData->value();
                auto upper = ramp.upper_bound(position);

                if (upper == ramp.begin()) {
                    (*out)[0] = upper->second;
                    (*out)[1] = (*out)[0];
                    (*out)[2] = (*out)[0];
                }
                else if (upper == ramp.end()) {
                    (*out)[0] = (--upper)->second;
                    (*out)[1] = (*out)[0];
                    (*out)[2] = (*out)[0];
                }
                else {
                    const float maxPos = upper->first;
                    const float maxVal = upper->second;

                    --upper;

                    const float minPos = upper->first;
                    const float minVal = upper->second;
                    const float coef = (position - minPos) / (maxPos - minPos);

                    // lerp
                    (*out)[0] = (minVal * (1.0f - coef)) + (maxVal * coef);
                    (*out)[1] = (*out)[0];
                    (*out)[2] = (*out)[0];
                }

                return;
            }

            const VectorRampCacheType* const vectorRampData =
                customData->getData<VectorRampCacheType>(nameString);

            if (vectorRampData) {
                const VectorRamp& ramp = vectorRampData->value();
                auto upper = ramp.upper_bound(position);

                if (upper == ramp.begin()) {
                    const openvdb::Vec3f& value = upper->second;
                    (*out)[0] = value[0];
                    (*out)[1] = value[1];
                    (*out)[2] = value[2];
                }
                else if (upper == ramp.end()) {
                    const openvdb::Vec3f& value = (--upper)->second;
                    (*out)[0] = value[0];
                    (*out)[1] = value[1];
                    (*out)[2] = value[2];
                }
                else {
                    const float maxPos = upper->first;
                    const openvdb::Vec3f& maxVal = upper->second;

                    --upper;

                    const float minPos = upper->first;
                    const openvdb::Vec3f& minVal = upper->second;
                    const float coef = (position - minPos) / (maxPos - minPos);

                    // lerp
                    const openvdb::Vec3f value = (minVal * (1.0f - coef)) + (maxVal * coef);
                    (*out)[0] = value[0];
                    (*out)[1] = value[1];
                    (*out)[2] = value[2];
                }
            }
        }
    };

    inline FunctionBase::Context context() const override final {
        return openvdb::ax::codegen::FunctionBase::All;
    }
    inline const std::string identifier() const override final {
        return std::string("chramp");
    }
    inline void getDocumentation(std::string& doc) const override final {
        doc = "Evaluate the channel referenced ramp value.";
    }

    Chramp() : openvdb::ax::codegen::FunctionBase({
            openvdb::ax::codegen::FunctionSignature<
                openvdb::ax::codegen::V3F*(const uint8_t* const, float)>::create
                    (nullptr, std::string("chramp"), 0)
        }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_chramp");
    }

    inline static FunctionBase::Ptr
    create(const openvdb::ax::FunctionOptions&) {
        return FunctionBase::Ptr(new Chramp());
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override {

        std::vector<llvm::Value*> internalArgs(args);
        internalArgs.emplace_back(globals.at("custom_data"));

        std::vector<llvm::Value*> results;
        Internal func;
        func.execute(internalArgs, globals, builder, M, &results);
        return results.front();
     }
};


///////////////////////////////////////////////////////////////////////////


void registerCustomHoudiniFunctions(openvdb::ax::codegen::FunctionRegistry& reg,
                                    const openvdb::ax::FunctionOptions& options)
{
    // @note - we could alias matching functions such as ch and chv here, but we opt
    // to use the modifier so that all supported VEX conversion is in one place. chramp
    // is a re-implemented function and is not currently supported outside of the Houdini
    // plugin

    if (!options.mLazyFunctions) {
        reg.insertAndCreate("chramp", openvdb_ax_houdini::Chramp::create, options);
        reg.insertAndCreate("internal_chramp",
            openvdb_ax_houdini::Chramp::Internal::create, options, true);
    }
    else {
        reg.insert("chramp", openvdb_ax_houdini::Chramp::create);
        reg.insert("internal_chramp", openvdb_ax_houdini::Chramp::Internal::create,  true);
    }
}

} // namespace openvdb_ax_houdini

#endif // OPENVDB_AX_HOUDINI_AX_UTILS_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
