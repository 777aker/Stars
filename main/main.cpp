#include "../window/window.hpp"
#include "point.hpp"
#include "tree.hpp"

#include <vector>
#include <random>

QuadTree *theroot;
std::vector<Point> points = {};
bool pause = false;
bool step = false;

/**
 * @brief respond to key pressed
 *
 * @param windowobj
 * @param key
 * @param scancode
 * @param action
 * @param mods
 */
void key(GLFWwindow *windowobj, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods)
{
	if (action == GLFW_RELEASE)
		return;

	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(windowobj, 1);
		break;
	case GLFW_KEY_SPACE:
		pause = !pause;
		break;
	case GLFW_KEY_ENTER:
		step = true;
		break;
	}
}

void do_physics()
{
	theroot->calcgrav();
	theroot->do_physics();
	for (Point &point : points)
	{
		point.doPhysics(dim, asp);
	}
}

void draw_points()
{
	glPointSize(1000.0f / dim);
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Point), &points[0].x);
	glColorPointer(3, GL_FLOAT, sizeof(Point), &points[0].r);
	glDrawArrays(GL_POINTS, 0, points.size());
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void build_tree()
{
	if (theroot != nullptr)
	{
		delete theroot;
	}

	theroot = new QuadTree(128, glm::vec2(-dim * asp, dim), glm::vec2(dim * asp, -dim));
	for (Point &point : points)
	{
		theroot->insert_point(&point);
	}
}

/**
 * @brief main display loop
 *
 * @param windowobj
 */
void display_loop(Window *windowobj)
{
	while (!glfwWindowShouldClose(windowobj->glwindow))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// want to see fps
		glColor3ub(nephritis.r, nephritis.g, nephritis.b);
		glRasterPos2i(-dim * asp + 0.05 * dim, dim - 0.05 * dim);
		Print("FPS=%d", windowobj->FramesPerSecond());
		glRasterPos2i(-dim * asp + 0.05 * dim, dim - 0.05 * dim - 0.02 * dim);
		Print("Particles=%d", points.size());

		if (!pause || step)
		{
			step = false;
			build_tree();
			// theroot->draw_me();
			do_physics();
		}
		draw_points();

		// check for display errors
		int err = glGetError();
		if (err)
		{
			fprintf(stderr, "ERROR: %s [%s]\n", gluErrorString(err), "display");
		}
		// swap buffers
		glFlush();
		glfwSwapBuffers(windowobj->glwindow);
		// get key board events
		glfwPollEvents();
	}
}

void init_stuff()
{
	std::mt19937 rng(1234);
	std::uniform_int_distribution<int32_t> dim_dist(-dim, dim);
#define P_SPEED 0.005f
	std::normal_distribution<float> v_dist(P_SPEED, P_SPEED / 2.0f);
	for (int i = 0; i < 5000; i++)
	{
		points.push_back(std::move(Point{
			(float)(dim_dist(rng) * asp),
			(float)dim_dist(rng),
			glm::vec2((float)v_dist(rng) - P_SPEED,
					  (float)v_dist(rng) - P_SPEED)}));
	}

	// points.push_back(std::move(Point{
	// 	-50.0f, 0.5f, glm::vec2(0.001f, 0.0f)}));
	// points.push_back(std::move(Point{
	// 	50.0f, 0.0f, glm::vec2(0.0f, 0.0f)}));
}

/**
 * @brief program entry point
 *
 * @param argc
 * @param argv
 * @return int
 */
int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
	Window mainwindow("Collisions", 0, 1000, 1000, key);
	glDisable(GL_DEPTH_TEST);
	glClearColor((float)midnight.r / 255.0, (float)midnight.g / 255.0, (float)midnight.b / 255.0, 1.0);

	init_stuff();
	display_loop(&mainwindow);
}