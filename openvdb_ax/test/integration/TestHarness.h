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
// *     Neither the name of DNEG Visual Effects nor the names
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

#ifndef OPENVDB_POINTS_UNITTEST_TEST_HARNESS_INCLUDED
#define OPENVDB_POINTS_UNITTEST_TEST_HARNESS_INCLUDED

#include "CompareGrids.h"

#include <openvdb_ax/compiler/Compiler.h>
#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/compiler/PointExecutable.h>
#include <openvdb_ax/compiler/VolumeExecutable.h>

#include <openvdb/points/PointAttribute.h>

#include <cppunit/TestCase.h>

#include <unordered_map>

/// @file unittest/TestHarness.h
///
/// @authors Francisco Gochez
///
/// @brief Structure for wrapping up most of the existing integration tests with a simple interface

namespace unittest_util
{

std::string loadText(const std::string& codeFileName);

void wrapExecution(openvdb::points::PointDataGrid& grid,
                   const std::string& codeFileName,
                   const std::string * const group = nullptr,
                   std::vector<std::string>* warnings = nullptr,
                   const openvdb::ax::CustomData::Ptr& data =
                      openvdb::ax::CustomData::create());

void wrapExecution(openvdb::GridPtrVec& grids,
                   const std::string& codeFileName,
                   std::vector<std::string>* warnings = nullptr,
                   const openvdb::ax::CustomData::Ptr& data =
                        openvdb::ax::CustomData::create());

struct AXTestHarness
{
    AXTestHarness() :
        mInputPointGrids()
      , mOutputPointGrids()
      , mInputVolumeGrids()
      , mOutputVolumeGrids()
      , mUseVolumes(true)
      , mUsePoints(true)
    {
        reset();
    }

    void addInputGroups(const std::vector<std::string>& names, const std::vector<bool>& defaults);
    void addExpectedGroups(const std::vector<std::string>& names, const std::vector<bool>& defaults);

    /// @brief adds attributes to both to input data set
    template <typename T>
    void addInputAttributes(const std::vector<std::string>& names,
                       const std::vector<T>& values)
    {
        if (mUsePoints) {
            addInputPtAttributes<T>(names, values);
        }

        if (mUseVolumes) {
            addInputVolumes(names, values);
        }
    }

    /// @brief override of addInputAttributes for strings which won't add to volumes
    ///        necessary since there are no "string volumes"
    void addInputAttributes(const std::vector<std::string>& names,
                            const std::vector<std::string>& values)
    {
        if (mUsePoints) {
            addInputPtAttributes<std::string>(names, values);
        }
    }

    /// @brief override of addInputAttributes for Mat4s which won't add to volumes
    ///        necessary since there are no matrix volumes
    void addInputAttributes(const std::vector<std::string>& names,
                            const std::vector<openvdb::math::Mat4s>& values)
    {
        if (mUsePoints) {
            addInputPtAttributes<openvdb::math::Mat4s>(names, values);
        }
    }

    /// @brief adds attributes to both to expected outptu data sets
    template <typename T>
    void addExpectedAttributes(const std::vector<std::string>& names,
                       const std::vector<T>& values)
    {
        if (mUsePoints) {
            addExpectedPtAttributes<T>(names, values);
        }

        if (mUseVolumes) {
            addExpectedVolumes<T>(names, values);
        }
    }

    /// @brief adds attributes to both to expected outptu data sets
    void addExpectedAttributes(const std::vector<std::string>& names,
                               const std::vector<std::string>& values)
    {
        addExpectedPtAttributes<std::string>(names, values);
    }

    /// @brief adds attributes to both to expected outptu data sets
    void addExpectedAttributes(const std::vector<std::string>& names,
                                const std::vector<openvdb::math::Mat4s>& values)
    {
        addExpectedPtAttributes<openvdb::math::Mat4s>(names, values);
    }

    /// @brief adds attributes to both to expected outptu data sets
    void addExpectedAttributes(const std::vector<std::string>& names,
                               const std::vector<short>& values)
    {
        addExpectedPtAttributes<short>(names, values);
    }

    /// @brief adds attributes to both input and expected data
    template <typename T>
    void addAttributes(const std::vector<std::string>& names,
                       const std::vector<T>& inputValues,
                       const std::vector<T>& expectedValues)
    {
       addInputAttributes(names, inputValues);
       addExpectedAttributes(names, expectedValues);
    }

    /// @brief adds attributes to both input and expected data, with input data set to 0 values
    template <typename T>
    void addAttributes(const std::vector<std::string>& names,
                       const std::vector<T>& expectedValues)
    {
       std::vector<T> zeroVals(expectedValues.size(), openvdb::zeroVal<T>());
       addAttributes(names, zeroVals, expectedValues);
    }

    template <typename T>
    void addAttribute(const std::string& name, const T& inVal, const T& expVal)
    {
        addAttributes<T>({name}, {inVal}, {expVal});
    }

    template <typename T>
    void addAttribute(const std::string& name, const T& expVal)
    {
        addAttribute<T>(name, openvdb::zeroVal<T>(), expVal);
    }

    template <typename T>
    void addExpectedAttribute(const std::string& name, const T& expVal)
    {
        addExpectedAttributes<T>({name}, {expVal});
    }

    /// @brief excecutes a snippet of code contained in a file to the input data sets
    void executeCode(const std::string& codeFile,
                     const std::string * const group = nullptr,
                     std::vector<std::string>* warnings = nullptr,
                     const openvdb::ax::CustomData::Ptr& data =
                        openvdb::ax::CustomData::create());

    /// @brief rebuilds the input and output data sets to their "default" states
    void reset();

    /// @brief compares the input and expected point grids and outputs a report of differences to
    /// the provided stream
    bool checkAgainstExpected(std::ostream& sstream);

    void testVolumes(const bool);
    void testPoints(const bool);

    std::unordered_map<std::string, openvdb::points::PointDataGrid::Ptr> mInputPointGrids;
    std::unordered_map<std::string, openvdb::points::PointDataGrid::Ptr> mOutputPointGrids;

    std::unordered_map<std::string, openvdb::GridPtrVec> mInputVolumeGrids;
    std::unordered_map<std::string, openvdb::GridPtrVec> mOutputVolumeGrids;

    bool mUseVolumes;
    bool mUsePoints;

private:
    template <typename T>
    void addInputPtAttributes(const std::vector<std::string>& names,
                              const std::vector<T>& values)
    {
        for (size_t i = 0; i < names.size(); i++) {
            for (auto& grid : mInputPointGrids) {
                openvdb::points::appendAttribute<T>(grid.second->tree(), names[i], values[i]);
           }
        }
    }

    template <typename T>
    void addInputVolumes(const std::vector<std::string>& names,
                         const std::vector<T>& values)
    {
        using GridType = typename openvdb::BoolGrid::ValueConverter<T>::Type;
        if (!GridType::isRegistered()) {
            throw std::runtime_error("Attempted to insert a volume of type \"" +
                std::string(openvdb::typeNameAsString<T>()) + "\" which is not registered.");
        }

        for (size_t i = 0; i < names.size(); i++) {
            typename GridType::Ptr grid = GridType::create();
            grid->tree().setValue(openvdb::Coord(0, 0, 0), values[i]);
            grid->setName(names[i]);
            mInputVolumeGrids["one_voxel"].emplace_back(grid);
        }
    }

    template <typename T>
    void addExpectedPtAttributes(const std::vector<std::string>& names,
                                 const std::vector<T>& values)
    {
        for (size_t i = 0; i < names.size(); i++) {
            for (auto& grid : mOutputPointGrids) {
                openvdb::points::appendAttribute<T>(grid.second->tree(), names[i], values[i]);
           }
        }
    }

    template <typename T>
    void addExpectedVolumes(const std::vector<std::string>& names,
                            const std::vector<T>& values)
    {
        using GridType = typename openvdb::BoolGrid::ValueConverter<T>::Type;
        if (!GridType::isRegistered()) {
            throw std::runtime_error("Attempted to insert a volume of type \"" +
                std::string(openvdb::typeNameAsString<T>()) + "\" which is not registered.");
        }

        for (size_t i = 0; i < names.size(); i++) {
            typename GridType::Ptr grid = GridType::create();
            grid->tree().setValue(openvdb::Coord(0, 0, 0), values[i]);
            grid->setName(names[i] + "_expected");
            mOutputVolumeGrids["one_voxel"].emplace_back(grid);
        }
    }

};

class AXTestCase : public CppUnit::TestCase
{
public:
    void setUp() override;

protected:
    AXTestHarness mHarness;
};

} // namespace unittest_util

#define AXTESTS_STANDARD_ASSERT_HARNESS(harness) \
    {   std::stringstream out; \
        const bool correct = harness.checkAgainstExpected(out); \
        CPPUNIT_ASSERT_MESSAGE(out.str(), correct); }

#define AXTESTS_STANDARD_ASSERT() \
      AXTESTS_STANDARD_ASSERT_HARNESS(mHarness);

#endif // OPENVDB_POINTS_UNITTEST_TEST_HARNESS_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
