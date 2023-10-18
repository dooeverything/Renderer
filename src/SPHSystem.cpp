#include "SPHSystem.h"

SPHSystem::SPHSystem(float width, float height, float depth)
{
	m_name = "Fluid";

	cout << endl;
	cout << "*************************Fluid Information**************************" << endl;
	setParticleRadius(0.10f);

	m_simulation = false;
	m_grid_width = width;
	m_grid_height = height;
	m_grid_depth = depth;
	m_fb_width = 1400;
	m_fb_height = 800;

	MASS = 0.02f;
	K = 1.5f;
	rDENSITY = 500.0f;
	VISC = 0.50f;
	WALL = -0.5f;
	SCALE = 1.0f;

	t = 0.0085f;
	render_type = 0;
	iteration = 10;

	int num_particles = int(m_grid_width * m_grid_height * m_grid_depth);
	m_hash_table.reserve(TABLE_SIZE);

	//vector<string> shader_path = { "Shaders/BRDF.vert", "Shaders/BRDF.frag" };
	//m_shader = make_shared<Shader>(shader_path);
	//m_shader->processShader();

	setupFB();
	setupShader();
	
	//cout << "Mesh!!" << endl;
	m_mesh = make_shared<Mesh>();
	initParticles();
	buildHash();
	//createVertex();

	cout << "Number of particles : " << m_particles.size() << endl;
	cout << "********************************end*********************************" << endl;
	cout << endl;
}

uint SPHSystem::getHashIndex(glm::ivec3& pos)
{
	return ((uint)(pos.x * 73856093) ^
			(uint)(pos.y * 19349663) ^
			(uint)(pos.z * 83492791)) % TABLE_SIZE;
}

glm::ivec3 SPHSystem::snapToGrid(glm::vec3 pos)
{
	return { pos.x / H, pos.y / H, pos.z / H };
}

void SPHSystem::setupFB()
{
	cout << "setup fb" << endl;

	m_fb = make_unique<ShadowBuffer>();
	m_fb->createBuffers(m_fb_width, m_fb_height);

	m_fb_blur_x = make_unique<ShadowBuffer>();
	m_fb_blur_x->createBuffers(m_fb_width, m_fb_height);

	m_fb_blur_y = make_unique<ShadowBuffer>();
	m_fb_blur_y->createBuffers(m_fb_width, m_fb_height);

	m_fb_curvature = make_unique<ShadowBuffer>();
	m_fb_curvature->createBuffers(m_fb_width, m_fb_height);

	m_fb_curvature2 = make_unique < ShadowBuffer>();
	m_fb_curvature2->createBuffers(m_fb_width, m_fb_height);

	m_fb_normal = make_unique<FrameBuffer>();
	m_fb_normal->createBuffers(m_fb_width, m_fb_height);
}

void SPHSystem::setupShader()
{
	vector<string> depth_shader = { "assets/shaders/Point.vert", "assets/shaders/Point.frag" };
	m_shader_depth = make_unique<Shader>(depth_shader);
	m_shader_depth->processShader();

	vector<string> blur_shader = { "assets/shaders/Debug.vert", "assets/shaders/Smooth.frag" };
	m_screen = make_unique<GameObject>("assets/models/Debug.txt", blur_shader);

	vector<string> curvature_shader = { "assets/shaders/Debug.vert", "assets/shaders/CurvatureFlow.frag" };
	m_shader_curvature = make_unique<Shader>(curvature_shader);
	m_shader_curvature->processShader();

	vector<string> curvature_normal_shader = { "assets/shaders/Debug.vert", "assets/shaders/CurvatureNormal.frag" };
	m_shader_curvature_normal = make_unique<Shader>(curvature_normal_shader);
	m_shader_curvature_normal->processShader();

	//vector<string> normal_shader = { "Shaders/Debug.vert", "Shaders/Normal.frag" };
	//m_shader_normal = make_unique<Shader>(normal_shader);
	//m_shader_normal->processShader();

	vector<string> render_shader = { "assets/shaders/Debug.vert", "assets/shaders/Render.frag" };
	m_shader_render = make_unique<Shader>(render_shader);
	m_shader_render->processShader();
}

void SPHSystem::initParticles()
{
	srand(1024);
	float particle_seperation = H + 0.03f;
	for (float z = 0.0f; z < m_grid_depth; ++z)
	{
		for (float y = 0.0f; y < m_grid_height*0.5f; ++y)
		{
			for (float x = 0.0f; x < m_grid_width; ++x)
			{
				float ran_x = (float(rand()) / float(RAND_MAX) * 0.5f - 1) * H / 10;
				float ran_y = (float(rand()) / float(RAND_MAX) * 0.5f - 1) * H / 10;
				float ran_z = (float(rand()) / float(RAND_MAX) * 0.5f - 1) * H / 10;
				
				glm::vec3 pos = glm::vec3(
					x * particle_seperation + ran_x - m_grid_width*H / 2.0f,
					y * particle_seperation + ran_y + H + 0.5f,
					z * particle_seperation + ran_z - m_grid_depth*H / 2.0f
				);

				shared_ptr<FluidParticle> f = make_shared<FluidParticle>(pos);
				f->m_weight = rDENSITY;
				m_particles.push_back(f);
			}
		}
	}

	float box_x = m_grid_width * H;
	float box_y = 2 * m_grid_height * H;
	float box_z = m_grid_depth * H;
	cout << box_x << " " << box_y << " " << box_z << endl;

	vector<info::VertexLayout> layouts_box;
	for (float x = -box_x; x <= box_x; x+=box_x*2)
	{
		for (float y = 0; y <= box_y; y+= box_y)
		{
			for (float z = -box_z; z <= box_z; z+= box_z*2)
			{
				info::VertexLayout layout;
				layout.position = glm::vec3(x, y, z);
				layouts_box.push_back(layout);
			}
		}
	}
	m_mesh->getBuffer().createBuffers(layouts_box);
	m_mesh->computeBoundingBox();


	if (m_point == nullptr)
	{
		vector<info::VertexLayout> layouts;
		for (int i = 0; i < m_particles.size(); ++i)
		{
			info::VertexLayout layout;
			layout.position = m_particles[i]->m_position;
			layouts.emplace_back(layout);
		}
		m_point = make_unique<Point>(layouts);	
	}
	else
	{
		vector<info::VertexLayout> layouts = m_point->getMesh().getBuffer().getLayouts();
		for (int i = 0; i < m_particles.size(); ++i)
		{
			info::VertexLayout layout;
			layout.position = m_particles[i]->m_position;
			layouts[i] = layout;
		}
		m_point->getMesh().updateBuffer(layouts);
	}
}

void SPHSystem::update()
{
	if (!m_simulation) return;

	updateDensPress();
	updateForces();

	float box_x = m_grid_width * H;
	float box_z = m_grid_depth * H;
	float box_y = 2 * m_grid_height * H;

	glm::vec3 box = glm::vec3(box_x, box_y, box_z);
	
	setScale(m_property.at(2));
	setRotation(m_property.at(1));
	setPosition(m_property.at(0));

	glm::vec4 b = *m_mesh->getPosition() * *m_mesh->getScale() * glm::vec4(box, 1.0);

	for (int i = 0; i < m_particles.size(); ++i)
	{
		FluidParticle* p = m_particles[i].get();

		p->m_velocity += t * (p->m_force / p->m_density + p->m_gravity);
		p->m_position += t * p->m_velocity;
		
		if (p->m_position.x  > -H + b.x)
		{
			p->m_velocity.x *= WALL;
			p->m_position.x = -H + b.x;
		}
		if (p->m_position.x < H - b.x)
		{
			p->m_velocity.x *= WALL;
			p->m_position.x = H - b.x;
		}

		if (p->m_position.y > -H + b.y)
		{
			p->m_velocity.y *= WALL;
			p->m_position.y = -H + b.y;
		}
		if (p->m_position.y < H)
		{
			p->m_velocity.y *= WALL;
			p->m_position.y = H;
		}

		if (p->m_position.z > -H + b.z)
		{
			p->m_velocity.z *= WALL;
			p->m_position.z = -H + b.z;
		}
		if (p->m_position.z < H - b.z)
		{
			p->m_velocity.z *= WALL;
			p->m_position.z = H - b.z;
		}
	}

	buildHash();
	//updateVertex();
	
	// Update positions in a vertex buffer
	vector<info::VertexLayout> layouts = m_point->getMesh().getBuffer().getLayouts();
	for (int i = 0; i < m_particles.size(); ++i)
	{
		info::VertexLayout layout;
		layout.position = m_particles[i]->m_position;
		layouts[i] = layout;
	}
	m_point->getMesh().updateBuffer(layouts);
}

void SPHSystem::updateDensPress()
{
	for (int i = 0; i < m_particles.size(); ++i)
	{
		float sum = 0.0f;
		FluidParticle* p1 = m_particles[i].get();
		glm::ivec3 grid_pos = snapToGrid(p1->m_position);

		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					glm::ivec3 near_pos = grid_pos + glm::ivec3(x, y, z);
					uint index = getHashIndex(near_pos);
					FluidParticle* p2 = m_hash_table[index];

					while (p2)
					{
						const float r2 = glm::length2(p2->m_position - p1->m_position);
						if (r2 < H2 && p1 != p2)
						{
							sum += float(MASS * POLY6 * pow(H2 - r2, 3));
						}
						p2 = p2->m_next;
					}
				}
			}
		}

		p1->m_density = float(MASS * POLY6 * pow(H, 6)) + sum;
		p1->m_pressure = K * (p1->m_density - rDENSITY);
	}
}

void SPHSystem::updateForces()
{
	for (int i = 0; i < m_particles.size(); ++i)
	{
		FluidParticle* p1 = m_particles[i].get();
		glm::ivec3 grid_pos = snapToGrid(p1->m_position);
		p1->m_force = glm::vec3(0);
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					glm::ivec3 near_pos = grid_pos + glm::ivec3(x, y, z);
					uint index = getHashIndex(near_pos);
					FluidParticle* p2 = m_hash_table[index];
					
					while (p2)
					{
						const float r2 = glm::length2(p2->m_position - p1->m_position);
						if (r2 < H2 && p1 != p2)
						{
							const float r = sqrt(r2);
							glm::vec3 p_dir = glm::normalize(p2->m_position - p1->m_position);
							
							// Calculate Pressrue force
							float W = SPICKY * pow(H - r, 2);
							glm::vec3 a = -p_dir * MASS * (p1->m_pressure + p2->m_pressure) / (2 * p1->m_density);
							glm::vec3 f1 = a * W ;

							// Calculate Viscousity force
							glm::vec3 v_dir = p2->m_velocity - p1->m_velocity;
							float W2 = SPICKY2 * (H - r);
							glm::vec3 b = VISC * MASS * (v_dir / p2->m_density);
							glm::vec3 f2 = b * W2;

							p1->m_force += f1 + f2;
						}
						p2 = p2->m_next;
					}
					
				}
			}
		}
	}
}

void SPHSystem::buildHash()
{
	for (int i = 0; i < TABLE_SIZE; ++i)
	{
		m_hash_table[i] = nullptr;
	}

	for (int i = 0; i < m_particles.size(); ++i)
	{
		FluidParticle* p = m_particles[i].get();
		glm::ivec3 grid_pos = snapToGrid(p->m_position);
		uint index = getHashIndex(grid_pos);

		if (m_hash_table[index] == nullptr)
		{
			p->m_next = nullptr;
			m_hash_table[index] = p;
		}
		else
		{
			p->m_next = m_hash_table[index];
			m_hash_table[index] = p;
		}
	}
}

void SPHSystem::setupFrame(const glm::mat4& V, ShadowMap& depth, CubeMap& cubemap, Camera& camera)
{
	glViewport(0, 0, m_fb_width, m_fb_height);
	getDepth(camera.getSP(), V, camera);
	getCurvature(camera.getP(), V);
	getNormal(camera.getP(), V, depth, cubemap);
}

void SPHSystem::getDepth(const glm::mat4& P, const glm::mat4& V, Camera& camera)
{
	float aspect = float(m_fb_width / m_fb_height);
	float point_scale = m_fb_width / aspect * (1.0f / tanf(glm::radians(45.0f)));

	m_fb->bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		m_point->getShader().load();
		m_point->getShader().setFloat("point_radius", H * SCALE);
		m_point->getShader().setFloat("point_scale", point_scale);
		m_point->drawPoint(P, V);
	m_fb->unbind();
}

void SPHSystem::blurDepth()
{
	glm::vec2 dir_x = glm::vec2(1.0 / m_fb_width, 0.0);
	glm::vec2 dir_y = glm::vec2(0.0, 1.0 / m_fb_height);
	
	m_fb_blur_x->bind();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_screen->getShader()->load();
		glActiveTexture(GL_TEXTURE0);
		m_fb->bindFrameTexture();
		m_screen->getShader()->setInt("map", 0);
		m_screen->getShader()->setVec2("dir", dir_x);
		m_screen->getMesh()->draw();
	m_fb_blur_x->unbind();

	m_fb_blur_y->bind();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_screen->getShader()->load();
		glActiveTexture(GL_TEXTURE0);
		m_fb_blur_x->bindFrameTexture();
		m_screen->getShader()->setInt("map", 0);
		m_screen->getShader()->setVec2("dir", dir_y);
		m_screen->getMesh()->draw();
	m_fb_blur_y->unbind();

	m_fb_blur_x->bind();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_screen->getShader()->load();
		glActiveTexture(GL_TEXTURE0);
		m_fb_blur_y->bindFrameTexture();
		m_screen->getShader()->setInt("map", 0);
		m_screen->getShader()->setVec2("dir", dir_x);
		m_screen->getMesh()->draw();
	m_fb_blur_x->unbind();

	m_fb_blur_y->bind();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_screen->getShader()->load();
		glActiveTexture(GL_TEXTURE0);
		m_fb_blur_x->bindFrameTexture();
		m_screen->getShader()->setInt("map", 0);
		m_screen->getShader()->setVec2("dir", dir_y);
		m_screen->getMesh()->draw();
	m_fb_blur_y->unbind();
}

void SPHSystem::getCurvature(const glm::mat4& P, const glm::mat4& V)
{
	glm::vec2 res = glm::vec2(m_fb_width, m_fb_height);
	m_shader_curvature->load();
	m_shader_curvature->setInt("map", 0);
	m_shader_curvature->setMat4("projection", P);
	m_shader_curvature->setVec2("res", res);

	m_fb_curvature->bind();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		m_fb->bindFrameTexture();
		
		m_screen->getMesh()->draw();
	m_fb_curvature->unbind();

	bool swap = false;
	for (int i = 0; i < iteration; ++i)
	{
		if (swap)
		{
			m_fb_curvature->bind();
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glActiveTexture(GL_TEXTURE0);
				m_fb_curvature2->bindFrameTexture();
				
				m_screen->getMesh()->draw();
			m_fb_curvature->unbind();			
		}
		else
		{
			m_fb_curvature2->bind();
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glActiveTexture(GL_TEXTURE0);
				m_fb_curvature->bindFrameTexture();
				
				m_screen->getMesh()->draw();
			m_fb_curvature2->unbind();
		}

		swap = !swap;
	}
}

void SPHSystem::getNormal(const glm::mat4& P, const glm::mat4& V, ShadowMap& depth, CubeMap& cubemap)
{
	glm::vec2 inverse_tex = glm::vec2(1.0 / m_fb_width, 1.0 / m_fb_height);
	
	m_fb_normal->bind();
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		m_shader_curvature_normal->load();
		
		m_shader_curvature_normal->setInt("map", 0);
		glActiveTexture(GL_TEXTURE0);
		m_fb_curvature2->bindFrameTexture();
		
		m_shader_curvature_normal->setInt("depth_map", 1);
		glActiveTexture(GL_TEXTURE1);
		depth.getBuffer().bindFrameTexture();

		m_shader_curvature_normal->setInt("cubemap", 2);
		glActiveTexture(GL_TEXTURE0 + 2);
		cubemap.getCubemapBuffer()->bindCubemapTexture();
	
		m_shader_curvature_normal->setMat4("projection", P);
		m_shader_curvature_normal->setMat4("view", V);
		m_shader_curvature_normal->setVec2("inverse_tex", inverse_tex);
		m_shader_curvature_normal->setInt("render_type", render_type);
		
		m_screen->getMesh()->draw();
	}
	m_fb_normal->unbind();
}

void SPHSystem::draw()
{
	m_shader_render->load();
	glActiveTexture(GL_TEXTURE0);
	m_fb_normal->bindFrameTexture();
	m_shader_render->setInt("map", 0);
	m_screen->getMesh()->draw();
}

void SPHSystem::reset()
{
	cout << "Reset" << endl;
	
	for (int i = 0; i < TABLE_SIZE; ++i)
	{
		m_hash_table[i] = nullptr;
	}

	for (int i = 0; i < m_particles.size(); ++i)
	{
		m_particles[i] = nullptr;
	}
	m_particles.clear();
	
	int num_particles = int(m_grid_width * m_grid_height * m_grid_depth);
	m_hash_table.reserve(TABLE_SIZE);

	initParticles();
	buildHash();
}
