#include "../window/window.hpp"
#include "point.hpp"
#include "InitGPUcu.hpp"

#include <vector>
#include <random>
#include <thread>
#include "cuda.h"
#include "cuda_gl_interop.h"

Point *points;
std::vector<Point> grid[mydim][mydim] = {};
bool pausephysics = false;
bool step = false;
int GPUThreadsPerBlock = -1;
#define NUM_THREADS 1024
#define NUM_BLOCKS 8
#define NUM_POINTS (NUM_BLOCKS * NUM_THREADS)
unsigned int VAO, VBO;
struct cudaGraphicsResource *cuda_vbo_resource;

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
		pausephysics = !pausephysics;
		break;
	case GLFW_KEY_ENTER:
		step = true;
		break;
	}
}

__global__ void bruteForce(Point points[])
{
	unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
	for (int i = 0; i < NUM_BLOCKS * NUM_THREADS; i++)
	{
		Point other = points[i];
		glm::vec2 pos(points[tid].x, points[tid].y);
		glm::vec2 otherpos(other.x, other.y);
		float d = glm::distance(pos, otherpos);
		if (d < 1.0f)
		{
			continue;
		}
		glm::vec2 dir = otherpos - pos;
		dir = glm::normalize(dir);
		if (d > 2.0f)
		{
			points[tid].nextv += 1 / (d * d * d) * dir * 0.001f;
			continue;
		}
		if (glm::dot(dir, other.velocity - points[tid].velocity) > 0.0f)
		{
			continue;
		}
		glm::vec2 otherdir = glm::normalize(pos - otherpos);
		points[tid].nextv -= dir * glm::dot(dir, points[tid].velocity) * 0.8f;
		points[tid].nextv += otherdir * glm::dot(otherdir, other.velocity) * 0.8f;
		points[tid].r += 0.0001f;
		points[tid].g += 0.05f;
		points[tid].b -= 0.00001f;
	}
	// points[tid].x += 0.001f;
}

__global__ void movePoints(Point points[])
{
	unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
	points[tid].velocity = points[tid].nextv;
	points[tid].x += points[tid].velocity.x;
	points[tid].y += points[tid].velocity.y;
}

// do the physics step
void do_physics()
{
	cudaGraphicsMapResources(1, &cuda_vbo_resource, 0);
	Point *point_ptr;
	size_t num_bytes = NUM_POINTS * sizeof(Point);
	cudaGraphicsResourceGetMappedPointer((void **)&point_ptr, &num_bytes, cuda_vbo_resource);
	bruteForce<<<NUM_BLOCKS, NUM_THREADS>>>(point_ptr);
	cudaDeviceSynchronize();
	movePoints<<<NUM_BLOCKS, NUM_THREADS>>>(point_ptr);
	cudaDeviceSynchronize();
	cudaGraphicsUnmapResources(1, &cuda_vbo_resource, 0);
}

// draw all the points
void draw_points()
{
	// use the dim to show how big are points are
	// assumes screen size is 1000px
	glPointSize(1000.0f / dim);
	// use efficient memory arrays to draw color and position

	glDrawArrays(GL_POINTS, 0, NUM_POINTS);
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
		Print("Particles=%d", NUM_POINTS);

		if (!pausephysics || step)
		{
			step = false;
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
	free(points);
	cudaGraphicsUnregisterResource(cuda_vbo_resource);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

// stuff we initialize
void init_stuff()
{
	GPUThreadsPerBlock = InitGPU(1);

	points = (Point *)malloc(NUM_POINTS * sizeof(Point));
	std::mt19937 rng(1234);
	std::uniform_int_distribution<int32_t> dim_dist(-dim, dim);
#define P_SPEED 0.005f
	std::normal_distribution<float> v_dist(P_SPEED, P_SPEED / 2.0f);
	for (int i = 0; i < NUM_POINTS; i++)
	{
		points[i] = std::move(Point{
			(float)(dim_dist(rng) * asp),
			(float)dim_dist(rng),
			glm::vec2((float)v_dist(rng) - P_SPEED,
					  (float)v_dist(rng) - P_SPEED)});
	}
	// for testing collision logic
	// points.push_back(std::move(Point{
	// 	-50.0f, 0.5f, glm::vec2(0.001f, 0.0f)}));
	// points.push_back(std::move(Point{
	// 	50.0f, 0.0f, glm::vec2(0.0f, 0.0f)}));

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, NUM_POINTS * sizeof(Point), points, GL_DYNAMIC_DRAW);
	cudaGraphicsGLRegisterBuffer(&cuda_vbo_resource, VBO, cudaGraphicsRegisterFlagsWriteDiscard);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(VAO);
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