﻿#ifndef _ECS_H
#define _ECS_H

#include <iostream>

#include <chrono>
#include <unordered_map>
#include <functional>
#include <vector>
#include <map>
#include <algorithm>


#include <string>
#include <memory>

#include <typeindex>
#include <typeinfo>

#include "ThreadPool.h"
#include "taskserializeThreadPool.h"

#define SYSTEM_TYPE_SYNCWORLD 0
#define SYSTEM_TYPE_SYNC 1
#define SYSTEM_TYPE_ASYNC 2

namespace JT {
  
  class World;
  class EntitySystem;
  class Entity;
  class Component;

  typedef std::type_index TypeIndex;

  template<typename T>
  TypeIndex getTypeIndex()
  {
    return TypeIndex(typeid(T));
  }
  
  typedef std::unordered_map<std::type_index, std::shared_ptr<Component>> Components;
  //typedef std::vector<std::shared_ptr<Component>> Components;
  typedef std::vector<std::shared_ptr<Entity>> Entites;
  typedef std::vector<std::shared_ptr<EntitySystem>> Systems;

  // 컴포넌트 원형
  class Component : public std::enable_shared_from_this<Component>{
  public:
    virtual ~Component() {}
  };

  // 시스템 원형
  class EntitySystem : public std::enable_shared_from_this<EntitySystem>{
  public :
    EntitySystem() {}
    EntitySystem(std::shared_ptr<World> _world) : world_(_world) {}

    virtual void tick(){}

    void SetType(size_t _type) { system_type_ = _type; }
    size_t GetType() { return system_type_; }

    void SetTime() { systemtime_ = std::chrono::system_clock::now(); }
    void SetDeltaTime() { 
      deltatime_ = std::chrono::system_clock::now() - systemtime_;
      SetTime();
    }
    float GetDeltaTime() { return deltatime_.count(); }

    virtual std::type_index GetTypeIndex() { return std::type_index(typeid(this)); }

  protected :
    size_t system_type_ = SYSTEM_TYPE_SYNCWORLD;

    std::shared_ptr<World> world_;
    std::chrono::system_clock::time_point systemtime_;
    std::chrono::duration<double> deltatime_;
  };

  class Entity : public std::enable_shared_from_this<Entity>{
  public:
    //엔티티는 아무것도 없다. 컴포넌트를 가지고 있을뿐.
    Entity(std::shared_ptr<World> _world) : world(_world) {}
    Entity(std::shared_ptr<World> _world, std::string _name) : world(_world), name(_name) {}

    void SetName(std::string n) { name = n; }
    std::string GetName() { return name; }

    // 엔티티가 컴포넌트를 가지고 있는지 확인한다.
    template<class T>
    bool searchComponent() {
      Components::const_iterator got = components.find(std::type_index(typeid(T)));

      if (got == components.end()) {
        return false;
      }
      else {
        return true;
      }

      return false;
    }

    // 간단한 컴포넌트 반환 함수
    template <class T>
    std::shared_ptr<T> GetComponent() {

      std::shared_ptr<T> reComponent = std::make_shared<T>();

      auto com = components.begin();
      for (com = components.begin(); com != components.end(); ++com) {
        if (std::type_index(typeid(T)) == (*com).first) {
          return std::static_pointer_cast<T>((*com).second);
        }
      }
      return std::shared_ptr<T>(nullptr);

    }

    // 컴포넌트가 추가 되면 월드에 있는 시스템에 추가한다.
    template<typename T>
    int AddComponent(T _c) {

      std::cout << GetName() << " add Component : " << typeid(_c.get()).name() << std::endl;
      components.insert(std::make_pair(std::type_index(typeid(*(_c.get()))), _c));
      return 0;
    }

    template <typename T>
    bool DeleteComponent() {
      components.erase(std::type_index(typeid(T)));
    }
  private:
    std::string name;
    std::shared_ptr<World> world;
    Components components;

  };
  // 월드
  class World : public std::enable_shared_from_this<World>{
  public :
    //World() {
      //c_time = std::chrono::system_clock::now();
    //}
      // 엔티티를 만들고 넘겨 준다.
    std::shared_ptr<Entity> create() {
      std::shared_ptr<Entity> ent = std::make_shared<Entity>(shared_from_this());
      std::cout << "World Create Entity" << std::endl;
      entites.push_back(ent);

      return ent;
    }
    //std::shared_ptr<Entity> GetEntity() { return entites; }

    // 월드는 받은 펑터로 원하는 타입의 컴포넌트가 있을경우 실행한다.
    template<typename T>
    void each(std::function<void(std::shared_ptr<Entity>)> _func) {
      auto ent = entites.begin();
      for (ent = entites.begin(); ent != entites.end(); ++ent) {
        if(ent->get()->searchComponent<T>()){
          _func((*ent));
        }
      }
    }

    template<typename T>
    void AddSystem(T _sys) {
      std::cout << "Add System : " << typeid(T).name() << std::endl;
      systems.push_back(_sys);
      _sys->SetTime();
    }
    // 월드가 움직인다.
    void tick(){
      for (auto sys : systems)
      {
        /*
        std::cout << "Test : " << std::type_index(typeid(sys->tick)).name() << std::endl;
        system("pause");*/
        // 혹시 필요할지 모른다.
        sys->SetDeltaTime();

        switch (sys->GetType())
        {
        case SYSTEM_TYPE_SYNCWORLD:
          sys->tick();
          break;
        case SYSTEM_TYPE_SYNC:
          ts_threadpool_.enqueue([&]() {
            //std::cout << "Running Task Sync" << std::endl;
            sys->tick();
          }, sys->GetTypeIndex());
          //system("pause");
          break;
        case SYSTEM_TYPE_ASYNC:
          bs_threadpool_.enqueue([&]() {
            sys->tick(); 
          });
          break;
        default:
          break;
        }
        std::cout << "this" << sys->GetDeltaTime() << "sec" << std::endl;
      }
      bs_threadpool_.join();
      ts_threadpool_.join();
    }
    std::shared_ptr<Entity> SearchEntity(std::string _enname) {
      for (auto &en : entites) {
        if (en.get()->GetName() == _enname)
          return en;
      }
      return std::shared_ptr<Entity>(nullptr);
    }
    // 삭제 명령어
#pragma region DeleteAll

    bool DeleteEntitys(std::string _enname) {
      if (entites.empty()) return false;

      bool isdelete = false;
      for (auto &en : entites) {
        if (en.get()->GetName() == _enname) {
          auto findentity = std::find(entites.begin(), entites.end(), en);
          entites.erase(findentity);
          isdelete = true;
        }
      }

      return isdelete;
    }
    bool DeleteEntity(std::string _enname) {
      if (entites.empty()) return false;

      for (auto &en : entites) {
        if (en.get()->GetName() == _enname) {
          auto findentity = std::find(entites.begin(), entites.end(), en);
          entites.erase(findentity);
          return true;
        }
      }
      return false;
    }
    bool DeleteEntity(std::string _enname, bool _all) {
      if (_all) {
        return DeleteEntitys(_enname);
      }
      else {
        return DeleteEntity(_enname);
      }
    }
    bool DeleteEntity(std::shared_ptr<Entity> _en) {
      // 엔티티가 비였다면 삭제불가.
      if (entites.empty()) return false;

      auto findentity = std::find(entites.begin(), entites.end(), _en);
      if ( findentity != entites.end()) {
        entites.erase(findentity);
        return true;
      }
      else {
        return false;
      }
    }

#pragma endregion

  private :
    Systems systems;
    Entites entites;

    ThreadPool bs_threadpool_;
    SerializeThreadPool ts_threadpool_;

    //std::chrono::system_clock::time_point c_time;
  };
}

#endif // _ECS_H

