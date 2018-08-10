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

#include "Functions.h"

#include <boost/functional/hash.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>

#include <tbb/enumerable_thread_specific.h>

#include <stdint.h>
#include <stddef.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

namespace
{

// Reduce a size_t hash down into an unsigned int, taking all bits in the size_t into account. We
// achieve this by repeatedly XORing as many bytes that fit into an unsigned int, and then shift those
// bytes out of the hash. We repeat until we have no bits left in the hash.
template <typename SeedType>
inline SeedType hashToSeed(std::size_t hash) {
    SeedType seed = 0;
    do {
        seed ^= (SeedType) hash;
    } while (hash >>= sizeof(SeedType) * 8);
    return seed;
}

}

double Rand::rand_double(double seed){
    using ThreadLocalEngineContainer = tbb::enumerable_thread_specific<boost::mt19937>;

    // Obtain thread-local engine (or create if it doesn't exist already).

    static ThreadLocalEngineContainer ThreadLocalEngines;
    static boost::uniform_01<double> Generator;

    // We initially hash the double-precision seed with `boost::hash`. The important thing about the
    // hash is that it produces a "reliable" hash value, taking into account a number of special cases
    // for floating point numbers (e.g. -0 and +0 must return the same hash value, etc). Other than
    // these special cases, this function will usually just copy the binary representation of a float
    // into the resultant `size_t`
    std::size_t hash = boost::hash<double>()(seed);

    // Now that we have a reliable hash (with special floating-point cases taken care of), we proceed
    // to use this hash to seed a random number generator. The generator takes an unsigned int, which
    // is not guaranteed to be the same size as size_t.
    //
    // So, we must convert it. I should note that the OpenVDB math libraries will do this for us, but
    // its implementation static_casts `size_t` to `unsigned int`, and because `boost::hash` returns
    // a binary copy of the original double-precision number in almost all cases, this ends up producing
    // noticable patterns in the result (e.g. by truncating the upper 4 bytes, values of 1.0, 2.0, 3.0,
    // and 4.0 all return the same hash value because their lower 4 bytes are all zero).
    //
    // We use the `hashToSeed` function to reduce our `size_t` to an `unsigned int`, whilst taking all
    // bits in the `size_t` into account.
    boost::mt19937& engine = ThreadLocalEngines.local();
    engine.seed(static_cast<boost::mt19937::result_type>(hashToSeed<unsigned int>(hash)));

    // Once we have seeded the random number generator, we then evaluate it, which returns a floating
    // point number in the range [0,1)
    return Generator(engine);
}

double Rand::rand_int(int32_t seed) {

    using ThreadLocalEngineContainer = tbb::enumerable_thread_specific<boost::mt19937>;

    static ThreadLocalEngineContainer ThreadLocalEngines;
    static boost::uniform_01<double> Generator;

    boost::mt19937& engine = ThreadLocalEngines.local();
    engine.seed(static_cast<uint32_t>(seed));

    return Generator(engine);
}

} // namespace codegen
} // namespace ax
} // namespace openvdb_version
} // namespace openvdb


// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )

