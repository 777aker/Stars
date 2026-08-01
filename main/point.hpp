#pragma once

class Point
{
public:
    Point(float tx, float ty, float tvx, float tvy) : x(tx), y(ty), vx(tvx), vy(tvy)
    {
    }
    float x;
    float y;
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