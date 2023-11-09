#include "Renderer.h"
#include "imgui_internal.h"

Renderer::Renderer() :
	m_shadow_map(nullptr), m_click_object(nullptr),
	m_frame_events({}), m_scene_objects({}), m_lights({}), m_panels({}), m_gizmos({}),
	m_is_running(true), m_ticks(0), m_start_time(0), m_is_mouse_down(false),
	m_is_click_gizmo(false), m_mouse_in_panel(false), m_is_moving_gizmo(false), m_is_drag(false)
{
	m_sdl_window = make_unique<SDL_GL_Window>();
	init();
}

Renderer::~Renderer() {}

void Renderer::init()
{
	cout << "Initialize Renderer" << endl;
	m_start_time = SDL_GetTicks64();

	int width = 1400;
	int height = 800;
	m_sdl_window->init(width, height, "Renderer");

	m_framebuffer = make_unique<FrameBuffer>();
	m_framebuffer->createBuffers(1024, 1024);

	m_grid = make_unique<Grid>();
	m_outline = make_unique<Outline>(1024, 1024);

	m_camera = make_unique<Camera>(glm::vec3(0.0f, 7.5f, 27.0f), -90.0f, -11.0f);
	
	glm::vec3 light_pos = { 10.0f, 10.0f, 10.0f };
	m_depth_map = make_unique<ShadowMap>(1024, 1024);
	m_shadow_map = make_unique<ShadowMap>(1024, 1024, light_pos, false);
	m_cubemap = make_unique<CubeMap>(1024, 1024);
	m_irradiancemap = make_unique<IrradianceMap>(32, 32);
	m_prefilter = make_unique<PrefilterMap>(256, 256);
	m_lut = make_unique<LUTMap>(512, 512);

	m_popup = make_unique<PopupPanel>("Popup");
	m_panels.push_back(make_shared<ObjectPanel>("SceneObjects"));
	m_panels.push_back(make_shared<PropertyPanel>("Properties"));
	
	cout << "********************Loading Gizmos********************" << endl;
	for (int axis = 0; axis < 4; ++axis)
	{
		shared_ptr<Gizmo> gizmo = make_shared<Gizmo>(axis);
		m_gizmos.push_back(gizmo);
	}
	cout << "********************Finish loading gizmo********************\n" << endl;

	//shared_ptr<SPHSystemCuda> sph = make_shared<SPHSystemCuda>(64.0f, 32.0f, 64.0f);
	//m_sph = sph;
	//m_scene_objects.push_back(sph);

	// Setup lights
	glm::vec3 dir = -light_pos;
	glm::vec3 amb = { 1.0f, 1.0f, 1.0f };
	glm::vec3 diff = { 0.8f, 0.8f, 0.8f };
	glm::vec3 spec = { 0.5f, 0.5f, 0.5f };
	m_lights.push_back(make_unique<Light>(dir, amb, diff, spec));
}

void Renderer::run()
{
	cout << "Render" << endl;

	// Setup a cubemap
	m_cubemap->drawMap();
	
	// Setup an irradiance map
	m_irradiancemap->drawMap(*m_cubemap->getCubemapBuffer());

	// Setup an prefilter map
	m_prefilter->drawMap(*m_cubemap->getCubemapBuffer());

	// Setup a lut map
	m_lut->drawMap();

	while (m_is_running)
	{
		render();
	}
}

void Renderer::render()
{
	float current_frame = (float)SDL_GetTicks() * 0.001f;
	float last_frame = m_camera->getLastFrame();
	m_camera->setDeltaTime(current_frame - last_frame);
	m_camera->setLastFrame(current_frame);
	m_camera->processInput();

	for (int i = 0; i < m_scene_objects.size(); ++i)
	{
		m_scene_objects.at(i)->setIsClick(false);
		m_scene_objects.at(i)->resetRayHit();
	}

	handleInput();

	// Get shadows
	m_shadow_map->draw(m_scene_objects);
	renderImGui();
	m_sdl_window->swapWindow();
}

void Renderer::handleInput()
{
	SDL_Event event;
	ImGuiIO& io = ImGui::GetIO();
	while (SDL_PollEvent(&event))
	{
		m_frame_events.push_back(event);

		if (!io.WantCaptureMouse)
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			ImVec2 pos = io.MousePos;

			switch (event.type)
			{
				case SDL_QUIT:
					m_is_running = false;
					break;
				
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						int width = event.window.data1;
						int height = event.window.data2;
						m_sdl_window->resizeWindow(width, height);
						cout << "window resize: " << width << " " << height << endl;
					}
					break;

				default:
					break;
			}
			m_frame_events.clear();
		}
		else if(!m_is_click_gizmo)
		{
			ImVec2 pos = io.MousePos;
			ImVec2 scene_min = m_sdl_window->getSceneMin();
			ImVec2 scene_max = m_sdl_window->getSceneMax();
			
			ImGui_ImplSDL2_ProcessEvent(&event);

			if (m_click_object)
			{
				if (m_click_object->getIsPopup())
				{
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						m_frame_events.clear();
						return;
					}
				}
			}

			// if mouse is out of scene, then do nothing
			if (pos.x < scene_min.x || pos.y < scene_min.y)
			{
				m_frame_events.clear();
				return;
			}
			if (pos.x > scene_max.x || pos.y > scene_max.y)
			{
				m_frame_events.clear();
				return;
			}

			switch (event.type)
			{
				case SDL_MOUSEBUTTONUP:
					m_is_mouse_down = false;
					m_is_drag = false;
					m_camera->processMouseUp(event, m_sdl_window.get());
					break;

				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_RIGHT)
					{
						m_is_drag = true;				
						if (m_click_object)
						{
							m_click_object->setIsPopup(true);
						}
					}
					else if (event.button.button == SDL_BUTTON_LEFT)
					{
						m_is_mouse_down = true;
					}

					m_camera->processMouseDown(event);
					m_frame_events.clear();
					return; // To avoid dragging when mouse is clicked

				case SDL_MOUSEMOTION:
					if (m_is_drag)
					{
						if (m_click_object)
						{
							if (m_click_object->getIsPopup())
								break;
						}
						m_camera->processMouseDrag(event);
					}
					break;
			}

			m_frame_events.clear();
		}
	}
}

void Renderer::moveObject(GameObject& go)
{
	SDL_Event event;
	ImGuiIO& io = ImGui::GetIO();
	for(auto& it : m_frame_events)
	{
		event = it;
		ImGui_ImplSDL2_ProcessEvent(&event);		
		switch (event.type)
		{
		case SDL_MOUSEBUTTONUP:
			m_is_mouse_down = false;
			m_is_moving_gizmo = false;
			m_gizmos[go.getMoveAxis()]->setIsClick(false);
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				//cout << "Click gizmo: " << go.getMoveAxis() << endl;
				m_gizmos[go.getMoveAxis()]->setIsClick(true);
				m_is_moving_gizmo = true;
				break;
			}

		case SDL_MOUSEMOTION:
			if (m_gizmos[go.getMoveAxis()]->getIsClick())
			{
				go.move(*m_camera);
				break;
			}
		}
	}

	m_frame_events.clear();
}

void Renderer::renderImGui()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

	bool demo = true;
	ImGui::ShowDemoWindow(&demo);

	// Draw panels
	for (auto& it : m_panels)
		it->render(m_scene_objects, m_click_object);

	bool open = true;
	ImGui::Begin("Scenes", &open, ImGuiWindowFlags_MenuBar);
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Add"))
			{
				ImGuiMenuBar mb("SceneMenuBar");
				mb.render(m_scene_objects, m_click_object);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::BeginChild("SceneRender");
		{
			ImVec2 scene_min = ImGui::GetWindowContentRegionMin();
			ImVec2 scene_max = ImGui::GetWindowContentRegionMax();
			scene_min.x += ImGui::GetWindowPos().x;
			scene_min.y += ImGui::GetWindowPos().y;
			scene_max.x += ImGui::GetWindowPos().x;
			scene_max.y += ImGui::GetWindowPos().y;

			m_sdl_window->setScene(scene_min, scene_max);

			ImVec2 wsize = ImGui::GetWindowSize();
			m_camera->setSceneWidth(wsize.x);
			m_camera->setSceneHeight(wsize.y);
			m_camera->updateSceneProjection();

			m_camera->setWidth(float(m_sdl_window->getWidth()));
			m_camera->setHeight(float(m_sdl_window->getHeight()));
			m_camera->updateProjection();
			m_camera->updateView();

			const glm::mat4& P = m_camera->getSP();
			const glm::mat4& V = m_camera->getV();

			// Get Depth map
			m_depth_map->setProj(P);
			m_depth_map->setView(V);
			m_depth_map->draw(m_scene_objects);

			if (m_sph != nullptr)
			{
				m_sph->setupFramebuffer(V, *m_depth_map, *m_cubemap, *m_camera);
			}

			if (m_click_object != nullptr)
				m_outline->setupBuffers(*m_click_object, V, wsize.x, wsize.y);
			else
				m_outline->clearOutlineFrame();

			m_framebuffer->bind();
			m_sdl_window->clearWindow();
			renderScene();
			m_framebuffer->unbind();

			ImGui::Image((ImTextureID)m_framebuffer->getTextureID(), wsize, ImVec2(0, 1), ImVec2(1, 0));
		}
		ImGui::EndChild();
	}
	ImGui::End();

	// imGui docking
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}
}

void Renderer::renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, m_framebuffer->getWidth(), m_framebuffer->getHeight());

	glm::vec3 cam_pos = m_camera->getPos();

	// Setup world to pixel coordinate transformation 
	const glm::mat4& P = m_camera->getSP();
	const glm::mat4& V = m_camera->getV();

	// Calculate a ray to check whether object is clicked
	int x, y;
	SDL_GetGlobalMouseState(&x, &y);
	int mouse_x = x - int(m_sdl_window->getSceneMin().x);
	int mouse_y = y - int(m_sdl_window->getSceneMin().y);
	m_camera->processPicker(mouse_x, mouse_y);
	glm::vec3 ray_dir = m_camera->getRay();
	glm::vec3 ray_pos = m_camera->getPos();

	// Check whether any object is clicked 
	if (m_is_mouse_down && !m_is_drag && !m_is_click_gizmo)
	{
		for (int i = 0; i < m_scene_objects.size(); ++i)
		{
			m_scene_objects[i]->isClick(ray_dir, ray_pos);
		}

		if (!m_scene_objects.empty())
		{
			std::sort(m_scene_objects.begin(), m_scene_objects.end(),
				[](const shared_ptr<GameObject>& lhs, const shared_ptr<GameObject>& rhs)
				{
					float d1 = lhs->getMesh()->getRayHitMin();
					float d2 = rhs->getMesh()->getRayHitMin();
					return d1 < d2;
				});
		
			if(m_scene_objects.at(0)->getIsClick())
			{
				m_click_object = m_scene_objects.at(0);
			}
			else
			{
				if (m_click_object != nullptr)
				{		
					if(m_click_object->getIsPopup() == false)
						m_click_object = nullptr;
				}
			}
		}

	}

	bool is_popup = false;
	m_popup->popup(m_scene_objects, m_click_object, is_popup, m_is_click_gizmo);

	// Check whether any gizmos is clicked
	if (m_is_moving_gizmo)
	{
		moveObject(*m_click_object);
	}
	else if(m_click_object != nullptr && is_popup == false && !m_is_drag)
	{
		for (int axis = 0; axis < 3; ++axis)
		{
			//cout << "Axis: " << axis << endl;
			if (m_gizmos.at(axis)->isClick(ray_dir, ray_pos))
			{
				m_is_click_gizmo = true;
				m_click_object->setMoveAxis(axis);
				moveObject(*m_click_object);
				break;
			}
			else
			{
				m_click_object->setMoveAxis(-1);
				m_is_click_gizmo = false;
			}
		}
	}

	m_scene_objects.erase(
		remove_if(m_scene_objects.begin(), m_scene_objects.end(),
			[](const shared_ptr<GameObject>& go) {return go->getIsDelete() == true; }
		),
		m_scene_objects.end()
	);

	// Draw objects
	for (int i = 0; i < m_scene_objects.size(); ++i)
	{
		if (m_scene_objects.at(i)->getSoftBodySolver())
		{
			m_scene_objects.at(i)->getSoftBodySolver()->simulate();
		}

		m_scene_objects.at(i)->draw(P, V, *m_lights.at(0), cam_pos,
			*m_shadow_map, *m_irradiancemap, *m_prefilter, *m_lut);
	}

	// Draw background
	m_cubemap->draw(P, V);

	// Draw Grid
	m_grid->draw(P, V, cam_pos);

	if (m_click_object != nullptr)
	{
		// Draw Outline
		glDisable(GL_DEPTH_TEST);
		m_outline->draw(*m_click_object);
		glEnable(GL_DEPTH_TEST);

		// Draw gizmos for clicked objects
		glDisable(GL_DEPTH_TEST);
		for (int axis = 0; axis < 4; ++axis)
			m_gizmos[axis]->draw(*m_click_object, P, V, cam_pos);
		glEnable(GL_DEPTH_TEST);
	}

}

void Renderer::end()
{
	m_sdl_window->unload();
}