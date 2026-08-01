#pragma once

#include <cmath>
#include <glm/glm.hpp>

class Point
{
public:
    float x;
    float y;
    glm::vec2 velocity;

    Point(float tx, float ty, glm::vec2 tv) : x(tx), y(ty), velocity(tv)
    {
        nextv = velocity;
        nextpos = glm::vec2(x, y);
    }

    void doGravandCollide(Point other)
    {
        // get direction to other point
        glm::vec2 pos(x, y);
        glm::vec2 otherpos(other.x, other.y);
        glm::vec2 dir = otherpos - pos;
        // get distance
        float d = glm::distance(pos, otherpos);
        // if we aren't colliding
        // gravity to other point
        if (d == 0)
        {
            return;
        }
        if (d > 2)
        {
            nextv += 1 / (d * d * d) * dir * 0.00001f;
            return;
        }
        glm::vec2 otherdir = pos - otherpos;
        dir = glm::normalize(dir);
        otherdir = glm::normalize(otherdir);

        // moving apart abort probably already collided
        // glm::vec2 vdir = other.velocity - velocity;
        // if (glm::dot(dir, vdir) > 0)
        // {
        //     return;
        // }

        nextv += otherdir * glm::dot(otherdir, other.velocity) * 0.8f;
        nextv -= dir * glm::dot(dir, velocity) * 0.8f;
        nextpos += otherdir * (2 - d) * 1.2f;
    }

    void doPhysics(float dim, float asp)
    {
        velocity = nextv;
        nextpos += velocity;
        // add velocity to position
        x = nextpos.x;
        y = nextpos.y;
    }

private:
    glm::vec2 nextpos;
    glm::vec2 nextv;
};