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
#include <openvdb_ax/ast/PrintTree.h>
#include <openvdb_ax/compiler/Compiler.h>
#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/compiler/PointExecutable.h>
#include <openvdb_ax/compiler/VolumeExecutable.h>

#include <houdini_utils/ParmFactory.h>
#include <houdini_utils/geometry.h>
#include <openvdb_houdini/Utils.h>
#include <openvdb_houdini/SOP_NodeVDB.h>

#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include <openvdb/points/PointDelete.h>
#include <openvdb/points/IndexIterator.h>

#include <CH/CH_Channel.h>
#include <OP/OP_Expression.h>
#include <PRM/PRM_Parm.h>
#include <UT/UT_Ramp.h>

#include <tbb/mutex.h>

#include "ax/HoudiniAXUtils.h"

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


////////////////////////////////////////


class SOP_OpenVDB_AX: public hvdb::SOP_NodeVDB
{
public:

    SOP_OpenVDB_AX(OP_Network*, const char* name, OP_Operator*);
    ~SOP_OpenVDB_AX() override {}

    static OP_Node* factory(OP_Network*, const char* name, OP_Operator*);

    static int print(void* data, int index, float now, const PRM_Template*);

protected:
    OP_ERROR cookVDBSop(OP_Context&) override;
    virtual bool updateParmsFlags();

private:

    void evaluateChannelExpressions(ax::CustomData& data, const float contextTime);

    unsigned mHash;

    hax::TargetType mTargetType;
    CompilerCache mCompilerCache;

    // The current set of channel expressions.

    hax::ChannelExpressionSet mExpressionSet;
    std::vector<std::string> mWarnings;

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
        .setHelpText("A snippet of AX code that will manipulate the attributes on the VDB Points or the VDB voxel values.")
        .setSpareData(&PRM_SpareData::stringEditor));

    parms.add(hutil::ParmFactory(PRM_TOGGLE, "prune", "Prune")
        .setDefault(PRMoneDefaults)
        .setHelpText("Whether to prune VDBs after execution. Does not affect VDB Point Grids."));

    // parms.add(hutil::ParmFactory(PRM_CALLBACK, "print", "Print AST")
    //     .setCallbackFunc(&SOP_OpenVDB_AX::print)
    //     .setTooltip("Print the abstract syntax tree to the command line."));

    //////////
    // Register this operator.

    DnegVDBOpFactory("OpenVDB AX",
        SOP_OpenVDB_AX::factory, parms, *table)
        .addInput("VDBs to manipulate")
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
Note that the language is not as extensive as Houdini VEX and only supports a subset of similar functionality.\n\
\n\
:tip:\n\
All attributes are created if they do not exist.\n\
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
vector@v2 = { 5.0f, 5.0f, 10.3f }; // Create a new float vector attribute as in VEX \n\
}}}\n\
{{{\n\
#!vex\n\
vec3i@vid = { 3, -1, 10 }; // Create a new integer vector attribute\n\
}}}\n\
@functions Supported Functions\n\
#filtered: no\n\
\n\
Function ||\n\
    Description ||\n\
\n\
int abs(int), \n\
long abs(long) |\n\
    Computes the absolute value of an integer number.\n\
\n\
double acos(double), \n\
float acos(float) |\n\
    Computes the principal value of the arc cosine of the input.\n\
\n\
void addtogroup(string) |\n\
    Add the current point to the given group name, effectively setting its membership to true. \n\
    If the group does not exist, it is implicitly created. \n\
    This function has no effect if the point already belongs to the given group.\n\
\n\
double asin(double), \n\
float asin(float) |\n\
    Computes the principal value of the arc sine of the input.\n\
\n\
double atan(double), \n\
float atan(float) |\n\
    Computes the principal value of the arc tangent of the input.\n\
\n\
double atan2(double, double), \n\
float atan2(float, float) |\n\
    Computes the arc tangent of y/x using the signs of arguments to determine the correct quadrant.\n\
\n\
double atof(string) |\n\
    Parses the string input, interpreting its content as a floating point number and returns its value\n\
    as a double.\n\
\n\
int atoi(string), \n\
long atoi(string) |\n\
    Parses the string input, interpreting its content as a integral number and returns its value\n\
    of type int.\n\
\n\
double ch(string) |\n\
    Returns the value of the supplied scalar Houdini parameter channel.\n\
\n\
vector chv(string) |\n\
    Returns the value of the supplied vector Houdini parameter channel.\n\
\n\
double chramp(string, double),\n\
vec3d chramp(string, double) |\n\
    Samples the Houdini ramp parameter, for both scalar and vector ramps.\n\
\n\
bool ingroup(string) |\n\
    Returns whether or not the point being accessed is a member of the provided group.\n\
\n\
double sqrt(double) |\n\
    Return the square root of the argument.\n\
\n\
double sin(double) |\n\
    Returns the sine of the argument.\n\
\n\
double cos(double) |\n\
    Returns the cosine of the argument.\n\
\n\
double tan(double) |\n\
    Returns the tangent of the argument.\n\
\n\
double log(double) |\n\
    Returns the natural logarithm of the argument.\n\
\n\
double log10(double) |\n\
    Returns the logarithm (base 10) of the argument.\n\
\n\
double log2(double) |\n\
    Returns the logarithm (base 2) of the argument.\n\
\n\
double exp(double) |\n\
    Returns the exponential function of the argument.\n\
\n\
double exp2(double) |\n\
    Returns 2 raised to the power of the argument.\n\
\n\
double fabs(double) |\n\
    Returns the absolute value of the argument.\n\
\n\
double floor(double) |\n\
    Returns the largest integer less than or equal to the argument.\n\
\n\
double ceil(double) |\n\
    Returns the smallest integer greater than or equal to the argument.\n\
\n\
double pow(double, double) |\n\
    Raises the first argument to the power of the second argument.\n\
\n\
double round(double) |\n\
    Round to the closest integer.\n\
\n\
double min(double, double) |\n\
    Return the minimum of two values.\n\
\n\
double max(double, double) |\n\
    Return the maximum of two values.\n\
\n\
double clamp(double value, double min, double max) |\n\
    Returns value clamped between min and max.\n\
\n\
double fit(double value, double omin, double omax, double nmin, double nmax) |\n\
    Takes the value in the range (omin, omax) and shifts it to the corresponding value in the new range (nmin, nmax).\n\
\n\
double length(vec3d) |\n\
    Return the length (distance) of the vector.\n\
\n\
double lengthsq(vec3d) |\n\
    Returns the squared length (distance) of the vector.\n\
\n\
vec3d normalize(vec3d) |\n\
    Returns the normalized vector.\n\
\n\
double dot(vec3d, vec3d) |\n\
    Returns the dot product between the arguments.\n\
\n\
double dot(vec3d, vec3d) |\n\
    Returns the dot product between the arguments.\n\
\n\
double rand(double) |\n\
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


SOP_OpenVDB_AX::SOP_OpenVDB_AX(OP_Network* net,
        const char* name, OP_Operator* op)
    : hvdb::SOP_NodeVDB(net, name, op)
    , mHash(0)
    , mTargetType(hax::TargetType::LOCAL)
    , mCompilerCache()
    , mExpressionSet()
    , mWarnings()
{
    ax::initialize();

    ax::FunctionOptions functionOptions;
    ax::codegen::FunctionRegistry::UniquePtr functionRegistry =
        ax::codegen::createStandardRegistry(functionOptions);

    hax::registerCustomHoudiniFunctions(*functionRegistry, functionOptions);
    mCompilerCache.mCustomData.reset(new ax::CustomData);
    mCompilerCache.mCompiler = ax::Compiler::create();
    mCompilerCache.mCompiler->setFunctionRegistry(std::move(functionRegistry));
}

bool
SOP_OpenVDB_AX::updateParmsFlags()
{
    bool changed = false;
    const bool points = evalInt("targettype", 0, 0) == 0;
    changed |= enableParm("pointsgroup", points);
    changed |= enableParm("prune", !points);

    // used for internal debugging
    // changed |= setVisibleState("print", false);

    return changed;
}

int
SOP_OpenVDB_AX::print(
    void *data, int /*index*/, float /*now*/,
    const PRM_Template*)
{
    if (SOP_OpenVDB_AX* self = static_cast<SOP_OpenVDB_AX*>(data)) {
        if (self->mCompilerCache.mSyntaxTree) {
            ax::ast::print(*(self->mCompilerCache.mSyntaxTree), std::cerr);
        }
        return 1;
    }
    return 0;
}


////////////////////////////////////////


struct PruneOp {
    template<typename GridT>
    void operator()(GridT& grid) const {
        tools::prune(grid.tree());
    }
};

OP_ERROR
SOP_OpenVDB_AX::cookVDBSop(OP_Context& context)
{
    try {
        hutil::ScopedInputLock lock(*this, context);

        if (duplicateSourceStealable(0, context) >= UT_ERROR_ABORT) return error();
        if (gdp == nullptr) return error();

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

        const int targetInt = evalInt("targettype", 0, time);
        const hax::TargetType targetType =
            static_cast<hax::TargetType>(targetInt);

        // get the expression from the snippet field
        // @TODO use parameter update notifications to query if the snippet
        // has changed rather than hashing the code

        UT_String snippet;
        evalString(snippet, "snippet", 0, time);
        if (snippet.length() == 0) return error();

        const unsigned hash = snippet.hash();

        if (hash != mHash || targetType != mTargetType) {

            // Empty the hash - if there are any compiler failures, the hash won't be
            // initialized but the engine data maybe modified. If the code is then changed
            // back to the previous hash, this path will not be correctly executed
            // without this explicit change
            mHash = 0;
            mWarnings.clear();
            mExpressionSet.clear();

            mCompilerCache.mSyntaxTree = ax::ast::parse(snippet.nonNullBuffer());

            // The type of ramp being used within ramp expressions can not be
            // inferred by the compiler and must be injected into the engine data
            // prior to compilation

            hax::findChannelExpressions(*mCompilerCache.mSyntaxTree, mExpressionSet);

            // convert from Houdini VEX

            hax::convertASTFromVEX(*mCompilerCache.mSyntaxTree, targetType);

            // clear all custom data - this currently clears attribute and group
            // mappings too

            mCompilerCache.mCustomData->reset();
            evaluateChannelExpressions(*mCompilerCache.mCustomData, time);

            if (targetType == hax::TargetType::POINTS) {

                mCompilerCache.mRequiresDeletion =
                    openvdb::ax::ast::callsFunction(*mCompilerCache.mSyntaxTree, "deletepoint");

                mCompilerCache.mPointExecutable =
                    mCompilerCache.mCompiler->compile<ax::PointExecutable>
                        (*mCompilerCache.mSyntaxTree, mCompilerCache.mCustomData, &mWarnings);
            }
            else if (targetType == hax::TargetType::VOLUMES) {
                mCompilerCache.mVolumeExecutable =
                    mCompilerCache.mCompiler->compile<ax::VolumeExecutable>
                        (*mCompilerCache.mSyntaxTree, mCompilerCache.mCustomData, &mWarnings);
            }

            // set the hash only if compilation was successful - Houdini sops tend to cook
            // multiple times, especially on fail. If we assign the hash prior to this it will
            // be incorrectly cached

            mHash = hash;
            mTargetType = targetType;
        }
        else {
            evaluateChannelExpressions(*mCompilerCache.mCustomData, time);
        }

        snippet.clear();

        for (const std::string& warning : mWarnings) {
            addWarning(SOP_MESSAGE, warning.c_str());
        }

        if (targetType == hax::TargetType::POINTS) {
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

                if (mCompilerCache.mRequiresDeletion) {
                    openvdb::points::deleteFromGroup(points->tree(), "dead", false, false);
                }
            }
        }
        else if (targetType == hax::TargetType::VOLUMES) {

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

void SOP_OpenVDB_AX::evaluateChannelExpressions(ax::CustomData& data,
                                                const float contextTime)
{
    // evaluate channel expressions.

    using VectorData = TypedMetadata<math::Vec3<float>>;
    using FloatData = TypedMetadata<float>;
    using FloatRampData = hax::RampDataCache<float>;
    using VectorRampData = hax::RampDataCache<math::Vec3<float>>;

    for (const hax::ChannelExpressionPair& expresionPair : mExpressionSet) {

        const hax::ChannelTokens token = expresionPair.first;
        const std::string& channelPath = expresionPair.second;
        if (channelPath.empty()) continue;

        int index(0);
        int subIndex(0);
        OP_Node* node(nullptr);

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

        const bool lookForChannel = (token != hax::ChannelTokens::CHRAMP);

        // findParmRelativeTo finds the node and parameter index on the node which is
        // related to the channelPath relative to this node
        // @note Do NOT use OPgetParameter() directly as this seems to cause crashes
        // when used with various DOP networks

        // @todo: cache channelFinder?

        OP_ExprFindCh channelFinder;
        bool validPath =
            channelFinder.findParmRelativeTo(*this,
                                             channelPath.c_str(),
                                             contextTime,
                                             node,            /*the node holding the param*/
                                             index,           /*parm index on the node*/
                                             subIndex,        /*sub index of parm if not channel*/
                                             lookForChannel); /*is_for_channel_name*/

        // if no channel found and we're using CHV, try looking for the parm directly

        if (!validPath && token == hax::ChannelTokens::CHV) {
            validPath =
                channelFinder.findParmRelativeTo(*this,
                                                 channelPath.c_str(),
                                                 contextTime,
                                                 node,       /*the node holding the param*/
                                                 index,      /*parm index on the node*/
                                                 subIndex,   /*sub index of parm if not channel*/
                                                 false);     /*is_for_channel_name*/
        }

        if (validPath) {

            assert(node);

            if (token == hax::ChannelTokens::CHV) {
                VectorData::Ptr vecData(new VectorData());

                Vec3f value;

                if (subIndex != -1) {
                    // parm was a channel
                    value = openvdb::Vec3f(node->evalFloat(index, subIndex, contextTime));
                }
                else {
                    // parm was a direct parm
                    value[0] = node->evalFloat(index, 0, contextTime);
                    value[1] = node->evalFloat(index, 1, contextTime);
                    value[2] = node->evalFloat(index, 2, contextTime);
                }

                vecData->setValue(value);

                // add an extra input to all the relevant channels of this
                // parameter by passing subindex
                addExtraInput(*node, index, subIndex);

                data.insertData<VectorData>(channelPath, vecData);
            }
            else if (token == hax::ChannelTokens::CH) {
                // use evalFloat rather than parm->getValue() to wrap the conversion to a float

                assert(subIndex != -1);
                const float value = node->evalFloat(index, subIndex, contextTime);

                FloatData::Ptr floatData(new FloatData(value));
                data.insertData(channelPath, floatData);

                addExtraInput(*node, index, subIndex);
            }
            else if (token == hax::ChannelTokens::CHRAMP) {

                PRM_Parm& parm = node->getParm(index);
                const bool isRamp = parm.isRampType();

                if (!isRamp) {
                    const std::string message =
                        "Invalid channel reference: " + channelPath + ". Parameter is not a ramp.";
                    addWarning(SOP_MESSAGE, message.c_str());
                    FloatRampData::Ptr floatRampData(new FloatRampData());
                    data.insertData(channelPath, floatRampData);
                    continue;
                }

                UT_Ramp houdiniRamp;
                node->updateRampFromMultiParm(contextTime, parm, houdiniRamp);
                addMultiparmInterests(node, parm);

                const int numNodes = houdiniRamp.getNodeCount();
                const bool isVectorRamp = parm.isRampTypeColor();
                if (!isVectorRamp) {
                    // must be a float ramp

                    FloatRampData::Ptr floatRampData(new FloatRampData());

                    for (int i = 0; i < numNodes; ++i) {
                        const UT_ColorNode* const rampNode = houdiniRamp.getNode(i);
                        floatRampData->insertRampPoint(rampNode->t, rampNode->rgba.r);
                    }

                    data.insertData(channelPath, floatRampData);
                }
                else {
                    VectorRampData::Ptr vectorRampData(new VectorRampData());

                    for (int i = 0; i < numNodes; ++i) {
                        const UT_ColorNode* const rampNode = houdiniRamp.getNode(i);
                        vectorRampData->insertRampPoint(rampNode->t,
                            Vec3R(rampNode->rgba.r, rampNode->rgba.g, rampNode->rgba.b));
                    }

                    data.insertData(channelPath, vectorRampData);
                }

                // add an extra input to all the channels of this parameter by passing
                // subindex as -1
                addExtraInput(*node, index, /*subindex*/-1);
            }
        }
        else {

            if (token == hax::ChannelTokens::CHV) {
                VectorData::Ptr vecData(new VectorData(openvdb::Vec3f::zero()));
                data.insertData<VectorData>(channelPath, vecData);
            }
            else if (token == hax::ChannelTokens::CH) {
                FloatData::Ptr floatData(new FloatData(0.0f));
                data.insertData<FloatData>(channelPath, floatData);
            }
            else if (token == hax::ChannelTokens::CHRAMP) {
                FloatRampData::Ptr floatRampData(new FloatRampData());
                data.insertData<FloatRampData>(channelPath, floatRampData);
            }

            const std::string message = "Invalid channel reference: " + channelPath;
            addWarning(SOP_MESSAGE, message.c_str());
        }
    }
}

////////////////////////////////////////

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
