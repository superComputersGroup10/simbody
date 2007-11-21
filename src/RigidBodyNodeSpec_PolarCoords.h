#ifndef SimTK_SIMBODY_RIGID_BODY_NODE_SPEC_POLARCOORDS_H_
#define SimTK_SIMBODY_RIGID_BODY_NODE_SPEC_POLARCOORDS_H_

/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2005-9 Stanford University and the Authors.         *
 * Authors: Michael Sherman                                                   *
 * Contributors:                                                              *
 *    Charles Schwieters (NIH): wrote the public domain IVM code from which   *
 *                              this was derived.                             *
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


/**@file
 * Define the RigidBodyNode that implements a PolarCoords mobilizer, also known as
 * a BendStretch joint.
 */

#include "SimbodyMatterSubsystemRep.h"
#include "RigidBodyNode.h"
#include "RigidBodyNodeSpec.h"


    // BEND-STRETCH //

// This is a "bend-stretch" joint, meaning one degree of rotational freedom
// about a particular axis, and one degree of translational freedom
// along a perpendicular axis. The z axis of the parent's F frame is 
// used for rotation (and that is always aligned with the M frame z axis).
// The x axis of the *M* frame is used for translation; that is, first
// we rotate around z, which moves M's x with respect to F's x. Then
// we slide along the rotated x axis. The two
// generalized coordinates are the rotation and the translation, in that order.
class RBNodeBendStretch : public RigidBodyNodeSpec<2> {
public:
    virtual const char* type() { return "bendstretch"; }

    RBNodeBendStretch(const MassProperties& mProps_B,
                      const Transform&      X_PF,
                      const Transform&      X_BM,
                      bool                  isReversed,
                      UIndex&               nextUSlot,
                      USquaredIndex&        nextUSqSlot,
                      QIndex&               nextQSlot)
      : RigidBodyNodeSpec<2>(mProps_B,X_PF,X_BM,nextUSlot,nextUSqSlot,nextQSlot,
                             QDotIsAlwaysTheSameAsU, QuaternionIsNeverUsed, isReversed)
    {
        updateSlots(nextUSlot,nextUSqSlot,nextQSlot);
    }


    void setQToFitRotationImpl(const SBStateDigest& sbs, const Rotation& R_FM, Vector& q) const {
        // The only rotation our bend-stretch joint can handle is about z.
        // TODO: this code is bad -- see comments for Torsion joint above.
        const Vec3 angles123 = R_FM.convertRotationToBodyFixedXYZ();
        toQ(q)[0] = angles123[2];
    }

    void setQToFitTranslationImpl(const SBStateDigest& sbs, const Vec3& p_FM, Vector& q) const {
        // We can represent any translation that puts the M origin in the x-y plane of F,
        // by a suitable rotation around z followed by translation along x.
        const Vec2 r = p_FM.getSubVec<2>(0); // (rx, ry)

        // If we're not allowed to change rotation then we can only move along Mx.
//        if (only) {
//            const Real angle = fromQ(q)[0];
//            const Vec2 Mx(std::cos(angle), std::sin(angle)); // a unit vector
//            toQ(q)[1] = dot(r,Mx);
//            return;
//        }

        const Real d = r.norm();

        // If there is no translation worth mentioning, we'll leave the rotational
        // coordinate alone, otherwise rotate so M's x axis is aligned with r.
        if (d >= 4*Eps) {
            const Real angle = std::atan2(r[1],r[0]);
            toQ(q)[0] = angle;
            toQ(q)[1] = d;
        } else
            toQ(q)[1] = 0;
    }

    void setUToFitAngularVelocityImpl(const SBStateDigest& sbs, const Vector& q, const Vec3& w_FM, Vector& u) const {
        // We can only represent an angular velocity along z with this joint.
        toU(u)[0] = w_FM[2];
    }

    // If the translational coordinate is zero, we can only represent a linear velocity 
    // of OM in F which is along M's current x axis direction. Otherwise, we can 
    // represent any velocity in the x-y plane by introducing angular velocity about z.
    // We can never represent a linear velocity along z.
    void setUToFitLinearVelocityImpl
       (const SBStateDigest& sbs, const Vector& q, const Vec3& v_FM, Vector& u) const
    {
        // Decompose the requested v into "along Mx" and "along My" components.
        const Rotation R_FM = Rotation( fromQ(q)[0], ZAxis ); // =[ Mx My Mz ] in F
        const Vec3 v_FM_M = ~R_FM*v_FM; // re-express in M frame

        toU(u)[1] = v_FM_M[0]; // velocity along Mx we can represent directly

//        if (only) {
//            // We can't do anything about My velocity if we're not allowed to change
//            // angular velocity, so we're done.
//            return;
//        }

        const Real x = fromQ(q)[1]; // translation along Mx (signed)
        if (std::abs(x) < SignificantReal) {
            // No translation worth mentioning; we can only do x velocity, which we just set above.
            return;
        }

        // significant translation
        toU(u)[0] = v_FM_M[1] / x; // set angular velocity about z to produce vy
    }

    // This is required for all mobilizers.
    bool isUsingAngles(const SBStateDigest& sbs, MobilizerQIndex& startOfAngles, int& nAngles) const {
        // Bend-stretch joint has one angular coordinate, which comes first.
        startOfAngles = MobilizerQIndex(0); nAngles=1; 
        return true;
    }

    // Precalculate sines and cosines.
    void calcJointSinCosQNorm(
        const SBModelVars&  mv,
        const SBModelCache& mc,
        const SBInstanceCache& ic,
        const Vector&       q, 
        Vector&             sine, 
        Vector&             cosine, 
        Vector&             qErr,
        Vector&             qnorm) const
    {
        const Real& angle = fromQ(q)[0];
        toQ(sine)[0]    = std::sin(angle);
        toQ(cosine)[0]  = std::cos(angle);
        // no quaternions
    }

    // Calculate X_FM.
    void calcAcrossJointTransform(
        const SBStateDigest& sbs,
        const Vector&        q,
        Transform&           X_F0M0) const
    {
        const Vec2& coords  = fromQ(q);    // angular coordinate

        X_F0M0.updR().setRotationFromAngleAboutZ(coords[0]);
        X_F0M0.updP() = X_F0M0.R()*Vec3(coords[1],0,0); // because translation is in M frame
    }

    // The generalized speeds for this bend-stretch joint are (1) the angular
    // velocity of M in the F frame, about F's z axis, expressed in F, and
    // (2) the (linear) velocity of M's origin in F, along *M's* current x axis
    // (that is, after rotation about z). (The z axis is also constant in M for this joint.)
    void calcAcrossJointVelocityJacobian(
        const SBStateDigest& sbs,
        HType&               H_FM) const
    {
        const SBTreePositionCache& pc = sbs.updTreePositionCache(); // use "upd" because we're realizing positions now
        const Transform X_F0M0 = findX_F0M0(pc);
        const Rotation& R_F0M0 = X_F0M0.R();

        // Dropping the 0's here.
        const Vec3&     p_FM = X_F0M0.p();
        const Vec3&     Mx_F = R_F0M0.x(); // M's x axis, expressed in F

        H_FM(0) = SpatialVec( Vec3(0,0,1), Vec3(0,0,1) % p_FM   );
        H_FM(1) = SpatialVec( Vec3(0),            Mx_F          );
    }

    // Since the the Jacobian above is not constant in F,
    // its time derivative is non zero. Here we use the fact that for
    // a vector r_B_A fixed in a moving frame B but expressed in another frame A,
    // its time derivative in A is the angular velocity of B in A crossed with
    // the vector, i.e., d_A/dt r_B_A = w_AB % r_B_A.
    void calcAcrossJointVelocityJacobianDot(
        const SBStateDigest& sbs,
        HType&               HDot_FM) const
    {
        const SBTreePositionCache& pc = sbs.getTreePositionCache();
        const SBTreeVelocityCache& vc = sbs.updTreeVelocityCache(); // use "upd" because we're realizing velocities now

        const Transform  X_F0M0 = findX_F0M0(pc);
        const Rotation&  R_F0M0 = X_F0M0.R();
        const SpatialVec V_F0M0 = findV_F0M0(pc,vc);

        // Dropping the 0's here.
        const Vec3&     Mx_F = R_F0M0.x(); // M's x axis, expressed in F
        const Vec3&     w_FM = V_F0M0[0]; // angular velocity of M in F
        const Vec3&     v_FM = V_F0M0[1]; // linear velocity of OM in F

        HDot_FM(0) = SpatialVec( Vec3(0), Vec3(0,0,1) % v_FM );
        HDot_FM(1) = SpatialVec( Vec3(0),    w_FM % Mx_F );
    }

};


#endif // SimTK_SIMBODY_RIGID_BODY_NODE_SPEC_POLARCOORDS_H_
