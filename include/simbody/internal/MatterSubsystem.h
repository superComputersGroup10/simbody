#ifndef SimTK_MATTER_SUBSYSTEM_H_
#define SimTK_MATTER_SUBSYSTEM_H_

/* Portions copyright (c) 2005-6 Stanford University and Michael Sherman.
 * Contributors:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "SimTKcommon.h"
#include "simbody/internal/common.h"
#include "simbody/internal/State.h"
#include "simbody/internal/Subsystem.h"

namespace SimTK {

/// The still-abstract parent of all MatterSubsystems (such as the
/// one generated by Simbody). This is derived from Subsystem.
class SimTK_SIMBODY_EXPORT MatterSubsystem : public Subsystem {
public:
    MatterSubsystem() { }

    // Topological information (no state)
    int getNBodies()      const;    // includes ground, also # mobilizers+1
    int getNParticles()   const;
    int getNMobilities()  const;

    int getNConstraints() const;    // i.e., Constraint definitions (each is multiple equations)

    int        getParent  (int bodyNum) const;
    Array<int> getChildren(int bodyNum) const;

    const Transform&  getMobilizerFrame(const State&, int bodyNum) const;
    const Transform&  getMobilizerFrameOnParent(const State&, int bodyNum) const;

    const Real& getBodyMass(const State&, int bodyNum) const;
    const Vec3& getBodyCenterOfMassStation(const State&, int bodyNum) const;

    const Vector&        getParticleMasses(const State&)      const;
    const Vector_<Vec3>& getParticleLocations(const State& s) const; 


    /// This can be called at any time after construction. It sizes a set of
    /// force arrays (if necessary) and then sets them to zero. The "addIn"
    /// operators below can then be used to accumulate forces.
    void resetForces(Vector_<SpatialVec>& bodyForces,
                     Vector_<Vec3>&       particleForces,
                     Vector&              mobilityForces) const 
    {
        bodyForces.resize(getNBodies());         bodyForces.setToZero();
        particleForces.resize(getNParticles());  particleForces.setToZero();
        mobilityForces.resize(getNMobilities()); mobilityForces.setToZero();
    }

    /// Apply a force to a point on a body (a station). Provide the
    /// station in the body frame, force in the ground frame. Must
    /// be realized to Position stage prior to call.
    void addInStationForce(const State&, int body, const Vec3& stationInB, 
                           const Vec3& forceInG, Vector_<SpatialVec>& bodyForces) const;

    /// Apply a torque to a body. Provide the torque vector in the
    /// ground frame.
    void addInBodyTorque(const State&, int body, const Vec3& torqueInG, 
                         Vector_<SpatialVec>& bodyForces) const;

    /// Apply a scalar joint force or torque to an axis of the
    /// indicated body's inboard joint.
    void addInMobilityForce(const State&, int body, int axis, const Real& f,
                            Vector& mobilityForces) const;

    // Kinematic information.

    /// Extract from the state cache the already-calculated spatial configuration of
    /// body B's body frame, measured with respect to the ground frame and expressed
    /// in the ground frame. That is, we return the location of the body frame's
    /// origin, and the orientation of its x, y, and z axes, as the transform X_GB.
    /// This response is available at Position stage.
    const Transform& getBodyPosition(const State&, int body) const;

    /// Extract from the state cache the already-calculated spatial orientation
    /// of body B's body frame x, y, and z axes expressed in the ground frame,
    /// as the rotation matrix R_GB. This response is available at Position stage.
    const Rotation& getBodyRotation(const State& s, int body) const {
        return getBodyPosition(s,body).R();
    }
    /// Extract from the state cache the already-calculated spatial location
    /// of body B's body frame origin, measured from the ground origin and
    /// expressed in the ground frame, as the translation vector T_GB.
    /// This response is available at Position stage.
    const Vec3& getBodyLocation(const State& s, int body) const {
        return getBodyPosition(s,body).T();
    }

    /// Extract from the state cache the already-calculated spatial velocity of
    /// body B's body frame, measured with respect to the ground frame and expressed
    /// in the ground frame. That is, we return the linear velocity v_GB of the body
    /// frame's origin, and the body's angular velocity w_GB as the spatial velocity
    /// vector V_GB = {w_GB, v_GB}. This response is available at Velocity stage.
    const SpatialVec& getBodyVelocity(const State&, int body) const;

    /// Extract from the state cache the already-calculated inertial angular
    /// velocity vector w_GB of body B, measured with respect to the ground frame
    /// and expressed in the ground frame. This response is available at Velocity stage.
    const Vec3& getBodyAngularVelocity(const State& s, int body) const {
        return getBodyVelocity(s,body)[0]; 
    }
    /// Extract from the state cache the already-calculated inertial linear
    /// velocity vector v_GB of body B, measured with respect to the ground frame
    /// and expressed in the ground frame. This response is available at Velocity stage.
    const Vec3& getBodyLinearVelocity(const State& s, int body) const {
        return getBodyVelocity(s,body)[1];
    }

    /// Return the Cartesian (ground) location of a station fixed to a body. That is
    /// we return locationInG = X_GB * stationB. Cost is 18 flops. This operator is
    /// available at Position stage.
    Vec3 calcStationLocation(const State& s, int bodyB, const Vec3& stationB) const {
        return getBodyPosition(s,bodyB)*stationB;
    }
    /// Given a station on body B, return the station of body A which is at the same location
    /// in space. That is, we return stationInA = X_AG * (X_GB*stationB). Cost is 36 flops.
    /// This operator is available at Position stage.
    Vec3 calcStationLocationInBody(const State& s, int bodyB, const Vec3& stationB, int bodyA) {
        return ~getBodyPosition(s,bodyA) 
                * calcStationLocation(s,bodyB,stationB);
    }
    /// Re-express a vector expressed in the B frame into the same vector in G. That is,
    /// we return vectorInG = R_GB * vectorInB. Cost is 15 flops. 
    /// This operator is available at Position stage.
    Vec3 calcVectorOrientation(const State& s, int bodyB, const Vec3& vectorB) const {
        return getBodyRotation(s,bodyB)*vectorB;
    }
    /// Re-express a vector expressed in the B frame into the same vector in some other
    /// body A. That is, we return vectorInA = R_AG * (R_GB * vectorInB). Cost is 30 flops.
    /// This operator is available at Position stage.
    Vec3 calcVectorOrientationInBody(const State& s, int bodyB, const Vec3& vectorB, int bodyA) const {
        return ~getBodyRotation(s,bodyA) * calcVectorOrientation(s,bodyB,vectorB);
    }

    /// Given a station fixed on body B, return its inertial (Cartesian) velocity,
    /// that is, its velocity relative to the ground frame, expressed in the
    /// ground frame. Cost is 27 flops. This operator is available at Velocity stage.
    Vec3 calcStationVelocity(const State& s, int bodyB, const Vec3& stationB) const {
        const SpatialVec& V_GB       = getBodyVelocity(s,bodyB);
        const Vec3        stationB_G = calcVectorOrientation(s,bodyB,stationB);
        return V_GB[1] + V_GB[0] % stationB_G; // v + w X r
    }

    /// Given a station fixed on body B, return its velocity relative to the body frame of
    /// body A, and expressed in body A's body frame. Cost is 54 flops.
    /// This operator is available at Velocity stage.
    /// TODO: UNTESTED!!
    /// TODO: maybe these between-body routines should return results in ground so that they
    /// can be easily combined. Easy to re-express vector afterwards.
    Vec3 calcStationVelocityInBody(const State& s, int bodyB, const Vec3& stationB, int bodyA) const {
        // If body B's origin were coincident with body A's, then Vdiff_AB would be the relative angular
        // and linear velocity of body B in body A, expressed in G. To get the point we're interested in,
        // we need the vector from body A's origin to stationB to account for the extra linear velocity
        // that will be created by moving away from the origin.
        const SpatialVec Vdiff_AB = getBodyVelocity(s,bodyB) - getBodyVelocity(s,bodyA); // 6

        // This is a vector from body A's origin to the point of interest, expressed in G.
        const Vec3 stationA_G = calcStationLocation(s,bodyB,stationB) - getBodyLocation(s,bodyA); // 21
        const Vec3 v_AsB_G = Vdiff_AB[1] + Vdiff_AB[0] % stationA_G; // 12
        return ~getBodyRotation(s,bodyA) * v_AsB_G; // 15
    }

    const Real& getMobilizerQ(const State&, int body, int axis) const;
    const Real& getMobilizerU(const State&, int body, int axis) const;

    void setMobilizerQ(State&, int body, int axis, const Real&) const;
    void setMobilizerU(State&, int body, int axis, const Real&) const;

    /// At stage Position or higher, return the cross-mobilizer transform.
    /// This is X_MbM, the body's inboard mobilizer frame M measured and expressed in
    /// the parent body's corresponding outboard frame Mb.
    const Transform& getMobilizerPosition(const State&, int body) const;

    /// At stage Velocity or higher, return the cross-mobilizer velocity.
    /// This is V_MbM, the relative velocity of the body's inboard mobilizer
    /// frame M in the parent body's corresponding outboard frame Mb, 
    /// measured and expressed in Mb. Note that this isn't the usual 
    /// spatial velocity since it isn't expressed in G.
    const SpatialVec& getMobilizerVelocity(const State&, int body) const;

    /// This is a solver which sets the body's mobilizer transform as close
    /// as possible to the supplied Transform. The degree to which this is
    /// possible depends of course on the mobility provided by this body's
    /// mobilizer. However, no error will occur; on return the coordinates
    /// for this mobilizer will be as close as we can get them. Note: this
    /// has no effect on any coordinates except the q's for this mobilizer.
    /// You can call this solver at Stage::Model or higher; it will
    /// leave you no higher than Stage::Time since it changes the configuration.
    void setMobilizerPosition(State&, int body, const Transform& X_MbM) const;

    /// This is a solver which sets the body's cross-mobilizer velocity as close
    /// as possible to the supplied angular and linear velocity. The degree to which this is
    /// possible depends of course on the mobility provided by this body's
    /// mobilizer. However, no error will occur; on return the velocity coordinates
    /// (u's) for this mobilizer will be as close as we can get them. Note: this
    /// has no effect on any coordinates except the u's for this mobilizer.
    /// You can call this solver at Stage::Model or higher; it will
    /// leave you no higher than Stage::Position since it changes the velocities.
    void setMobilizerVelocity(State&, int body, const SpatialVec& V_MbM) const;


    /// This is available at Stage::Position. These are *absolute* constraint
    /// violations qerr=g(t,q), that is, they are unweighted.
    const Vector& getQConstraintErrors(const State&) const;

    /// This is the weighted norm of the errors returned by getQConstraintErrors(),
    /// available whenever this subsystem has been realized to Stage::Position.
    /// This is the scalar quantity that we need to keep below "tol"
    /// during integration.
    Real calcQConstraintNorm(const State&) const;

    /// This is available at Stage::Velocity. These are *absolute* constraint
    /// violations verr=v(t,q,u), that is, they are unweighted.
    const Vector& getUConstraintErrors(const State&) const;

    /// This is the weighted norm of the errors returned by getUConstraintErrors().
    /// That is, this is the scalar quantity that we need to keep below "tol"
    /// during integration.
    Real calcUConstraintNorm(const State&) const;

    /// This is available at Stage::Acceleration. These are *absolute* constraint
    /// violations aerr = A udot - b, that is, they are unweighted.
    const Vector& getUDotConstraintErrors(const State&) const;

    /// This is the weighted norm of the errors returned by getUDotConstraintErrors().
    Real calcUDotConstraintNorm(const State&) const;

    /// This is a solver you can call after the State has been realized
    /// to stage Time (i.e., Position-1). It will project the Q constraints
    /// along the error norm so that getQConstraintNorm() <= tol, and will
    /// project out the corresponding component of y_err so that y_err's Q norm
    /// is reduced. Returns true if it does anything at all to State or y_err.
    bool projectQConstraints(State&, Vector& y_err, Real tol, Real targetTol) const;

    /// This is a solver you can call after the State has been realized
    /// to stage Position (i.e., Velocity-1). It will project the U constraints
    /// along the error norm so that getUConstraintNorm() <= tol, and will
    /// project out the corresponding component of y_err so that y_err's U norm
    /// is reduced.
    bool projectUConstraints(State&, Vector& y_err, Real tol, Real targetTol) const;

    SimTK_PIMPL_DOWNCAST(MatterSubsystem, Subsystem);
    class MatterSubsystemRep& updRep();
    const MatterSubsystemRep& getRep() const;
};

} // namespace SimTK

#endif // SimTK_MATTER_SUBSYSTEM_H_
