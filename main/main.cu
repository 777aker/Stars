#include "../window/window.hpp"
#include "point.hpp"
#include "tree.hpp"
#include "InitGPUcu.hpp"

#include <vector>
#include <random>
#include <thread>
#include <iostream>

QuadTree *theroot;
std::vector<Point> points = {};
bool pausephysics = false;
bool step = false;

int GPUThreadsPerBlock = -1;

#define NUM_THREADS 1024
#define NUM_BLOCKS (46 * 3)
#define NUM_PARTICLES (NUM_THREADS * NUM_BLOCKS)
#define QUAD_TREE_SIZE 256

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

__device__ void handleCollide(Point *point, Point other)
{
	glm::vec2 pos(point->x, point->y);
	glm::vec2 otherpos(other.x, other.y);
	float d = glm::distance(pos, otherpos);
	if (d < 1.0f || d > 2.0f)
	{
		return;
	}
	glm::vec2 dir = otherpos - pos;
	dir = glm::normalize(dir);
	if (glm::dot(dir, other.velocity - point->velocity) > 0.0f)
	{
		return;
	}
	glm::vec2 otherdir = glm::normalize(pos - otherpos);
	point->nextv -= dir * glm::dot(dir, point->velocity) * 0.8f;
	point->nextv += otherdir * glm::dot(otherdir, other.velocity) * 0.8f;
	point->r += 0.0001f;
	point->g += 0.05f;
	point->b -= 0.00001f;
}

__global__ void doGPUPhysics(Point points[], flat_info flatted_info[], quadGrav grav[], int gravsize)
{
	unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
	int starting_check = flatted_info[points[tid].myint].starting_index;
	int end_check = starting_check + flatted_info[points[tid].myint].size;
	for (int i = starting_check; i < end_check; i++)
	{
		handleCollide(&points[tid], points[i]);
	}

	for (int i = 0; i < gravsize; i++)
	{
		glm::vec2 pos = glm::vec2(points[tid].x, points[tid].y);
		float d = glm::distance(pos, grav[i].center);
		if (d > 2.0f)
		{
			glm::vec2 dir = grav[i].center - pos;
			points[tid].nextv += grav[i].power / (d * d * d) * dir * 0.001f;
		}
	}
}

void *do_root_physics(size_t tid)
{
	// theroot->do_physics(tid, NUM_THREADS);
	// for (Point *point : *theroot->flattened_points)
	// {
	// 	int starting_check = (*theroot->toplevelint)[point->myint].starting_index;
	// 	int end = starting_check + (*theroot->toplevelint)[point->myint].size;
	// 	for (int i = starting_check; i < end; i++)
	// 	{
	// 		point->doCollide(*(*theroot->flattened_points)[i]);
	// 	}
	// 	for (quadGrav grav : *theroot->toplevelgrav)
	// 	{
	// 		glm::vec2 pos = glm::vec2(point->x, point->y);
	// 		float d = glm::distance(pos, grav.center);
	// 		if (d > 2.0f)
	// 		{
	// 			glm::vec2 dir = grav.center - pos;
	// 			point->nextv += grav.power / (d * d * d) * dir * 0.001f;
	// 		}
	// 	}
	// }

	Point *cuda_points;
	if (cudaMalloc((void **)&cuda_points, NUM_PARTICLES * sizeof(Point)))
	{
		printf("Could not allocate cuda point memory\n");
		return NULL;
	}
	if (cudaMemcpy(cuda_points, theroot->flattened_points->data(), NUM_PARTICLES * sizeof(Point), cudaMemcpyHostToDevice))
	{
		cudaFree(cuda_points);
		printf("Could not copy point data to cuda\n");
		return NULL;
	}
	flat_info *cuda_flatted;
	if (cudaMalloc((void **)&cuda_flatted, theroot->toplevelint->size() * sizeof(flat_info)))
	{
		cudaFree(cuda_points);
		printf("Could not allocate cuda flatted\n");
		return NULL;
	}
	if (cudaMemcpy(cuda_flatted, theroot->toplevelint->data(), theroot->toplevelint->size() * sizeof(flat_info), cudaMemcpyHostToDevice))
	{
		cudaFree(cuda_flatted);
		cudaFree(cuda_points);
		printf("Could not copy cuda flatted\n");
		return NULL;
	}
	quadGrav *cuda_grav;
	if (cudaMalloc((void **)&cuda_grav, theroot->toplevelgrav->size() * sizeof(quadGrav)))
	{
		cudaFree(cuda_flatted);
		cudaFree(cuda_points);
		printf("Could not allocate cuda grav\n");
		return NULL;
	}
	if (cudaMemcpy(cuda_grav, theroot->toplevelgrav->data(), theroot->toplevelgrav->size() * sizeof(quadGrav), cudaMemcpyHostToDevice))
	{
		cudaFree(cuda_grav);
		cudaFree(cuda_flatted);
		cudaFree(cuda_points);
		printf("Could not copy cuda gravity\n");
		return NULL;
	}

	dim3 threads(NUM_THREADS, 1);
	dim3 grid(NUM_BLOCKS, 1);
	doGPUPhysics<<<grid, threads>>>(cuda_points, cuda_flatted, cuda_grav, theroot->toplevelgrav->size());
	cudaError_t err = cudaDeviceSynchronize();
	if (err != cudaSuccess)
	{
		printf("CUDA Error: %s\n", cudaGetErrorString(err));
		return NULL;
	}
	if (cudaMemcpy(&points[0], cuda_points, NUM_PARTICLES * sizeof(Point), cudaMemcpyDeviceToHost))
	{
		printf("Couldn't get points from cuda\n");
	}

	cudaFree(cuda_grav);
	cudaFree(cuda_flatted);
	cudaFree(cuda_points);

	return NULL;
}

// do the physics step
void do_physics()
{
	// first calculate the gravity effect every quad has
	theroot->flatten();
	// resolve collisions and apply gravity to every point
	// std::vector<std::thread *> threads;
	// threads.resize(NUM_THREADS);
	// for (size_t i = 1; i < NUM_THREADS; i++)
	// {
	// 	threads[i] = new std::thread(do_root_physics, i);
	// }
	do_root_physics(0);

	// for (size_t i = 1; i < NUM_THREADS; i++)
	// {
	// 	threads[i]->join();
	// 	delete threads[i];
	// }

	// move every point
	for (Point &point : points)
	{
		point.doPhysics(dim, asp);
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

// build the quad tree
void build_tree()
{
	if (theroot != nullptr)
	{
		delete theroot;
	}

	theroot = new QuadTree(QUAD_TREE_SIZE, glm::vec2(-dim * asp, dim), glm::vec2(dim * asp, -dim), NUM_PARTICLES);
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

		if (!pausephysics || step)
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

// stuff we initialize
void init_stuff()
{
	GPUThreadsPerBlock = InitGPU(1);

	std::mt19937 rng(1234);
	std::uniform_int_distribution<int32_t> dim_dist(-dim, dim);
#define P_SPEED 0.005f
	std::normal_distribution<float> v_dist(P_SPEED, P_SPEED / 2.0f);
	for (int i = 0; i < NUM_PARTICLES; i++)
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