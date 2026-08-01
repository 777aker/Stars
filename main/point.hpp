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
        if (d > 2)
        {
            // gravity to other point
            // nextv += 1 / (d * d * d) * dir * 0.000001f;
            return;
        }
        if (d == 0)
        {
            return;
        }
        glm::vec2 otherdir = pos - otherpos;
        dir = glm::normalize(dir);
        otherdir = glm::normalize(otherdir);

        nextv += otherdir * glm::dot(otherdir, other.velocity) * 0.8f;
        nextv -= dir * glm::dot(dir, velocity) * 0.8f;
    }

    void doPhysics(float dim, float asp)
    {

        // if (nextv.x < -dim * asp)
        // {
        //     nextv.x = abs(nextv.x) * 0.2;
        // }
        // else if (nextv.x > dim * asp)
        // {
        //     nextv.x = -abs(nextv.x) * 0.2;
        // }

        // if (nextv.y < -dim)
        // {
        //     nextv.y = abs(nextv.y) * 0.2;
        // }
        // else if (nextv.y > dim)
        // {
        //     nextv.y = -abs(nextv.y) * 0.2;
        // }

        velocity = nextv;
        // add velocity to position
        x += velocity.x;
        y += velocity.y;
    }

private:
    glm::vec2 nextv;
};