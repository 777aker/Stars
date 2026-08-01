#include "../window/window.hpp"
#include "point.hpp"

#include <vector>
#include <random>

std::vector<Point> points = {};

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
	}
}

void do_physics()
{
	for (Point &point : points)
	{
		for (Point &others : points)
		{
			point.doGravandCollide(others);
		}
		point.doPhysics(dim, asp);
	}
}

void draw_points()
{
	glPointSize(1.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Point), &points[0].x);
	glDrawArrays(GL_POINTS, 0, points.size());
	glDisableClientState(GL_VERTEX_ARRAY);
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

		do_physics();
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
#define P_SPEED 0.0005f
	std::normal_distribution<float> v_dist(P_SPEED, P_SPEED / 2.0f);
	for (int i = 0; i < 1000; i++)
	{
		points.push_back(std::move(Point{
			(float)(dim_dist(rng) * asp),
			(float)dim_dist(rng),
			(float)v_dist(rng) - P_SPEED,
			(float)v_dist(rng) - P_SPEED}));
	}
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