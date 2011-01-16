#ifndef SimTK_SIMBODY_DECORATION_GENERATOR_H_
#define SimTK_SIMBODY_DECORATION_GENERATOR_H_

/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2010 Stanford University and the Authors.           *
 * Authors: Peter Eastman                                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "SimTKcommon.h"
#include "simbody/internal/common.h"

namespace SimTK {

class DecorativeGeometry;

/**
 * A DecorationGenerator is used to define geometry that may change over the course of
 * a simulation.  Example include
 *
 * <ul>
 * <li>Geometry whose position is not fixed relative to any single body.</li>
 * <li>Geometry which may appear or disappear during the simulation.</li>
 * <li>Geometry whose properties (color, size, etc.) may change during the simulation.</li>
 * </ul>
 *
 * To use it, define a concrete subclass that implements generateDecorations() to generate
 * whatever geometry is appropriate for a given State.  It can then be added to a
 * DecorationSubsystem, or directly to a Visualizer.
 */
class SimTK_SIMBODY_EXPORT DecorationGenerator {
public:
    /**
     * This will be called every time a new State is about to be visualized.  It should generate
     * whatever decorations are appropriate for the State and append them to the array.
     */
    virtual void generateDecorations(const State& state, Array_<DecorativeGeometry>& geometry) = 0;

    /** Destructor is virtual; be sure to override it if you have something
    to clean up at the end. **/
    virtual ~DecorationGenerator() {}
};

} // namespace SimTK

#endif // SimTK_SIMBODY_DECORATION_GENERATOR_H_