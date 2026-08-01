#pragma once

#include <cmath>

class Point
{
public:
    Point(float tx, float ty, float tvx, float tvy) : x(tx), y(ty), vx(tvx), vy(tvy)
    {
    }
    float x;
    float y;
    void doGravandCollide(Point other)
    {
        float xdir = other.x - x;
        float ydir = other.y - y;
        float d = std::sqrt(xdir * xdir + ydir * ydir);
        if (d < 2)
        {
            return;
        }
        vx += 1 / (d * d * d) * xdir * 0.01;
        vy += 1 / (d * d * d) * ydir * 0.01;
    }
    void doPhysics(float dim, float asp)
    {
        if (x <= -dim * asp)
        {
            vx = abs(vx);
        }
        else if (x >= dim * asp)
        {
            vx = -abs(vx);
        }
        if (y <= -dim)
        {
            vy = abs(vy);
        }
        else if (y >= dim)
        {
            vy = -abs(vy);
        }
        x += vx;
        y += vy;
    }

private:
    float vx;
    float vy;
};