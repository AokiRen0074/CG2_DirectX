#include "Math/TimedCall.h"
#include "Math/MathUtility.h"
#include "3D/Particle.h"
#include "Collision/CollisionManager.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void Require(bool value) { if (!value) throw std::runtime_error("Engine utility regression"); }
struct Sphere : Collider {
    Vector3 position;
    int hits = 0;
    Vector3 GetWorldPosition() override { return position; }
    void OnCollision() override { ++hits; }
};
int main() {
    int calls = 0;
    TimedCall immediate([&] { ++calls; }, 0);
    immediate.Update(); immediate.Update();
    Require(calls == 1);
    TimedCall delayed([&] { ++calls; }, 2);
    delayed.Update(); Require(calls == 1);
    delayed.Update(); delayed.Update(); Require(calls == 2);

    const std::vector<Vector3> points{{0,0,0},{1,0,0},{2,0,0},{3,0,0}};
    Require(std::abs(CatmullRomPosition(points, 0).x) < 0.0001f);
    Require(std::abs(CatmullRomPosition(points, 1).x - 3) < 0.0001f);
    Require(std::abs(CatmullRomPosition(points, 0.5f).x - 1.5f) < 0.0001f);

    ParticleParameters parameters;
    parameters.velocity = {2,0,0};
    Particle particle(parameters);
    particle.Update(0.5f);
    Require(particle.IsAlive() && particle.GetPosition().x == 1 && particle.GetColor().w == 0.5f);
    particle.Update(0.5f);
    Require(!particle.IsAlive() && particle.GetColor().w == 0);
    bool rejected = false;
    try { particle.Update(-1); } catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected);

    Sphere a, b;
    b.position = {2,0,0};
    CollisionManager manager;
    manager.AddCollider(&a); manager.AddCollider(&b);
    manager.CheckAllCollisions(); Require(a.hits == 1 && b.hits == 1);
    a.SetCollisionMask(0);
    manager.CheckAllCollisions(); Require(a.hits == 1 && b.hits == 1);
    manager.ClearColliders();
    std::cout << "PASS: timers, spline endpoints, particle lifetime and collision masks.\n";
}
