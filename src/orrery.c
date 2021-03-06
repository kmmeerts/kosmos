#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <allegro5/allegro.h>
#include <ralloc.h>

#include "mathlib.h"
#include "shader.h"
#include "glm.h"
#include "camera.h"
#include "mesh.h"
#include "solarsystem.h"
#include "render.h"
#include "input.h"
#include "util.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846L
#endif

static void calcfps(void);
int init_allegro(Camera *cam);

ALLEGRO_DISPLAY *dpy;
ALLEGRO_EVENT_QUEUE *ev_queue = NULL;

Vec3 light_pos = {0, 0, 0};
GLfloat light_ambient[3] = {0.25, 0.20725, 0.20725};
GLfloat light_diffuse[3] = {1, 0.829, 0.829};
GLfloat light_specular[3] = {0.296648, 0.296648, 0.296648};

SolarSystem *solsys;

double t = 0;

int init_allegro(Camera *cam)
{
	if (!al_init())
		return 1;
	if (!al_install_mouse())
		return 1;
	if (!al_install_keyboard())
		return 1;
	
	al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_OPENGL | ALLEGRO_OPENGL_FORWARD_COMPATIBLE | ALLEGRO_RESIZABLE);
	
	al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);

	dpy = al_create_display(cam->left + cam->width, cam->bottom + cam->height);
	glViewport(cam->left, cam->bottom, cam->width, cam->height);

	ev_queue = al_create_event_queue();
	al_register_event_source(ev_queue, al_get_display_event_source(dpy));
	al_register_event_source(ev_queue, al_get_mouse_event_source());
	al_register_event_source(ev_queue, al_get_keyboard_event_source());

	return 0;
}

static void calcfps()
{
	const double SAMPLE_TIME = 0.250;
	static int frames;
	static double tock=0.0;
	double tick;
	char string[64];

	frames++;

	tick = al_get_time();
	if (tick - tock > SAMPLE_TIME)
	{
		snprintf(string, 64, "%d FPS", (int) (frames/(tick - tock) + 0.5));
		al_set_window_title(dpy, string);

		frames = 0;
		tock = tick;
	}
}

int main(int argc, char **argv)
{
	int i;
	Light light;
	const char *filename;
	Camera cam;
	Vec3 position = {0, 0, 150e9};
	Vec3 up =  {0, 1, 0};
	Vec3 target = {0, 0, 0};
	Shader *shader_light;
	Shader *shader_simple;
	Mesh *mesh;
	Renderable planet;
	if (argc < 2)
		filename = STRINGIFY(ROOT_PATH) "/data/teapot.ply";
	else
		filename = argv[1];

	solsys = solsys_load(STRINGIFY(ROOT_PATH) "/data/sol.ini");
	if (solsys == NULL)
		return 1;

	mesh = mesh_import(filename);
	if (mesh == NULL)
		return 1;
	for (i = 0; i < mesh->num_vertices; i++) /* Blow up the teapot */
	{
		mesh->vertex[i].x = mesh->vertex[i].x * 100;
		mesh->vertex[i].y = mesh->vertex[i].y * 100;
		mesh->vertex[i].z = mesh->vertex[i].z * 100;
	}

	cam.fov = M_PI/4;
	cam.left = 0;
	cam.bottom = 0;
	cam.width = 1024;
	cam.height = 768;
	cam.zNear = 1e6;
	cam.zFar = 4.5e15;
	init_allegro(&cam);
	cam_lookat(&cam, position, target, up);

	glewInit();

	shader_light = shader_create(STRINGIFY(ROOT_PATH) "/data/lighting.v.glsl", 
	                             STRINGIFY(ROOT_PATH) "/data/lighting.f.glsl");
	if (shader_light == NULL)
		return 1;
	
	shader_simple = shader_create(STRINGIFY(ROOT_PATH) "/data/simple.v.glsl", 
	                              STRINGIFY(ROOT_PATH) "/data/simple.f.glsl");
	if (shader_simple == NULL)
		return 1;

	glmProjectionMatrix = glmNewMatrixStack();
	glmViewMatrix = glmNewMatrixStack();
	glmModelMatrix = glmNewMatrixStack();

	light.position = light_pos;
	memcpy(light.ambient, light_ambient, sizeof(light_ambient));
	memcpy(light.diffuse, light_diffuse, sizeof(light_diffuse));
	memcpy(light.specular, light_specular, sizeof(light_specular));

	glClearColor(20/255., 30/255., 50/255., 1.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glPointSize(2);

	planet.data = mesh;
	planet.upload_to_gpu = mesh_upload_to_gpu;
	planet.render = mesh_render;
	planet.shader = shader_light;
	renderable_upload_to_gpu(&planet);

	/* Transformation matrices */
	cam_projection_matrix(&cam, glmProjectionMatrix);

	/* Start rendering */
	while(handle_input(ev_queue, &cam))
	{
		void *ctx;
		Entity *renderlist, *prev;

		t += 365*86400;

		/* Physics stuff */
		solsys_update(solsys, t);
		ctx = ralloc_context(NULL);

		prev = NULL;
		for (i = 0; i < solsys->num_bodies; i++)
		{
			Entity *e;

			e = ralloc(ctx, Entity);
			e->orientation = (Quaternion) {1, 0, 0, 0};
			e->renderable = &planet;
			e->position = solsys->body[i].position;
			e->radius = solsys->body[i].radius;
			e->prev = prev;
			e->next = NULL;
			if (prev != NULL)
				e->prev->next = e;
			prev = e;

			if (i == 0)
				renderlist = e;
		}

		/* Rendering stuff */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glmLoadIdentity(glmModelMatrix);
		glmLoadIdentity(glmViewMatrix);
		cam_view_matrix(&cam, glmViewMatrix); /* view */

		light_upload_to_gpu(&light, shader_light);

		render_entity_list(renderlist);

		al_flip_display();
		calcfps();

		ralloc_free(ctx);
	}

	ralloc_free(mesh);
	ralloc_free(solsys);

	shader_delete(shader_light);
	shader_delete(shader_simple);
	glmFreeMatrixStack(glmProjectionMatrix);
	glmFreeMatrixStack(glmViewMatrix);
	glmFreeMatrixStack(glmModelMatrix);
	al_destroy_display(dpy);
	return 0;
}

