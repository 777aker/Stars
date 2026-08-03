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
        dir = glm::normalize(dir);
        // get distance
        float d = glm::distance(pos, otherpos);
        // if we aren't colliding
        // gravity to other point
        if (d < 1.0f)
        {
            return;
        }
        if (d > 2.0f)
        {
            nextv += 1 / (d * d * d) * dir * 0.001f;
            return;
        }
        if (glm::dot(dir, other.velocity - velocity) > 0.0f)
        {
            return;
        }
        glm::vec2 otherdir = glm::normalize(pos - otherpos);
        nextv -= dir * glm::dot(dir, velocity) * 0.8f;
        nextv += otherdir * glm::dot(otherdir, other.velocity) * 0.8f;
    }

    void doPhysics(float dim, float asp)
    {
        if (x < -dim * asp)
        {
            nextv.x = abs(nextv.x);
        }
        else if (x > dim * asp)
        {
            nextv.x = -abs(nextv.x);
        }
        if (y < -dim)
        {
            nextv.y = abs(nextv.y);
        }
        else if (y > dim)
        {
            nextv.y = -abs(nextv.y);
        }
        velocity = nextv;
        // add velocity to position
        x += velocity.x;
        y += velocity.y;
    }

private:
    glm::vec2 nextv;
};