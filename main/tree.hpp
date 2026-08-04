#pragma once

// different headers depending on what machine you're on
#ifdef USEGLEW
#include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#include <OpenGL/gl.h>
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif

#include "point.hpp"

#include <glm/glm.hpp>
#include <vector>

struct quadGrav
{
    glm::vec2 center;
    float power;
};

class QuadTree
{
public:
    QuadTree(int tmax_size, glm::vec2 tlowerleft, glm::vec2 ttopright)
        : max_size(tmax_size), lowerleft(tlowerleft), topright(ttopright)
    {
        gravowner = true;
        toplevelgrav = new std::vector<quadGrav>();
    }

    QuadTree(int tmax_size, glm::vec2 tlowerleft, glm::vec2 ttopright, std::vector<quadGrav> *topgrav)
        : max_size(tmax_size), lowerleft(tlowerleft), topright(ttopright), toplevelgrav(topgrav)
    {
    }

    ~QuadTree()
    {
        if (split)
        {
            delete bot_left;
            delete bot_right;
            delete top_left;
            delete top_right;
        }

        if (gravowner)
        {
            delete toplevelgrav;
        }
    }

    void insert_point(Point *point)
    {
        if (split)
        {
            if (point->x <= center.x && point->y <= center.y)
            {
                top_left->insert_point(point);
            }
            if (point->x <= center.x && point->y >= center.y)
            {
                bot_left->insert_point(point);
            }
            if (point->x >= center.x && point->y <= center.y)
            {
                top_right->insert_point(point);
            }
            if (point->x >= center.x && point->y >= center.y)
            {
                bot_right->insert_point(point);
            }
            return;
        }
        mypoints.push_back(point);
        if (mypoints.size() > max_size)
        {
            split = true;
            center = glm::vec2(0, 0);
            for (Point *point : mypoints)
            {
                center += glm::vec2(point->x, point->y);
            }
            center /= static_cast<float>(mypoints.size());

            bot_left = new QuadTree(max_size, lowerleft, center, toplevelgrav);
            bot_right = new QuadTree(max_size, glm::vec2(center.x, lowerleft.y), glm::vec2(topright.x, center.y), toplevelgrav);
            top_left = new QuadTree(max_size, glm::vec2(lowerleft.x, center.y), glm::vec2(center.x, topright.y), toplevelgrav);
            top_right = new QuadTree(max_size, center, topright, toplevelgrav);

            for (Point *point : mypoints)
            {
                insert_point(point);
            }
            mypoints.clear();
            return;
        }
    }

    void draw_me()
    {
        if (split)
        {
            bot_left->draw_me();
            bot_right->draw_me();
            top_left->draw_me();
            top_right->draw_me();

            glColor3f(1.0f, 0.0f, 1.0f);
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glVertex2f(lowerleft.x, center.y);
            glVertex2f(topright.x, center.y);
            glVertex2f(center.x, lowerleft.y);
            glVertex2f(center.x, topright.y);
            glEnd();
        }
    }

    void calcgrav()
    {
        if (split)
        {
            bot_left->calcgrav();
            bot_right->calcgrav();
            top_left->calcgrav();
            top_right->calcgrav();
            return;
        }
        struct quadGrav mygrav = {
            center,
            mypoints.size()};
        toplevelgrav->push_back(std::move(mygrav));
    }

    void do_physics()
    {
        if (split)
        {
            bot_left->do_physics();
            bot_right->do_physics();
            top_left->do_physics();
            top_right->do_physics();
            return;
        }

        for (Point *point : mypoints)
        {
            for (Point *other : mypoints)
            {
                point->doGravandCollide(*other);
            }
            for (quadGrav grav : *toplevelgrav)
            {
                glm::vec2 pos(point->x, point->y);
                glm::vec2 dir = grav.center - pos;
                float d = glm::distance(pos, grav.center);
                if (d > 2.0f)
                    point->nextv += grav.power / (d * d * d) * dir * 0.001f;
            }
        }
    }

private:
    int max_size;
    bool split = false;
    bool gravowner = false;
    std::vector<Point *> mypoints;

    std::vector<quadGrav> *toplevelgrav;

    glm::vec2 lowerleft;
    glm::vec2 topright;
    glm::vec2 center;

    QuadTree *bot_left;
    QuadTree *bot_right;
    QuadTree *top_left;
    QuadTree *top_right;
};