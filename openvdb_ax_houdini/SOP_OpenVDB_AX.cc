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
//
/// @file SOP_OpenVDB_AX.cc
///
/// @author DNEG FX R&D
///
/// @brief AX SOP for OpenVDB Points and Volumes
///

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/ast/Literals.h>
#include <openvdb_ax/compiler/Compiler.h>
#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/compiler/PointExecutable.h>
#include <openvdb_ax/compiler/VolumeExecutable.h>

#include <openvdb_ax/deprecated/tools.h>

#include <houdini_utils/ParmFactory.h>
#include <houdini_utils/geometry.h>
#include <openvdb_houdini/Utils.h>
#include <openvdb_houdini/SOP_NodeVDB.h>

#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include <openvdb/points/PointDelete.h>
#include <openvdb/points/IndexIterator.h>

#include <CH/CH_Channel.h>
#include <CH/CH_Manager.h>
#include <CH/CH_LocalVariable.h>
#include <CMD/CMD_Manager.h>
#include <CMD/CMD_Variable.h>
#include <OP/OP_CommandManager.h>
#include <OP/OP_Director.h>
#include <OP/OP_Expression.h>
#include <OP/OP_Channels.h>
#include <PRM/PRM_Parm.h>
#include <UT/UT_Ramp.h>
#include <UT/UT_Version.h>

#include <tbb/mutex.h>

#include "ax/HoudiniAXUtils.h"

#if UT_MAJOR_VERSION_INT >= 16
#define VDB_COMPILABLE_SOP 1
#else
#define VDB_COMPILABLE_SOP 0
#endif

namespace hvdb = openvdb_houdini;
namespace hax =  openvdb_ax_houdini;
namespace hutil = houdini_utils;

using namespace openvdb;


////////////////////////////////////////


/// @brief OpPolicy for OpenVDB operator types from DNEG
class DnegVDBOpPolicy: public hutil::OpPolicy
{
public:
    std::string getName(const houdini_utils::OpFactory&, const std::string& english) override
    {
        UT_String s(english);
        // Remove non-alphanumeric characters from the name.
        s.forceValidVariableName();
        std::string name = s.toStdString();
        // Remove spaces and underscores.
        name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
        name.erase(std::remove(name.begin(), name.end(), '_'), name.end());
        name = "DN_" + name;
        return name;
    }

    /// @brief OpenVDB operators of each flavor (SOP, POP, etc.) share
    /// an icon named "SOP_OpenVDB", "POP_OpenVDB", etc.
    std::string getIconName(const houdini_utils::OpFactory& factory) override
    {
        return factory.flavorString() + "_OpenVDB";
    }
};

/// @brief OpFactory for OpenVDB operator types from DNEG
class DnegVDBOpFactory: public hutil::OpFactory
{
public:
    DnegVDBOpFactory(const std::string& english, OP_Constructor, hutil::ParmList&,
    OP_OperatorTable&, hutil::OpFactory::OpFlavor = SOP);
};

DnegVDBOpFactory::DnegVDBOpFactory(const std::string& english,
    OP_Constructor ctor,
    houdini_utils::ParmList& parms,
    OP_OperatorTable& table,
    hutil::OpFactory::OpFlavor flavor):
    hutil::OpFactory(DnegVDBOpPolicy(), english, ctor, parms, table, flavor) {}


////////////////////////////////////////


struct CompilerCache
{
    ax::Compiler::Ptr mCompiler = nullptr;
    ax::ast::Tree::Ptr mSyntaxTree = nullptr;
    ax::CustomData::Ptr mCustomData = nullptr;
    ax::PointExecutable::Ptr mPointExecutable = nullptr;
    ax::VolumeExecutable::Ptr mVolumeExecutable = nullptr;

    // point variables

    bool mRequiresDeletion = false;
};

/// @brief  A cached set of parameters, usually evaluated from the Houdini
///         UI which, on change, requires a re-compilation of AX. Does not
///         include the code snippet itself as this is handled separately.
/// @note   Should generally not be used to query the current state of the
///         parameters on the UI as this is only guaranteed to be up to date
///         on successfully compilations.
struct ParameterCache
{
    inline bool operator==(const ParameterCache& other) const
    {
        return mHScriptSupport == other.mHScriptSupport &&
               mVEXSupport == other.mVEXSupport &&
               mTargetType == other.mTargetType;
    }

    inline bool operator!=(const ParameterCache& other) const
    {
        return !(other == *this);
    }

    bool mHScriptSupport = true;
    bool mVEXSupport = true;
    hax::TargetType mTargetType = hax::TargetType::LOCAL;
};

/// @brief  Initialize the compiler function registry with a list of
///         available function calls. Optionally include houdini VEX hooks.
///
/// @param  compiler  The compiler object to set the function registry on
/// @param  allowVex  Whether to include support for available houdini functions
void initializeFunctionRegistry(ax::Compiler& compiler, const bool allowVex)
{
    ax::FunctionOptions functionOptions;
    ax::codegen::FunctionRegistry::UniquePtr functionRegistry =
        ax::codegen::createStandardRegistry(functionOptions);

    if (allowVex) {
        hax::registerCustomHoudiniFunctions(*functionRegistry, functionOptions);
    }

    compiler.setFunctionRegistry(std::move(functionRegistry));
}

////////////////////////////////////////


class SOP_OpenVDB_AX: public hvdb::SOP_NodeVDB
{
public:

    SOP_OpenVDB_AX(OP_Network*, const char* name, OP_Operator*);
    ~SOP_OpenVDB_AX() override = default;

    static OP_Node* factory(OP_Network*, const char* name, OP_Operator*);

#if VDB_COMPILABLE_SOP
    class Cache : public SOP_VDBCacheOptions
    {
    public:
        Cache();
        ~Cache() override = default;

        OP_ERROR cookVDBSop(OP_Context&) override;
        /// @brief  See SOP_OpenVDB_AX::evaluateExternalExpressions
        void evaluateExternalExpressions(const float time,
                        const hax::ChannelExpressionSet& set,
                        const bool hvars);
        /// @brief  See SOP_OpenVDB_AX::evalInsertHScriptVariable
        bool evalInsertHScriptVariable(const std::string& name,
                        const std::string& accessedType,
                        ax::CustomData& data);

    private:
        unsigned mHash;

        ParameterCache mParameterCache;
        CompilerCache mCompilerCache;

        // The current set of channel and $ expressions.

        hax::ChannelExpressionSet mChExpressionSet;
        hax::ChannelExpressionSet mDollarExpressionSet;
        std::vector<std::string> mWarnings;
    };

#else
protected:
    OP_ERROR cookVDBSop(OP_Context&) override;

private:
    /// @brief  Evaulate all externally requested channels and $ nodes and insert
    ///         any available values into AX's CustomData.
    ///
    /// @param  time   the context time at which to evaluate Houdini parameter values
    /// @param  set    the type name set of external channels to evaluate
    /// @param  hvars  whether or not to attempt to evaluate members of the expression set
    ///                as HScript variables first. Falls back to channel evaluation on failure
    void evaluateExternalExpressions(const float time,
                        const hax::ChannelExpressionSet& set,
                        const bool hvars);

    /// @brief  Given a name specified with an AX lookup, attempt to evaluate it as
    ///         if it were an accessible HScript Variable. If successful, add
    ///         into the custom data.
    /// @note   accessedType is the type requested by the user. If this is not the
    ///         actual type of the variable, we let the compiler set a zero val and
    ///         promote a warning.
    /// @note   Returns true if the variable was a valid HScript token and will be
    ///         treated as such. Does not guarantee the value has been updated. This
    ///         is dependant on the accessType.
    ///
    /// @param  name   The name of the HScript variable, also used as the name in AX
    /// @param  accessedType  The user requested type of the variable
    /// @param  data   The CustomData for AX
    bool evalInsertHScriptVariable(const std::string& name,
                        const std::string& accessedType,
                        ax::CustomData& data);

    unsigned mHash;

    ParameterCache mParameterCache;
    CompilerCache mCompilerCache;

    // The current set of channel and $ expressions.

    hax::ChannelExpressionSet mChExpressionSet;
    hax::ChannelExpressionSet mDollarExpressionSet;
    std::vector<std::string> mWarnings;

#endif

protected:
    bool updateParmsFlags() override;

}; // class SOP_OpenVDB_AX


////////////////////////////////////////


// Build UI and register this operator.
void
newSopOperator(OP_OperatorTable* table)
{
    if (table == nullptr) return;

    hutil::ParmList parms;

    parms.add(hutil::ParmFactory(PRM_STRING, "vdbgroup", "Group")
        .setHelpText("Specify a subset of the input VDB grids to be processed.")
        .setChoiceList(&hutil::PrimGroupMenu));

    {
        const char* items[] = {
            "points",   "Points",
            "volumes",  "Volumes",
            nullptr
        };

        // attribute compression menu
        parms.add(hutil::ParmFactory(PRM_ORD, "targettype", "Target Type")
            .setDefault("points")
            .setHelpText("Whether to run this snippet over OpenVDB Points or OpenVDB Volumes.")
                .setChoiceListItems(PRM_CHOICELIST_SINGLE, items));
    }

    parms.add(hutil::ParmFactory(PRM_STRING, "pointsgroup", "VDB Points Group")
        .setHelpText("Specify a point group name to perform the execution on. If no name is "
                     "given, the AX snippet is applied to all points."));

    parms.add(hutil::ParmFactory(PRM_STRING, "snippet", "AX Expression")
        .setHelpText("A snippet of AX code that will manipulate the attributes on the VDB Points or "
                     "the VDB voxel values.")
        .setSpareData(&PRM_SpareData::stringEditor));

    // language modifiers

    parms.add(hutil::ParmFactory(PRM_TOGGLE, "allowvex", "Allow VEX")
        .setDefault(PRMoneDefaults)
        .setHelpText("Whether to enable support for various VEX functionality. When disabled, only AX "
                     "syntax is supported."));

    parms.add(hutil::ParmFactory(PRM_TOGGLE, "hscriptvars", "Allow HScript Variables")
        .setDefault(PRMoneDefaults)
        .setHelpText("Whether to enable support for various $ variables available in the current node's "
                     "context. As $ is used for custom parameters in AX, a warning will be generated "
                     "if a Houdini parameter also exists of the same name as the given $ variable."));

    // vdb modifiers

    parms.add(hutil::ParmFactory(PRM_TOGGLE, "prune", "Prune")
        .setDefault(PRMoneDefaults)
        .setHelpText("Whether to prune VDBs after execution. Does not affect VDB Point Grids."));


    //////////
    // Register this operator.

    DnegVDBOpFactory("OpenVDB AX",
        SOP_OpenVDB_AX::factory, parms, *table)
        .addInput("VDBs to manipulate")
#if VDB_COMPILABLE_SOP
        .setVerb(SOP_NodeVerb::COOK_INPLACE, []() { return new SOP_OpenVDB_AX::Cache; })
#endif
        .setDocumentation("\
#internal: DN_OpenVDBAX\n\
#icon: COMMON/openvdb\n\
#tags: vdb\n\
\n\
\"\"\"Runs an AX snippet to modify point and volume values in VDBs\"\"\"\n\
\n\
Please contact the [OpenVDB AX team|mailto:openvdbax@dneg.com] with any bug reports, questions or suggestions.\n\
\n\
@overview\n\
\n\
This is a very powerful, low-level node that lets those who are familiar with the AX language manipulate attributes on points and voxel values in VDBs.\n\
\n\
AX is a language created by the DNEG FX R&D team that closely matches VEX but operates natively on VDB point and volume grids.\n\
Note that the language is not yet as extensive as Houdini VEX and only supports a subset of similar functionality.\n\
\n\
@examples\n\
\n\
{{{\n\
#!vex\n\
@density = 1.0f; // Set the float attribute density to 1.0f\n\
}}}\n\
{{{\n\
#!vex\n\
i@id = 5; // Set the integer attribute id to 5\n\
}}}\n\
{{{\n\
#!vex\n\
vec3f@v1 = { 5.0f, 5.0f, 10.3f }; // Create a new float vector attribute\n\
vector@v2 = { 5.0f, 5.0f, 10.3f }; // Create a new float vector attribute using VEX syntax \n\
}}}\n\
{{{\n\
#!vex\n\
vec3i@vid = { 3, -1, 10 }; // Create a new integer vector attribute\n\
}}}\n\
@vexsyntax VEX Hooks\n\
The OpenVDB AX Houdini SOP supports all features of AX and a variety of Houdini VEX syntax/features to help users transition into writing AX code.\n\
The VEX feature set also gives users the ability to write typical Houdini specific functions within AX. The table below lists all VEX\n\
features which can be used, as well as equivalent AX functionality. If no AX function is shown, the VEX function can still be used but will not be\n\
available outside of a Houdini context.\n\
:note: Allow Vex Symbols must be enabled to access these features.\n\
:note: `$` AX syntax should always be used over the AX lookup() functions unless attempting to query unknown strings.\n\
\n\
VEX Syntax/Function ||\n\
    AX Syntax/Function ||\n\
        Description ||\n\
\n\
`ch(string_path)` |\n\
    `$string_path, lookupf(string_path)` |\n\
        Finds a float channel value. \n\
\n\
`chv(string_path)` |\n\
    `v$string_path, lookupvec3f(string_path)` |\n\
        Finds a string channel value. \n\
\n\
`chramp(string_path)` |\n\
    |\n\
        Provides access to the chramp VEX function. \n\
\n\
`vector` |\n\
    `vec3f` |\n\
        Syntax for creating a vector 3 of floats. \n\
\n\
`@ix, @iy, @iz` |\n\
    `getcoordx(), getcoordy(), getcoordz()` |\n\
        When executing over volumes, returns the index X, Y, Z coordinate of the current voxel.\n\
\n\
@hscriptsyntax HScript Variables\n\
HScript $ variables can also be accessed within AX. Note that the $ syntax in AX is equivalent to a Houdini channel function and is used to look-up\n\
custom variables within AX. A different set of HScript variables will be available depending on the current Houdini Context. For a complete\n\
list, [see here|/network/expressions#globals]\n\
:note: Allow HScript Variables must be enabled to access HScript variables.\n\
:tip: `@Frame` and `@Time` can be accessed with `$F` and `$T` respectively.\n\
\n\
@axverb AX as a Python Verb\n\
The AX SOP can be used within compiled blocks and as a verb through Houdini's python interface. The latter however introduces some restrictions to\n\
the code which can be used due to the lack of a connected Houdini network. Through Python, the following restriction are imposed:\n\
* $ Syntax for paths cannot be used. `ch` and `lookup` should be used instead.\n\
\n\
* Relative channel paths with `ch` and `lookup` functions will produce an error. These must be converted to absolute paths.\n\
\n\
For more information on Compiled Blocks and Python verbs [see here|/model/compile].\n\
\n\
@functions Supported Functions\n\
#filtered: no\n\
\n\
Function ||\n\
    Description ||\n\
\n\
`int abs(int),` \n\
`long abs(long)` |\n\
    Computes the absolute value of an integer number.\n\
\n\
`double acos(double),` \n\
`float acos(float)` |\n\
    Computes the principal value of the arc cosine of the input.\n\
\n\
`void addtogroup(string)` |\n\
    Add the current point to the given group name, effectively setting its membership to true. \n\
    If the group does not exist, it is implicitly created. \n\
    This function has no effect if the point already belongs to the given group.\n\
\n\
`double asin(double),` \n\
`float asin(float)` |\n\
    Computes the principal value of the arc sine of the input.\n\
\n\
`double atan(double),` \n\
`float atan(float)` |\n\
    Computes the principal value of the arc tangent of the input.\n\
\n\
`double atan2(double, double),` \n\
`float atan2(float, float)` |\n\
    Computes the arc tangent of y/x using the signs of arguments to determine the correct quadrant.\n\
\n\
`double atof(string)` |\n\
    Parses the string input, interpreting its content as a floating point number and returns its value\n\
    as a double.\n\
\n\
`int atoi(string),` \n\
`long atoi(string)` |\n\
    Parses the string input, interpreting its content as a integral number and returns its value\n\
    of type int.\n\
\n\
`double ch(string)` |\n\
    Returns the value of the supplied scalar Houdini parameter channel.\n\
\n\
`vector chv(string)` |\n\
    Returns the value of the supplied vector Houdini parameter channel.\n\
\n\
`double chramp(string, double)`,\n\
`vec3d chramp(string, double)` |\n\
    Samples the Houdini ramp parameter, for both scalar and vector ramps.\n\
\n\
`bool ingroup(string)` |\n\
    Returns whether or not the point being accessed is a member of the provided group.\n\
\n\
`double sqrt(double)` |\n\
    Return the square root of the argument.\n\
\n\
`double sin(double)` |\n\
    Returns the sine of the argument.\n\
\n\
`double cos(double)` |\n\
    Returns the cosine of the argument.\n\
\n\
`double tan(double)` |\n\
    Returns the tangent of the argument.\n\
\n\
`double log(double)` |\n\
    Returns the natural logarithm of the argument.\n\
\n\
`double log10(double)` |\n\
    Returns the logarithm (base 10) of the argument.\n\
\n\
`double log2(double)` |\n\
    Returns the logarithm (base 2) of the argument.\n\
\n\
`double exp(double)` |\n\
    Returns the exponential function of the argument.\n\
\n\
`double exp2(double)` |\n\
    Returns 2 raised to the power of the argument.\n\
\n\
`double fabs(double)` |\n\
    Returns the absolute value of the argument.\n\
\n\
`double floor(double)` |\n\
    Returns the largest integer less than or equal to the argument.\n\
\n\
`double ceil(double)` |\n\
    Returns the smallest integer greater than or equal to the argument.\n\
\n\
`double pow(double, double)` |\n\
    Raises the first argument to the power of the second argument.\n\
\n\
`double round(double)` |\n\
    Round to the closest integer.\n\
\n\
`double min(double, double)` |\n\
    Return the minimum of two values.\n\
\n\
`double max(double, double)` |\n\
    Return the maximum of two values.\n\
\n\
`double clamp(double value, double min, double max)` |\n\
    Returns value clamped between min and max.\n\
\n\
`double fit(double value, double omin, double omax, double nmin, double nmax)` |\n\
    Takes the value in the range (omin, omax) and shifts it to the corresponding value in the new range (nmin, nmax).\n\
\n\
`double length(vec3d)` |\n\
    Return the length (distance) of the vector.\n\
\n\
`double lengthsq(vec3d)` |\n\
    Returns the squared length (distance) of the vector.\n\
\n\
`vec3d normalize(vec3d)` |\n\
    Returns the normalized vector.\n\
\n\
`double dot(vec3d, vec3d)` |\n\
    Returns the dot product between the arguments.\n\
\n\
`double dot(vec3d, vec3d)` |\n\
    Returns the dot product between the arguments.\n\
\n\
`double rand(double)` |\n\
    Generate a random floating-point number in the range `[0,1)` from a double-precision seed value. Repeated calls to this function with the same seed yields the same result.\n\
\n\
:note:\n\
For an up-to-date list of available functions, see AX documentation or call `vdb_ax --list-functions` from the command line.\n\
\n\
");
}

////////////////////////////////////////


OP_Node*
SOP_OpenVDB_AX::factory(OP_Network* net,
                                    const char* name, OP_Operator* op)
{
    return new SOP_OpenVDB_AX(net, name, op);
}

#if VDB_COMPILABLE_SOP
SOP_OpenVDB_AX::SOP_OpenVDB_AX(OP_Network* net,
        const char* name, OP_Operator* op)
    : hvdb::SOP_NodeVDB(net, name, op)
{
    ax::initialize();
}

SOP_OpenVDB_AX::Cache::Cache()
    : mHash(0)
    , mParameterCache()
    , mCompilerCache()
    , mChExpressionSet()
    , mDollarExpressionSet()
    , mWarnings()
{
    mCompilerCache.mCompiler = ax::Compiler::create();
    mCompilerCache.mCustomData.reset(new ax::CustomData);

    // initialize the function registry with VEX support as default
    initializeFunctionRegistry(*mCompilerCache.mCompiler, /*allow vex*/true);
}

#else
SOP_OpenVDB_AX::SOP_OpenVDB_AX(OP_Network* net,
        const char* name, OP_Operator* op)
    : hvdb::SOP_NodeVDB(net, name, op)
    , mHash(0)
    , mParameterCache()
    , mCompilerCache()
    , mChExpressionSet()
    , mDollarExpressionSet()
    , mWarnings()
{
    ax::initialize();

    mCompilerCache.mCompiler = ax::Compiler::create();
    mCompilerCache.mCustomData.reset(new ax::CustomData);

    // initialize the function registry with VEX support as default
    initializeFunctionRegistry(*mCompilerCache.mCompiler, /*allow vex*/true);
}
#endif

bool
SOP_OpenVDB_AX::updateParmsFlags()
{
    bool changed = false;
    const bool points = evalInt("targettype", 0, 0) == 0;
    changed |= enableParm("pointsgroup", points);
    changed |= enableParm("prune", !points);


    return changed;
}


////////////////////////////////////////


struct PruneOp {
    template<typename GridT>
    void operator()(GridT& grid) const {
        tools::prune(grid.tree());
    }
};

OP_ERROR
VDB_NODE_OR_CACHE(VDB_COMPILABLE_SOP, SOP_OpenVDB_AX)::cookVDBSop(OP_Context& context)
{
    try {

#if !VDB_COMPILABLE_SOP
        hutil::ScopedInputLock lock(*this, context);
        lock.markInputUnlocked(0);
        duplicateSourceStealable(0, context);
        if (gdp == nullptr) return error();
        SOP_OpenVDB_AX* self = this;
#else
        // may be null if cooking as a verb i.e. through python
        SOP_OpenVDB_AX* self =
            static_cast<SOP_OpenVDB_AX*>(this->cookparms()->getSrcNode());
#endif

        hvdb::Interrupter boss("Executing OpenVDB AX");

        const fpreal time = context.getTime();

        // Get ui params, including grids to process
        UT_String groupStr;
        evalString(groupStr, "vdbgroup", 0, time);
        const GA_PrimitiveGroup *group =
            matchGroup(const_cast<GU_Detail&>(*gdp), groupStr.toStdString());
        groupStr.clear();

        hvdb::VdbPrimIterator vdbIt(gdp, group);
        if (!vdbIt) return error();

        // Evaluate the code snippet field

        // @note We generally want to evaluate the raw string to optimise channel
        // links and stop $ variables from auto expanding. If $ variables are
        // Houdini variables, such as $F and $T then it's fine if they're expanded,
        // but we won't be able to optimise using AX's custom data (the string will
        // require re-compilation on every cook). However, if $ variables are paths
        // using AX's $ syntax then we don't want to auto expand these to null
        // values. Unfortunately, if we've built with compiled SOP support, there's
        // no way to evaluate the raw string unless a source node instance exists.
        // This won't exist if it's being cooked as a verb through python - we
        // fall back to calling evalString in this case, but this will cause errors
        // if $ is being used for path syntax.

        UT_String snippet;
        if (self) self->evalStringRaw(snippet, "snippet", 0, time);
        else      this->evalString(snippet, "snippet", 0, time);
        if (snippet.length() == 0) return error();

        const int targetInt = evalInt("targettype", 0, time);

        ParameterCache parmCache;
        parmCache.mTargetType = static_cast<hax::TargetType>(targetInt);
        parmCache.mVEXSupport = evalInt("allowvex", 0, time);
        parmCache.mHScriptSupport = evalInt("hscriptvars", 0, time);

        // @TODO use parameter update notifications to query if the snippet
        // has changed rather than hashing the code

        const unsigned hash = snippet.hash();
        const bool recompile =
            (hash != mHash || parmCache != mParameterCache);

        if (recompile) {

            // Empty the hash - if there are any compiler failures, the hash won't be
            // initialized but the engine data maybe modified. If the code is then changed
            // back to the previous hash, this path will not be correctly executed
            // without this explicit change

            mHash = 0;

            mWarnings.clear();
            mChExpressionSet.clear();
            mDollarExpressionSet.clear();

            // if VEX support flag has changes, re-initialize the available functions

            if (mParameterCache.mVEXSupport != parmCache.mVEXSupport) {
                initializeFunctionRegistry(*mCompilerCache.mCompiler, parmCache.mVEXSupport);
            }

            // build the AST from the provided snippet

            mCompilerCache.mSyntaxTree = ax::ast::parse(snippet.nonNullBuffer());

            // find all externally accessed data - do this before conversion from VEX
            // so identify HScript tokens which have been explicitly requested with $
            // (as otherwise we'll pick up optimised or user $ paths)

            hax::findChannelExpressions(*mCompilerCache.mSyntaxTree, mChExpressionSet);
            hax::findDollarExpressions(*mCompilerCache.mSyntaxTree, mDollarExpressionSet);

            // begin preprocessors

            if (parmCache.mVEXSupport) {

                // if we're allowing VEX syntax, convert any supported VEX functions and
                // accesses to AX syntax. Note that there may be some functions, such as
                // chramp, that are not reliant on VEX as we re-implement them in the AX
                // Houdini plugin but not yet in the AX Core library

                hax::convertASTFromVEX(*mCompilerCache.mSyntaxTree, parmCache.mTargetType);
            }

            // optimise lookup function calls into $ calls if the argument is a string literal

            hax::convertASTKnownLookups(*mCompilerCache.mSyntaxTree);

            // end preprocessors

            // reset any custom data

            mCompilerCache.mCustomData->reset();

            // initialize local variables - do this outside of evaluateExternalExpressions
            // so it's not called for every cook if nothing has changed

            if (!mDollarExpressionSet.empty() && parmCache.mHScriptSupport) {
#if VDB_COMPILABLE_SOP
                this->cookparms()->setupLocalVars();
#else
                this->setupLocalVars();
#endif
            }

            evaluateExternalExpressions(time, mChExpressionSet, /*no $ support*/false);
            evaluateExternalExpressions(time, mDollarExpressionSet, parmCache.mHScriptSupport);

            if (parmCache.mTargetType == hax::TargetType::POINTS) {

                mCompilerCache.mRequiresDeletion =
                    openvdb::ax::ast::callsFunction(*mCompilerCache.mSyntaxTree, "deletepoint");

                mCompilerCache.mPointExecutable =
                    mCompilerCache.mCompiler->compile<ax::PointExecutable>
                        (*mCompilerCache.mSyntaxTree, mCompilerCache.mCustomData, &mWarnings);
            }
            else if (parmCache.mTargetType == hax::TargetType::VOLUMES) {
                mCompilerCache.mVolumeExecutable =
                    mCompilerCache.mCompiler->compile<ax::VolumeExecutable>
                        (*mCompilerCache.mSyntaxTree, mCompilerCache.mCustomData, &mWarnings);
            }

            // update the parameter cache

            mParameterCache = parmCache;

            // set the hash only if compilation was successful - Houdini sops tend to cook
            // multiple times, especially on fail. If we assign the hash prior to this it will
            // be incorrectly cached

            mHash = hash;
        }
        else {
            evaluateExternalExpressions(time, mChExpressionSet, /*no $ support*/false);
            evaluateExternalExpressions(time, mDollarExpressionSet, parmCache.mHScriptSupport);
        }

        snippet.clear();

        for (const std::string& warning : mWarnings) {
            addWarning(SOP_MESSAGE, warning.c_str());
        }

        if (mParameterCache.mTargetType == hax::TargetType::POINTS) {

            const bool automaticSorting(evalInt("autosort", 0, time) != 0);

            UT_String pointsStr;
            evalString(pointsStr, "pointsgroup", 0, time);
            const std::string pointsGroup = pointsStr.toStdString();

            for (; vdbIt; ++vdbIt) {
                if (boss.wasInterrupted()) {
                    throw std::runtime_error("processing was interrupted");
                }

                GU_PrimVDB* vdbPrim = *vdbIt;

                if (!(vdbPrim->getConstGridPtr()->isType<points::PointDataGrid>())) continue;
                vdbPrim->makeGridUnique();

                points::PointDataGrid::Ptr points =
                    gridPtrCast<points::PointDataGrid>(vdbPrim->getGridPtr());

                if (!mCompilerCache.mPointExecutable) {
                    throw std::runtime_error("No point executable has been built");
                }

                mCompilerCache.mPointExecutable->execute(*points, &pointsGroup);

                if (!automaticSorting) continue;

                if (mCompilerCache.mRequiresDeletion) {
                    openvdb::points::deleteFromGroup(points->tree(), "dead", false, false);
                }
            }
        }
        else if (mParameterCache.mTargetType == hax::TargetType::VOLUMES) {

            GridPtrVec grids;
            std::vector<GU_PrimVDB*> guPrims;
            std::set<std::string> names;

            for (; vdbIt; ++vdbIt) {
                if (boss.wasInterrupted()) {
                    throw std::runtime_error("processing was interrupted");
                }

                GU_PrimVDB* vdbPrim = *vdbIt;
                if (vdbPrim->getConstGridPtr()->isType<points::PointDataGrid>()) continue;
                vdbPrim->makeGridUnique();

                const std::string& name = vdbPrim->getConstGridPtr()->getName();
                if (names.count(name)) {
                    addWarning(SOP_MESSAGE,
                        std::string("Multiple VDBs \"" + name + "\" encountered. "
                        "Only the first grid will be processed.").c_str());
                }

                names.insert(name);
                grids.emplace_back(vdbPrim->getGridPtr());
                guPrims.emplace_back(vdbPrim);
            }

            if (!mCompilerCache. mVolumeExecutable) {
                throw std::runtime_error("No volume executable has been built");
            }

            mCompilerCache. mVolumeExecutable->execute(grids);

            if (evalInt("prune", 0, time)) {
                PruneOp op;
                for (auto& vdbPrim : guPrims) {
                    GEOvdbProcessTypedGridTopology(*vdbPrim, op, /*make_unique*/false);
                }
            }
        }

    } catch (std::exception& e) {
        addError(SOP_MESSAGE, e.what());
    }
    return error();
}

bool
VDB_NODE_OR_CACHE(VDB_COMPILABLE_SOP, SOP_OpenVDB_AX)::evalInsertHScriptVariable(const std::string& name,
                                                                      const std::string& accessedType,
                                                                      ax::CustomData& data)
{
    OP_Director* const director = OPgetDirector();
    OP_CommandManager* const manager = director ? director->getCommandManager() : nullptr;
    CMD_VariableTable* const table = manager ? manager->getGlobalVariables() : nullptr;

    bool isVariable = false;

    std::unique_ptr<UT_String> valueStrPtr;
    std::unique_ptr<fpreal32> valueFloatPtr;
    std::string expectedType;

    if (table && table->hasVariable(name.c_str())) {

        isVariable = true;

        // the accessed variable is a valid hscript global var - attempt to evaluate it as
        // a float or a string. If the float evaluation fails or returns 0.0f, assume the
        // variable is a string

        UT_String valueStr;
        table->getVariable(name.c_str(), valueStr);

        if (valueStr.length() > 0) {
            const std::string str = valueStr.toStdString();
            try {
                const fpreal32 valueFloat =
                    static_cast<fpreal32>(openvdb::ax::LiteralLimits<float>::convert(str));
                valueFloatPtr.reset(new fpreal32(valueFloat));
                expectedType = openvdb::typeNameAsString<float>();
            }
            catch (...) {}

            if (!valueFloatPtr) {
                valueStrPtr.reset(new UT_String(valueStr));
                expectedType = openvdb::typeNameAsString<std::string>();
            }
        }
    }
    else {

        // not a global variable, attempt to evaluate as a local

#if VDB_COMPILABLE_SOP
        OP_Node* self = this->cookparms()->getCwd();
#else
        OP_Node* self = this;
#endif

        OP_Channels* channels = self->getChannels();
        if (!channels) return false;

        int index = -1;
        const CH_LocalVariable* const var =
            channels->resolveVariable(name.c_str(), index);
        if (!var) return false;

        isVariable = true;
        UT_ASSERT(index >= 0);

        expectedType = var->flag & CH_VARIABLE_STRVAL ?
            openvdb::typeNameAsString<std::string>() :
            openvdb::typeNameAsString<float>();

        if (var->flag & CH_VARIABLE_STRVAL) {
            UT_String value;
            if (channels->getVariableValue(value, index, var->id, /*thread*/0)) {
                valueStrPtr.reset(new UT_String(value));
            }
        }
        else {
            fpreal value;
            if (channels->getVariableValue(value, index, var->id, /*thread*/0)) {
                valueFloatPtr.reset(new fpreal32(value));
            }
        }

        // If the channel is time dependent, ensure it's propagated to this node

        if (valueFloatPtr || valueStrPtr) {
#if VDB_COMPILABLE_SOP
            DEP_MicroNode* dep = this->cookparms()->depnode();
            if (dep && !dep->isTimeDependent() && var->isTimeDependent()) {
                dep->setTimeDependent(true);
            }
#else
            this->flags().timeDep |= var->isTimeDependent();
#endif
        }
    }

    if (!isVariable) return false;

    if (valueFloatPtr || valueStrPtr) {

        // if here, we've evaluated the variable successfully as either a float or string

        if (accessedType != expectedType) {
            // If the types differ, differ to the compiler to insert the correct zero val
            const std::string message = "HScript variable \"" + name + "\" has been accessed"
                " with an incompatible type. Expected to be \"" + expectedType + "\". Accessed "
                " with \"" + accessedType + "\".";
            addWarning(SOP_MESSAGE, message.c_str());
        }
        else if (valueStrPtr) {
            typename TypedMetadata<std::string>::Ptr meta(new TypedMetadata<std::string>(valueStrPtr->toStdString()));
            data.insertData<TypedMetadata<std::string>>(name, meta);
        }
        else {
            UT_ASSERT(valueFloatPtr);
            typename TypedMetadata<float>::Ptr meta(new TypedMetadata<float>(*valueFloatPtr));
            data.insertData<TypedMetadata<float>>(name, meta);
        }

        return true;
    }

    // we've been unable to insert a valid variable due to some internal Houdini
    // type evaluation error. The compiler will ensure that it's initialized to a
    // valid zero val.

    const std::string message = "Unable to evaluate accessed HScript Variable \"" + name + "\".";
    addWarning(SOP_MESSAGE, message.c_str());
    return false;
}

void
VDB_NODE_OR_CACHE(VDB_COMPILABLE_SOP, SOP_OpenVDB_AX)::evaluateExternalExpressions(const float time,
                                                                    const hax::ChannelExpressionSet& set,
                                                                    const bool hvars)
{
    using VectorData = TypedMetadata<math::Vec3<float>>;
    using FloatData = TypedMetadata<float>;
    using FloatRampData = hax::RampDataCache<float>;
    using VectorRampData = hax::RampDataCache<math::Vec3<float>>;

    ax::CustomData& data = *(mCompilerCache.mCustomData);

    // For compilable SOPs, see if we can connect this cache back to a SOP instance by
    // querying the source node. If this doesn't exist, then we're most likely being
    // cooked as a verb through python and we'll be unable to evaluate relative
    // references.

#if VDB_COMPILABLE_SOP
    OP_Node* self = this->cookparms()->getCwd();
    const bool hasSrcNode = this->cookparms()->getSrcNode() != nullptr;
    DEP_MicroNode* dep = this->cookparms()->depnode();
#else
    SOP_OpenVDB_AX* self = this;
    const bool hasSrcNode = true;
    DEP_MicroNode* dep = &(this->dataMicroNode());
#endif

    for (const hax::ChannelExpressionPair& expresionPair : set) {

        // get the type that was requested and the name of the item. The name is
        // either a channel path or a Houdini HScript Variable

        const std::string& type = expresionPair.first;
        const std::string& nameOrPath = expresionPair.second;
        if (nameOrPath.empty()) continue;

        // Process the item as if it were a hscript token first if hscript support
        // is enabled.

        if (hvars) {

            // try and add this item with the requested type and name. If the type
            // doesnt match it's actual type, defer to the compiler to initialise a zero val
            // item and continue with a warning. If the name isn't a hscript token, it's
            // most likely a channel path

            // @note that for compiled SOPs being cooked as verbs this will always return false
            // as we evaluate the expanded string .i.e. if hasSrcNode is false, nameOrPath will
            // never be a $ variable. Execute this branch anyway to support this in the future

            if (this->evalInsertHScriptVariable(nameOrPath, type, data)) {

                // see if the current SOP instance has a parm

                if (hasSrcNode && self->hasParm(nameOrPath.c_str())) {
                    const std::string message = "Initialized HScript Token \"" + nameOrPath +
                        "\" is also a valid channel path. Consider renaming this parameter.";
                    addWarning(SOP_MESSAGE, message.c_str());
                }
                continue;
            }
        }

        // If running in python, we can't process relative channel links as we don't know
        // the source location

        const bool isAbsolutePath = nameOrPath[0] == '/';
        if (!hasSrcNode && !isAbsolutePath) {
            throw std::runtime_error("Unable to process relative channel link \"" + nameOrPath
                + "\" when cooking as a verb.");
        }

        // if we're here, process the item as a channel

        // @note this currently matches houdini vex behaviour
        //
        // - 1) ch(p) with p = parm - return parm evalauted as float
        // - 2) ch(p) with p = channel - return channel evalauted as float
        //        in both cases, if the parm is not a single value, return 0
        //
        // - 3) chv(p) with p = parm - return parm evalauted as vec
        //        if p is not a vec3, fill as many elements as possible.
        //        for example, if p is a scalar value of 1, return (1,0,0)
        //        if p is a vec2 (2,3), return (2,3,0)
        // - 4) chv(p) with p = channel - return channel evalauted as vec
        //        as it's a channel it's always going to be a single value.
        //        in this case return a vector filled with the single value.
        //        for example, if p = 1, return (1,1,1)
        //
        //  -5) chramp(p) - as ramps are alwyas multi parms, we don't  have
        //        to consider the case where it could be a channel

        const bool isCHRampLookup(type == "ramp");
        const bool isCHLookup(!isCHRampLookup && type == openvdb::typeNameAsString<float>());
        const bool isCHVLookup(!isCHRampLookup && !isCHLookup &&
            type == openvdb::typeNameAsString<openvdb::Vec3f>());
        const bool lookForChannel = !isCHRampLookup;

        // findParmRelativeTo finds the node and parameter index on the node which is
        // related to the nameOrPath relative to this node
        // @note Do NOT use OPgetParameter() directly as this seems to cause crashes
        // when used with various DOP networks

        // @todo: cache channelFinder?

        int index(0);
        int subIndex(0);
        OP_Node* node(nullptr);

        OP_ExprFindCh channelFinder;
        bool validPath =
            channelFinder.findParmRelativeTo(*self,
                                             nameOrPath.c_str(),
                                             time,
                                             node,            /*the node holding the param*/
                                             index,           /*parm index on the node*/
                                             subIndex,        /*sub index of parm if not channel*/
                                             lookForChannel); /*is_for_channel_name*/

        // if no channel found and we're using CHV, try looking for the parm directly

        if (!validPath && isCHVLookup) {
            validPath =
                channelFinder.findParmRelativeTo(*self,
                                                 nameOrPath.c_str(),
                                                 time,
                                                 node,       /*the node holding the param*/
                                                 index,      /*parm index on the node*/
                                                 subIndex,   /*sub index of parm if not channel*/
                                                 false);     /*is_for_channel_name*/
        }

        if (validPath) {

            assert(node);

            if (isCHVLookup) {

                Vec3f value;
                if (subIndex != -1) {
                    // parm was a channel
                    value = openvdb::Vec3f(node->evalFloat(index, subIndex, time));
                }
                else {
                    // parm was a direct parm
                    value[0] = node->evalFloat(index, 0, time);
                    value[1] = node->evalFloat(index, 1, time);
                    value[2] = node->evalFloat(index, 2, time);
                }

                VectorData::Ptr vecData(new VectorData(value));
                data.insertData<VectorData>(nameOrPath, vecData);

                // add an extra input to all the relevant channels of this
                // parameter if this dep micronode exists

                if (dep) {
                    PRM_Parm& parm = node->getParm(index);

                    // micro node is guaranteed to exist as we've evaluated the param
                    if (subIndex == -1) {
                        dep->addExplicitInput(parm.microNode(0));
                        dep->addExplicitInput(parm.microNode(1));
                        dep->addExplicitInput(parm.microNode(2));
                    }
                    else {
                        dep->addExplicitInput(parm.microNode(subIndex));
                    }
                }
            }
            else if (isCHLookup) {

                assert(subIndex != -1);

                // use evalFloat rather than parm->getValue() to wrap the conversion to a float
                const float value = node->evalFloat(index, subIndex, time);

                FloatData::Ptr floatData(new FloatData(value));
                data.insertData(nameOrPath, floatData);

                // add a dependency to this micronode if it exists

                if (dep) {
                    // micro node is guaranteed to exist as we've evaluated the param
                    PRM_Parm& parm = node->getParm(index);
                    dep->addExplicitInput(parm.microNode(subIndex));
                }
            }
            else if (isCHRampLookup) {

                PRM_Parm& parm = node->getParm(index);
                const bool isRamp = parm.isRampType();

                if (!isRamp) {
                    const std::string message =
                        "Invalid channel reference: " + nameOrPath + ". Parameter is not a ramp.";
                    addWarning(SOP_MESSAGE, message.c_str());
                    FloatRampData::Ptr floatRampData(new FloatRampData());
                    data.insertData(nameOrPath, floatRampData);
                    continue;
                }

                UT_Ramp houdiniRamp;
                node->updateRampFromMultiParm(time, parm, houdiniRamp);

                const int numNodes = houdiniRamp.getNodeCount();
                const bool isVectorRamp = parm.isRampTypeColor();
                if (!isVectorRamp) {
                    // must be a float ramp

                    FloatRampData::Ptr floatRampData(new FloatRampData());

                    for (int i = 0; i < numNodes; ++i) {
                        const UT_ColorNode* const rampNode = houdiniRamp.getNode(i);
                        floatRampData->insertRampPoint(rampNode->t, rampNode->rgba.r);
                    }

                    data.insertData(nameOrPath, floatRampData);
                }
                else {
                    VectorRampData::Ptr vectorRampData(new VectorRampData());

                    for (int i = 0; i < numNodes; ++i) {
                        const UT_ColorNode* const rampNode = houdiniRamp.getNode(i);
                        vectorRampData->insertRampPoint(rampNode->t,
                            Vec3R(rampNode->rgba.r, rampNode->rgba.g, rampNode->rgba.b));
                    }

                    data.insertData(nameOrPath, vectorRampData);
                }

                // add all parms of this ramps multi parm as a dependency to this
                // micronode if it exists

#if UT_MAJOR_VERSION_INT <= 15
                const OP_InterestRef ref(*this, OP_INTEREST_DATA);
                OP_Node::addMultiparmInterests(ref, node, parm);
#else
                if (dep) {
                    OP_Node::addMultiparmInterests(*dep, node, parm);
                }
#endif
            }
        }
        else {

            if (isCHVLookup) {
                VectorData::Ptr vecData(new VectorData(openvdb::Vec3f::zero()));
                data.insertData<VectorData>(nameOrPath, vecData);
            }
            else if (isCHLookup) {
                FloatData::Ptr floatData(new FloatData(0.0f));
                data.insertData<FloatData>(nameOrPath, floatData);
            }
            else if (isCHRampLookup) {
                FloatRampData::Ptr floatRampData(new FloatRampData());
                data.insertData<FloatRampData>(nameOrPath, floatRampData);
            }

            const std::string message = "Invalid channel reference: " + nameOrPath;
            addWarning(SOP_MESSAGE, message.c_str());
        }
    }
}


////////////////////////////////////////

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
