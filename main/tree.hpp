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
#include <atomic>
#include <mutex>

struct quadGrav
{
    glm::vec2 center;
    float power;
};

class QuadTree
{
public:
    // constructor for the root of the quad tree
    QuadTree(unsigned long int tmax_size, glm::vec2 tlowerleft, glm::vec2 ttopright)
        : max_size(tmax_size), lowerleft(tlowerleft), topright(ttopright)
    {
        gravowner = true;
        toplevelgrav = new std::vector<quadGrav>();
    }

    // constructor for child nodes of the quad tree
    QuadTree(unsigned long int tmax_size, glm::vec2 tlowerleft, glm::vec2 ttopright, std::vector<quadGrav> *topgrav)
        : max_size(tmax_size), lowerleft(tlowerleft), topright(ttopright), toplevelgrav(topgrav)
    {
    }

    // clean up memory
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

    // insert a point to the quad tree
    void insert_point(Point *point)
    {
        if (split)
        {
            // we do = also because we want all potential collisions to be considered
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
        // if we over flow split
        if (mypoints.size() > max_size)
        {
            // calculate center of points because we want to maximize efficient use of space rather than use a boring quad tree that splits the space evenly
            // ie: split the points evenly
            split = true;
            center = glm::vec2(0, 0);
            for (Point *point : mypoints)
            {
                center += glm::vec2(point->x, point->y);
            }
            center /= static_cast<float>(mypoints.size());

            // make all of our new quad trees
            bot_left = new QuadTree(max_size, lowerleft, center, toplevelgrav);
            bot_right = new QuadTree(max_size, glm::vec2(center.x, lowerleft.y), glm::vec2(topright.x, center.y), toplevelgrav);
            top_left = new QuadTree(max_size, glm::vec2(lowerleft.x, center.y), glm::vec2(center.x, topright.y), toplevelgrav);
            top_right = new QuadTree(max_size, center, topright, toplevelgrav);

            // we're now split so this will actually insert each point into the proper children
            for (Point *point : mypoints)
            {
                insert_point(point);
            }
            // unnecessary but just in case
            mypoints.clear();
            return;
        }
    }

    // helper function for verifying and visualizing quad trees
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

    // calculate the gravity this quad has
    void calcgrav()
    {
        if (split)
        {
            bot_left->calcgrav();
            bot_right->calcgrav();
            top_left->calcgrav();
            top_right->calcgrav();
            // we don't have any points if we're split return
            return;
        }
        // calculate my gravity's center and mass
        glm::vec2 gravcenter(0.0f, 0.0f);
        for (Point *point : mypoints)
        {
            gravcenter += glm::vec2(point->x, point->y);
        }
        gravcenter /= mypoints.size();
        struct quadGrav mygrav = {
            gravcenter,
            static_cast<float>(mypoints.size())};
        // insert our gravity into a flat list for easy traversal
        toplevelgrav->push_back(std::move(mygrav));
    }

    // call physics on all of my points or children
    void do_physics(size_t tid)
    {
        if (visited.load())
        {
            return;
        }
        if (split)
        {
            switch (tid % 4)
            {
            case 0:
                bot_left->do_physics(tid);
                bot_right->do_physics(tid);
                top_left->do_physics(tid);
                top_right->do_physics(tid);
                break;
            case 1:
                top_right->do_physics(tid);
                top_left->do_physics(tid);
                bot_left->do_physics(tid);
                bot_right->do_physics(tid);
                break;
            case 2:
                top_left->do_physics(tid);
                top_right->do_physics(tid);
                bot_left->do_physics(tid);
                bot_right->do_physics(tid);
                break;
            case 3:
                bot_right->do_physics(tid);
                top_right->do_physics(tid);
                bot_left->do_physics(tid);
                top_left->do_physics(tid);
                break;
            }
            visited.store(true);
            return;
        }
        bool expected = false;
        if (!visited.compare_exchange_strong(expected, true))
        {
            return;
        }
        // for every point inside me check if they collide with any other points and do gravity effect
        for (Point *point : mypoints)
        {
            for (Point *other : mypoints)
            {
                point->doCollide(*other);
            }
            for (quadGrav grav : *toplevelgrav)
            {
                glm::vec2 pos(point->x, point->y);
                glm::vec2 dir = grav.center - pos;
                float d = glm::distance(pos, grav.center);
                if (d > 2.0f)
                {
                    point->nextv += grav.power / (d * d * d) * dir * 0.0001f;
                }
            }
        }
    }

private:
    unsigned long int max_size;
    bool split = false;
    bool gravowner = false;
    std::atomic<bool> visited = false;
    std::vector<Point *>
        mypoints;

    glm::vec2 lowerleft;
    glm::vec2 topright;
    glm::vec2 center;

    std::vector<quadGrav> *toplevelgrav;

    QuadTree *bot_left;
    QuadTree *bot_right;
    QuadTree *top_left;
    QuadTree *top_right;
};