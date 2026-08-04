#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <algorithm>

class Point
{
public:
    float x;
    float y;
    float r = 0.6f;
    float g = 0.3f;
    float b = 0.2f;
    glm::vec2 velocity;
    glm::vec2 nextv;

    Point(float tx, float ty, glm::vec2 tv) : x(tx), y(ty), velocity(tv)
    {
        nextv = velocity;
    }

    void doCollide(Point other)
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
            // nextv += 1 / (d * d * d) * dir * 0.001f;
            return;
        }
        if (glm::dot(dir, other.velocity - velocity) > 0.0f)
        {
            return;
        }
        glm::vec2 otherdir = glm::normalize(pos - otherpos);
        nextv -= dir * glm::dot(dir, velocity) * 0.8f;
        nextv += otherdir * glm::dot(otherdir, other.velocity) * 0.8f;
        r += 0.0001f;
        g += 0.05f;
        b -= 0.00001f;
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
        r -= 0.001f;
        g -= 0.01f;
        b -= 0.0001f;
        r = std::clamp(r, 0.6f, 0.7f);
        g = std::clamp(g, 0.3f, 0.6f);
        b = std::clamp(b, 0.0f, 0.2f);
    }
};