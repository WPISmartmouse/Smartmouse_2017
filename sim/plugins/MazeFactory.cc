#include <sim/SimMouse.h>
#include "MazeFactory.hh"

namespace gazebo {

  const double MazeFactory::WALL_HEIGHT = 0.05;
  const double MazeFactory::WALL_LENGTH = 0.192;
  const double MazeFactory::BASE_HEIGHT = 0.005;
  const double MazeFactory::RED_HIGHLIGHT_THICKNESS = 0.01;

  MazeFactory::MazeFactory() : neighbor_dist(0, 3), remove_dist(0, 15) {}

  void MazeFactory::Load(physics::WorldPtr _parent, sdf::ElementPtr _sdf) {
    this->parent = _parent;
    node = transport::NodePtr(new transport::Node());
    node->Init(parent->Name());
    regen_sub = node->Subscribe("~/maze/regenerate", &MazeFactory::Regenerate, this);

    //seed random generator
    generator.seed(time(0));
  }

  void MazeFactory::Regenerate(ConstGzStringPtr &msg) {
    maze_filename = msg->data();

    sdf::ElementPtr model = LoadModel();
    sdf::ElementPtr base_link = model->GetElement("link");

    // Add base
    sdf::ElementPtr base_collision_pose = base_link->GetElement("collision")->GetElement("pose");
    sdf::ElementPtr base_collision = base_link->GetElement("collision")->GetElement("geometry");
    sdf::ElementPtr base_visual_pose = base_link->GetElement("visual")->GetElement("pose");
    sdf::ElementPtr base_visual = base_link->GetElement("visual")->GetElement("geometry");
    sdf::ElementPtr collision_box_size = base_collision->AddElement("box")->GetElement("size");
    sdf::ElementPtr visual_box_size = base_visual->AddElement("box")->GetElement("size");
    base_collision_pose->Set("0 0 " + std::to_string(BASE_HEIGHT / 2) + " 0 0 0");
    base_visual_pose->Set("0 0 " + std::to_string(BASE_HEIGHT / 2) + " 0 0 0");
    char base_size_str[40];
    snprintf(base_size_str, 40, "%f %f %f",
             AbstractMaze::MAZE_SIZE * AbstractMaze::UNIT_DIST + AbstractMaze::HALF_UNIT_DIST,
             AbstractMaze::MAZE_SIZE * AbstractMaze::UNIT_DIST + AbstractMaze::HALF_UNIT_DIST, BASE_HEIGHT);
    collision_box_size->Set(base_size_str);
    visual_box_size->Set(base_size_str);

    // Add interia values
    sdf::ElementPtr inertial = base_link->AddElement("inertial");
    sdf::ElementPtr inertia = inertial->AddElement("inertia");
    sdf::ElementPtr ixx = inertia->GetElement("ixx");
    sdf::ElementPtr iyy = inertia->GetElement("iyy");
    sdf::ElementPtr izz = inertia->GetElement("izz");
    sdf::ElementPtr mass = inertial->AddElement("mass");
    ixx->Set(69);
    iyy->Set(34);
    izz->Set(34);
    mass->Set(50);

    if (maze_filename == "random") {
      //create random maze here
      AbstractMaze maze = AbstractMaze::gen_random_legal_maze();

      std::string buff;
      buff.resize(AbstractMaze::BUFF_SIZE);
      maze.print_maze_str(&buff[0]);

      std::fstream fs;
      maze_filename = "/tmp/random.mz";
      fs.open(maze_filename, std::fstream::out);
      fs << buff << std::endl;
      fs.close();

      InsertWallsFromFile(base_link);
    } else {
      //load maze from file
      gzmsg << "loading from file " << maze_filename << std::endl;
      InsertWallsFromFile(base_link);
    }

    model->GetAttribute("name")->Set("my_maze");
    model->GetElement("pose")->Set(ignition::math::Pose3d(-X_OFFSET, -Y_OFFSET, 0, 0, 0, 0));
    parent->InsertModelSDF(*modelSDF);
  }

  void MazeFactory::InsertWallsFromFile(sdf::ElementPtr base_link) {
    std::fstream fs;
    fs.open(maze_filename, std::fstream::in);

    if (fs.good()) {
      //clear the old walls

      std::string line;

      //look West and North to connect any nodes
      for (unsigned int i = 0; i < AbstractMaze::MAZE_SIZE; i++) { //read in each line
        std::getline(fs, line);

        if (!fs) {
          gzmsg << "getline failed" << std::endl;
          return;
        }

        unsigned int charPos = 0;
        for (unsigned int j = 0; j < AbstractMaze::MAZE_SIZE; j++) {
          if (line.at(charPos) == '|') {
            //add a wall on the west
            InsertWall(base_link, i, j, Direction::W);
          }
          charPos++;
          if (line.at(charPos) == '_') {
            //add a wall on the south
            InsertWall(base_link, i, j, Direction::S);
          }
          charPos++;
        }
      }

      //add east and north walls
      for (unsigned int i = 0; i < AbstractMaze::MAZE_SIZE; i++) {
        InsertWall(base_link, i, AbstractMaze::MAZE_SIZE - 1, Direction::E);
        InsertWall(base_link, 0, i, Direction::N);
      }
    } else {
      gzmsg << "failed to load file " << maze_filename << std::endl;
    }
  }

  void MazeFactory::InsertWall(sdf::ElementPtr link, int row, int col, Direction dir) {
    //ignore requests to insert center wall
    if ((row == AbstractMaze::MAZE_SIZE / 2 && col == AbstractMaze::MAZE_SIZE / 2 &&
         (dir == Direction::N || dir == Direction::W))
        || (row == AbstractMaze::MAZE_SIZE / 2 && col == AbstractMaze::MAZE_SIZE / 2 - 1 &&
            (dir == Direction::N || dir == Direction::E))
        || (row == AbstractMaze::MAZE_SIZE / 2 - 1 && col == AbstractMaze::MAZE_SIZE / 2 &&
            (dir == Direction::S || dir == Direction::W))
        || (row == AbstractMaze::MAZE_SIZE / 2 - 1 && col == AbstractMaze::MAZE_SIZE / 2 - 1 &&
            (dir == Direction::S || dir == Direction::E))) {
      return;
    }

    std::list <sdf::ElementPtr> walls_visuals = CreateWallVisual(row, col, dir);
    sdf::ElementPtr walls_collision = CreateWallCollision(row, col, dir);

    //insert all the visuals
    std::list<sdf::ElementPtr>::iterator list_iter = walls_visuals.begin();
    while (list_iter != walls_visuals.end()) {
      link->InsertElement(*(list_iter++));
    }

    link->InsertElement(walls_collision);
  }

  std::list <sdf::ElementPtr> MazeFactory::CreateWallVisual(int row, int col, Direction dir) {
    msgs::Pose *visual_pose = CreatePose(row, col, BASE_HEIGHT + (WALL_HEIGHT - RED_HIGHLIGHT_THICKNESS) / 2, dir);
    msgs::Pose *paint_visual_pose = CreatePose(row, col, BASE_HEIGHT + WALL_HEIGHT - RED_HIGHLIGHT_THICKNESS / 2, dir);

    msgs::Geometry *visual_geo = CreateBoxGeometry(WALL_LENGTH, AbstractMaze::WALL_THICKNESS,
                                                   WALL_HEIGHT - RED_HIGHLIGHT_THICKNESS);
    msgs::Geometry *paint_visual_geo = CreateBoxGeometry(WALL_LENGTH, AbstractMaze::WALL_THICKNESS,
                                                         RED_HIGHLIGHT_THICKNESS);

    msgs::Visual visual;
    std::string visual_name = "v_" + std::to_string(row)
                              + "_" + std::to_string(col) + "_" + to_char(dir);
    visual.set_name(visual_name);
    visual.set_allocated_geometry(visual_geo);
    visual.set_allocated_pose(visual_pose);

    msgs::Material_Script *paint_script = new msgs::Material_Script();
    std::string *uri = paint_script->add_uri();
    *uri = "file://media/materials/scripts/gazebo.material";
    paint_script->set_name("Gazebo/Red");

    msgs::Material *paint_material = new msgs::Material();
    paint_material->set_allocated_script(paint_script);

    msgs::Visual paint_visual;
    std::string paint_visual_name = "paint_v_" + std::to_string(row)
                                    + "_" + std::to_string(col) + "_" + to_char(dir);
    paint_visual.set_name(paint_visual_name);
    paint_visual.set_allocated_geometry(paint_visual_geo);
    paint_visual.set_allocated_pose(paint_visual_pose);
    paint_visual.set_allocated_material(paint_material);

    sdf::ElementPtr visualElem = msgs::VisualToSDF(visual);
    sdf::ElementPtr visualPaintElem = msgs::VisualToSDF(paint_visual);
    std::list <sdf::ElementPtr> visuals;
    visuals.push_front(visualElem);
    visuals.push_front(visualPaintElem);
    return visuals;
  }

  sdf::ElementPtr MazeFactory::CreateWallCollision(int row, int col, Direction dir) {
    msgs::Pose *collision_pose = CreatePose(row, col, BASE_HEIGHT + WALL_HEIGHT / 2, dir);

    msgs::Geometry *collision_geo = CreateBoxGeometry(WALL_LENGTH, AbstractMaze::WALL_THICKNESS, WALL_HEIGHT);

    msgs::Collision collision;
    std::string collision_name = "p_" + std::to_string(row) + "_" + std::to_string(col) + "_" + to_char(dir);
    collision.set_name(collision_name);
    collision.set_allocated_geometry(collision_geo);
    collision.set_allocated_pose(collision_pose);

    sdf::ElementPtr collisionElem = msgs::CollisionToSDF(collision);
    return collisionElem;
  }

  msgs::Pose *MazeFactory::CreatePose(int row, int col, double z, Direction dir) {
    double x_offset = 0, y_offset = 0;
    double z_rot = 0;

    switch (dir) {
      case Direction::N:
        y_offset = AbstractMaze::UNIT_DIST / 2;
        break;
      case Direction::E:
        x_offset = AbstractMaze::UNIT_DIST / 2;
        z_rot = M_PI / 2;
        break;
      case Direction::S:
        y_offset = -AbstractMaze::UNIT_DIST / 2;
        break;
      case Direction::W:
        x_offset = -AbstractMaze::UNIT_DIST / 2;
        z_rot = M_PI / 2;
        break;
      default:
        break;
    }

    double zero_offset = (AbstractMaze::UNIT_DIST * (AbstractMaze::MAZE_SIZE - 1) / 2);
    double x = -zero_offset + x_offset + col * AbstractMaze::UNIT_DIST;
    double y = zero_offset + y_offset - row * AbstractMaze::UNIT_DIST;

    msgs::Vector3d *position = new msgs::Vector3d();
    position->set_x(x);
    position->set_y(y);
    position->set_z(z);

    msgs::Quaternion *orientation = new msgs::Quaternion();
    orientation->set_z(sin(z_rot / 2));
    orientation->set_w(cos(z_rot / 2));

    msgs::Pose *pose = new msgs::Pose;
    pose->set_allocated_orientation(orientation);
    pose->set_allocated_position(position);

    return pose;
  }

  msgs::Geometry *MazeFactory::CreateBoxGeometry(double x, double y, double z) {
    msgs::Vector3d *size = new msgs::Vector3d();
    size->set_x(x);
    size->set_y(y);
    size->set_z(z);

    msgs::BoxGeom *box = new msgs::BoxGeom();
    box->set_allocated_size(size);

    msgs::Geometry *geo = new msgs::Geometry();
    geo->set_type(msgs::Geometry_Type_BOX);
    geo->set_allocated_box(box);

    return geo;
  }

  sdf::ElementPtr MazeFactory::LoadModel() {
    modelSDF.reset(new sdf::SDF);

    sdf::initFile("root.sdf", modelSDF);

    sdf::ElementPtr root = modelSDF->Root();
    sdf::ElementPtr model = root->GetElement("model");
    return model;
  }

  Direction operator++(Direction &dir, int) {
    switch (dir) {
      case Direction::N:
        dir = Direction::E;
        break;
      case Direction::E:
        dir = Direction::S;
        break;
      case Direction::S:
        dir = Direction::W;
        break;
      case Direction::W:
        dir = Direction::Last;
        break;
      default:
        dir = Direction::INVALID;
    }
    return dir;
  }

  char MazeFactory::to_char(Direction dir) {
    switch (dir) {
      case Direction::N:
        return 'N';
      case Direction::E:
        return 'E';
      case Direction::S:
        return 'S';
      case Direction::W:
        return 'W';
      default:
        return '\0';
    }
  }

  GZ_REGISTER_WORLD_PLUGIN(MazeFactory)
}
