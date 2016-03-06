#ifdef SIM
#include <gazebo/msgs/msgs.hh>
#include <gazebo/transport/transport.hh>
#include <ignition/math.hh>
#include <mutex>
#include <condition_variable>
#include "Mouse.h"

class SimMouse : public Mouse {
  public:

    virtual int forward() override;
    virtual void turn_to_face(Direction d) override;

    void simInit();

    gazebo::transport::PublisherPtr control_pub;
    void pose_callback(ConstPosePtr &msg);

  private:
    float rotation(ignition::math::Pose3d p0, ignition::math::Pose3d p1);
    float forwardDisplacement(ignition::math::Pose3d p0, ignition::math::Pose3d p1);
    float absYawDiff(float y1, float y2);

    ignition::math::Pose3d pose;
    std::mutex pose_mutex;
    std::condition_variable pose_cond;
};
#endif