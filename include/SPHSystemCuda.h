#pragma once
#ifndef SPHSYSTEMCUDA_H
#define SPHSYSTEMCUDA_H

#include <unordered_map>

#include "Camera.h"
#include "Object.h"
#include "Point.h"
//#include "Particle.h"

class FluidParticle;

// Host
class SPHSystemCuda : public Object
{
public:
	SPHSystemCuda(float width, float height, float depth);
	~SPHSystemCuda();
	//void updateHash();

	void simulate();
	virtual void draw(const glm::mat4& P, const glm::mat4& V,
				Light& light, glm::vec3& view_pos, ShadowMap& shadow,
				IrradianceMap& irradiance, PrefilterMap& prefilter, LUTMap& lut);

	virtual void setupFramebuffer(
					const glm::mat4& V, ShadowMap& depth,
					CubeMap& cubemap, Camera& camera);
	virtual void renderProperty();

private:
	void initParticle();
	void initFramebuffer();
	void initShader();
	void computeDepth(const glm::mat4& P, const glm::mat4& V);
	void computeCurvature(const glm::mat4& P, const glm::mat4& V);
	void computeNormal(const glm::mat4& P, const glm::mat4& V, 
						ShadowMap& depth, CubeMap& cubemap);
	void reset();

	vector<shared_ptr<FluidParticle>> m_particles;
	vector<int> m_neighbors;
	vector<int> m_hash;

	unique_ptr<Point> m_point;
	
	unique_ptr<ShadowBuffer> m_fb;
	unique_ptr<ShadowBuffer> m_fb_curvature;
	unique_ptr<ShadowBuffer> m_fb_curvature2;
	unique_ptr<FrameBuffer> m_fb_normal;

	info::SPHParams m_params;

	glm::vec3 m_min_box;
	glm::vec3 m_max_box;

	int m_fb_width;
	int m_fb_height;
	float m_grid_width;
	float m_grid_height;
	float m_grid_depth;
	
	// Parameters for experiment
	int iteration;
	int render_type;
	float t;
	bool m_simulation;
};

#endif // !SPHSYSTEMCUDA_H
