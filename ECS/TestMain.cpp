#pragma once
#pragma warning(disable:4996)


#include "ECS.h"
#include <stdio.h>
#include <conio.h>


using namespace std;

// 기본형 위치 컴포넌트
class Position : public JT::Component {
public :
  Position() : X(0), Y(0) {}
  Position(int x_, int y_) : X(x_), Y(y_) {}

  void SetPosition(int x_, int y_) {
    X = x_; Y = y_;
  }
  void SetX(int x_) { X = x_; }
  void SetY(int y_) { Y = y_; }
  int GetX() { return X; }
  int GetY() { return Y; }

private :
  int X, Y;
};

// 유저 컨트롤러 예시
class Controller : public JT::Component {
public :
  Controller() : R(1), U(10){}
  Controller(int r_, int u_) : R(r_), U(u_) {}

  int GetR() { return R; }
  int GetL() { return -R; }
  int GetU() { return U; }
  int GetD() { return -U; }

private :
  int R, U;
};


// 중력 엔티티시스템
class Gravity : public JT::EntitySystem {
public :
  Gravity(std::shared_ptr<JT::World> world_) : JT::EntitySystem(world_) {}
  void tick() override {
    //std::cout << "Start Gravity Tick " << std::endl;
    world_.get()->each<Position>([&](std::shared_ptr<JT::Entity> ent) {
      auto pos = ent->GetComponent<Position>().get();
      if (pos->GetY() > 0) {
        pos->SetY(pos->GetY() - 1);
      }
    });
  }
  std::type_index GetTypeIndex() override {
    return std::type_index(typeid(this)); 
  }

};

// 키입력 시스템
class User : public JT::EntitySystem {
public:
  User(std::shared_ptr<JT::World> world_) : JT::EntitySystem(world_) {}
  void tick() override {
    world_.get()->each<Controller>([&](std::shared_ptr<JT::Entity> ent) {
      auto pos = ent->GetComponent<Position>().get();
      auto con = ent->GetComponent<Controller>().get();
      char a;
      int b;
      a = getch();
      b = int(a);
      if (b == 75) {
        pos->SetX(pos->GetX() + con->GetL());
      }
      else if (b == 77) {
        pos->SetX(pos->GetX() + con->GetR());
      }
      else if (b == 72) {
        pos->SetY(pos->GetY() + con->GetU());
      }
      else if (b == 80) {
        pos->SetY(pos->GetY() + con->GetD());
      }
    });
  }
  std::type_index GetTypeIndex() override {
    return std::type_index(typeid(this));
  }
};

void main() {
  std::shared_ptr<JT::World> world = std::make_shared<JT::World>();
  std::shared_ptr<Gravity> gri = std::make_shared<Gravity>(world);
  std::shared_ptr<User> use = std::make_shared<User>(world);
  use->SetType(SYSTEM_TYPE_SYNC);
  world->AddSystem(gri);
  world->AddSystem(use);

  auto user = world->create();
  user->SetName("User");
  user.get()->AddComponent(std::make_shared<Position>());
  user.get()->AddComponent(std::make_shared<Controller>());

  auto Mon = world->create();
  Mon->SetName("Monster");
  Mon.get()->AddComponent(std::make_shared<Position>());

  while (true) {
    //std::cout << "World Tick" << std::endl;
    world->tick();
  }
}
