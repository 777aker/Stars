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
    std::vector<quadGrav> *toplevelgrav;
    std::vector<int> *toplevelint;
    std::vector<Point *> *flattened_points;

    // constructor for the root of the quad tree
    QuadTree(unsigned long int tmax_size, glm::vec2 tlowerleft, glm::vec2 ttopright, unsigned long num_particles)
        : max_size(tmax_size), lowerleft(tlowerleft), topright(ttopright)
    {
        gravowner = true;
        toplevelgrav = new std::vector<quadGrav>();
        toplevelint = new std::vector<int>();
        flattened_points = new std::vector<Point *>();
        flattened_points->reserve(num_particles);
    }

    // constructor for child nodes of the quad tree
    QuadTree(unsigned long int tmax_size, glm::vec2 tlowerleft, glm::vec2 ttopright, std::vector<quadGrav> *topgrav, std::vector<Point *> *topflat, std::vector<int> *topint)
        : toplevelgrav(topgrav), toplevelint(topint), flattened_points(topflat), max_size(tmax_size), lowerleft(tlowerleft), topright(ttopright)
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
            delete flattened_points;
        }
    }

    // insert a point to the quad tree
    void insert_point(Point *point)
    {
        if (split)
        {
            if (point->x <= center.x && point->y <= center.y)
            {
                top_left->insert_point(point);
            }
            if (point->x <= center.x && point->y > center.y)
            {
                bot_left->insert_point(point);
            }
            if (point->x > center.x && point->y <= center.y)
            {
                top_right->insert_point(point);
            }
            if (point->x > center.x && point->y > center.y)
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
            bot_left = new QuadTree(max_size, lowerleft, center, toplevelgrav, flattened_points, toplevelint);
            bot_right = new QuadTree(max_size, glm::vec2(center.x, lowerleft.y), glm::vec2(topright.x, center.y), toplevelgrav, flattened_points, toplevelint);
            top_left = new QuadTree(max_size, glm::vec2(lowerleft.x, center.y), glm::vec2(center.x, topright.y), toplevelgrav, flattened_points, toplevelint);
            top_right = new QuadTree(max_size, center, topright, toplevelgrav, flattened_points, toplevelint);

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
    void flatten()
    {
        if (split)
        {
            bot_left->flatten();
            bot_right->flatten();
            top_left->flatten();
            top_right->flatten();
            // we don't have any points if we're split return
            return;
        }
        // calculate my gravity's center and mass
        glm::vec2 gravcenter(0.0f, 0.0f);
        gravcenter /= mypoints.size();

        int me = toplevelint->size();
        toplevelint->push_back(mypoints.size());
        for (Point *point : mypoints)
        {
            point->myint = me;
            gravcenter += glm::vec2(point->x, point->y);
            flattened_points->push_back(point);
        }

        struct quadGrav mygrav = {
            gravcenter,
            static_cast<float>(mypoints.size())};
        // insert our gravity into a flat list for easy traversal
        toplevelgrav->push_back(std::move(mygrav));

        // std::vector<Point *> myflatpoints = {};
        // for (Point *point : mypoints)
        // {
        //     myflatpoints.push_back(point);
        //     gravcenter += glm::vec2(point->x, point->y);
        // }
        // int me = toplevelint->size();
        // flattened_points->push_back(std::move(myflatpoints));
    }

    // call physics on all of my points or children
    // void do_physics(size_t tid, int num_threads)
    // {
    //     int proportion = std::floor(static_cast<float>(flattened_points->size()) / static_cast<float>(num_threads));
    //     int mystart = proportion * (tid);
    //     for (int i = 0; i < proportion; i++)
    //     {
    //         int position = i + mystart;
    //         if (position >= flattened_points->size())
    //         {
    //             return;
    //         }

    //         for (Point *point : (*flattened_points)[position])
    //         {
    //             for (Point *other : (*flattened_points)[position])
    //             {
    //                 point->doCollide(*other);
    //             }
    //             for (quadGrav grav : *toplevelgrav)
    //             {
    //                 glm::vec2 pos = glm::vec2(point->x, point->y);
    //                 float d = glm::distance(pos, grav.center);
    //                 if (d > 2.0f)
    //                 {
    //                     glm::vec2 dir = grav.center - pos;
    //                     point->nextv += grav.power / (d * d * d) * dir * 0.001f;
    //                 }
    //             }
    //         }
    //     }

    //     return;
    // }

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

    QuadTree *bot_left;
    QuadTree *bot_right;
    QuadTree *top_left;
    QuadTree *top_right;
};