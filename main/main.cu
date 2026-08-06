#include "../window/window.hpp"
#include "point.hpp"
#include "InitGPUcu.hpp"

#include <vector>
#include <random>
#include <thread>

std::vector<Point> points = {};
std::vector<Point> grid[mydim][mydim] = {};
bool pausephysics = false;
bool step = false;
int GPUThreadsPerBlock = -1;
#define NUM_THREADS 1024
#define NUM_BLOCKS 1

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

__global__ void bruteForce(Point points[], float outputv[])
{
	unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
	Point mypoint = points[tid];
	// for (int i = 0; i < NUM_BLOCKS * NUM_THREADS; i++)
	// {
	// 	Point other = points[i];
	// 	glm::vec2 pos(mypoint.x, mypoint.y);
	// 	glm::vec2 otherpos(other.x, other.y);
	// 	float d = glm::distance(pos, otherpos);
	// 	if (d < 1.0f)
	// 	{
	// 		continue;
	// 	}
	// 	glm::vec2 dir = otherpos - pos;
	// 	dir = glm::normalize(dir);
	// 	if (d > 2.0f)
	// 	{
	// 		mypoint.nextv += 1 / (d * d * d) * dir * 0.001f;
	// 		continue;
	// 	}
	// 	if (glm::dot(dir, other.velocity - mypoint.velocity) > 0.0f)
	// 	{
	// 		continue;
	// 	}
	// 	glm::vec2 otherdir = glm::normalize(pos - otherpos);
	// 	mypoint.nextv -= dir * glm::dot(dir, mypoint.velocity) * 0.8f;
	// 	mypoint.nextv += otherdir * glm::dot(otherdir, other.velocity) * 0.8f;
	// }
	outputv[tid * 2 + 0] = mypoint.nextv.x;
	outputv[tid * 2 + 1] = mypoint.nextv.y;
}

// do the physics step
void do_physics()
{
	Point *cuda_points;
	if (cudaMalloc((void **)&cuda_points, points.size() * sizeof(Point)))
	{
		printf("Could not allocate point memory\n");
		return;
	}
	if (cudaMemcpy(cuda_points, &points, points.size() * sizeof(Point), cudaMemcpyHostToDevice))
	{
		printf("Couldn't copy point memory\n");
		return;
	}
	float *cuda_velocities;
	if (cudaMalloc((void **)&cuda_velocities, points.size() * 2 * sizeof(float)))
	{
		printf("Could not allocate velocity memory\n");
		return;
	}

	dim3 threads(NUM_THREADS, 1);
	dim3 grid(NUM_BLOCKS, 1);
	bruteForce<<<grid, threads>>>(cuda_points, cuda_velocities);

	float *new_velocities = (float *)malloc(points.size() * 2 * sizeof(float));
	if (cudaMemcpy(new_velocities, cuda_velocities, points.size() * 2 * sizeof(float), cudaMemcpyDeviceToHost))
	{
		printf("Couldn't copy velocity memory\n");
		return;
	}
	for (int i = 0; i < points.size(); i++)
	{
		points[i].nextv = glm::vec2(new_velocities[i * 2], new_velocities[i * 2 + 1]);
	}
	cudaFree(cuda_points);
	cudaFree(cuda_velocities);
	free(new_velocities);

	// move every point
	for (Point &point : points)
	{
		point.doPhysics(asp);
	}
}

// draw all the points
void draw_points()
{
	// use the dim to show how big are points are
	// assumes screen size is 1000px
	glPointSize(1000.0f / dim);
	// use efficient memory arrays to draw color and position
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Point), &points[0].x);
	glColorPointer(3, GL_FLOAT, sizeof(Point), &points[0].r);
	glDrawArrays(GL_POINTS, 0, points.size());
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
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
}

// stuff we initialize
void init_stuff()
{
	GPUThreadsPerBlock = InitGPU(1);

	std::mt19937 rng(1234);
	std::uniform_int_distribution<int32_t> dim_dist(-dim, dim);
#define P_SPEED 0.005f
	std::normal_distribution<float> v_dist(P_SPEED, P_SPEED / 2.0f);
	for (int i = 0; i < NUM_BLOCKS * NUM_THREADS; i++)
	{
		points.push_back(std::move(Point{
			(float)(dim_dist(rng) * asp),
			(float)dim_dist(rng),
			glm::vec2((float)v_dist(rng) - P_SPEED,
					  (float)v_dist(rng) - P_SPEED)}));
	}
	// for testing collision logic
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