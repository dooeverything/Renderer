#pragma once

#ifndef RENDERER_H
#define RENDERER_H


#include "Camera.h"
#include "ImGuiPanel.h"
#include "Light.h"
#include "Map.h"
#include "SDL_GL_Window.h"

class Cloth;
class Gizmo;
class Grid;
class Outline;
class MarchingCube;
class SPHSystemCuda;
class ObjectCollection;

using namespace std;

class Renderer
{
public:
	Renderer();
	~Renderer();

	void init();
	void run();
	void render();

	void handleInput();
	void moveObject(Object& go);
	void renderImGui();
	void renderScene();

	void end();

private:
	vector<SDL_Event> m_frame_events;
	vector<shared_ptr<Light>> m_lights;
	
	unique_ptr<SDL_GL_Window> m_sdl_window;
	unique_ptr<Camera> m_camera;
	unique_ptr<Grid> m_grid;

	unique_ptr<FrameBuffer> m_framebuffer_multi;
	unique_ptr<FrameBuffer> m_scene;

	unique_ptr<ShadowMap> m_depth_map;
	unique_ptr<ShadowMap> m_shadow_map;
	unique_ptr<CubeMap> m_cubemap;
	unique_ptr<IrradianceMap> m_irradiancemap;
	unique_ptr<PrefilterMap> m_prefilter;
	unique_ptr<LUTMap> m_lut;
	unique_ptr<EnvironmentMap> m_environment;
	
	vector<shared_ptr<Gizmo>> m_gizmos;
	vector<shared_ptr<Object>> m_scene_objects;
	shared_ptr<ObjectCollection> m_scene_collections;
	shared_ptr<Object> m_click_object;
	shared_ptr<SPHSystemCuda> m_sph;
	unique_ptr<Outline> m_outline;

	long long m_start_time;
	float m_ticks;
	
	bool m_is_running;
	bool m_is_mouse_down;
	bool m_mouse_in_panel;
	bool m_is_drag;
	bool m_is_click_gizmo;
	bool m_is_moving_gizmo;
	bool m_is_popup = false;

};

#endif