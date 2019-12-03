#ifndef OPENVDB_AX_CODEGEN_OPEN_SIMPLEX_NOISE_HAS_BEEN_INCLUDED
#define OPENVDB_AX_CODEGEN_OPEN_SIMPLEX_NOISE_HAS_BEEN_INCLUDED

// This code is based on https://gist.github.com/tombsar/716134ec71d1b8c1b530 (accessed on
// 22/05/2019).  We have simplified that code in a number of ways, most notably by removing the
// template on dimension (this only generates 3 dimensional noise) and removing the base class
// as it's unnecessary for our uses.  We also assume C++ 2011 or above and have thus removed a
// number of #ifdef blocks.

// The following is the original copyright notice:
/*
 *
 *
 * OpenSimplex (Simplectic) Noise in C++
 * by Arthur Tombs
 *
 * Modified 2015-01-08
 *
 * This is a derivative work based on OpenSimplex by Kurt Spencer:
 *   https://gist.github.com/KdotJPG/b1270127455a94ac5d19
 *
 * Anyone is free to make use of this software in whatever way they want.
 * Attribution is appreciated, but not required.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cstdint>

namespace OSN {

using inttype = int64_t;

// 3D Implementation of the OpenSimplexNoise generator.
class OSNoise {

public:

    // Initializes the class using a permutation array generated from a 64-bit seed.
    // Generates a proper permutation (i.e. doesn't merely perform N successive
    // pair swaps on a base array).
    // Uses a simple 64-bit LCG.
    OSNoise(inttype seed = 0LL);

    OSNoise(const int * p);

    template <typename T>
    T eval(const T x, const T y, const T z) const;

private:

    template <typename T>
    inline T extrapolate(const inttype xsb,
                         const inttype ysb,
                         const inttype zsb,
                         const T dx,
                         const T dy,
                         const T dz) const;

    template <typename T>
    inline T extrapolate(const inttype xsb,
                         const inttype ysb,
                         const inttype zsb,
                         const T dx,
                         const T dy,
                         const T dz,
                         T (&de) [3]) const;

    int mPerm [256];
    // Array of gradient values for 3D. Values are defined below the class definition.
    static const int sGradients [72];

    // Because 72 is not a power of two, extrapolate cannot use a bitmask to index
    // into the perm array. Pre-calculate and store the indices instead.
    int mPermGradIndex [256];
};

}
#endif // OPENVDB_AX_CODEGEN_OPEN_SIMPLEX_NOISE_HAS_BEEN_INCLUDED
