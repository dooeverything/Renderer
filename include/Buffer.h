#pragma once

#ifndef BUFFER_H
#define BUFFER_H

#include <GL/glew.h>
#include <SDL.h>

#include "Utils.h"

class VertexBuffer
{
private:
	enum attribs
	{
		POS_ATTRIB = 0,
		NORMAL_ATTRIB = 1,
		TANGENT_ATTRIB = 2,
		TEXCOORD_ATTRIB = 3,
		BONE_ATTRIB = 4,
		BONE_WEIGHT_ATTRIB = 5,
	};

public:
	VertexBuffer();
	void createBuffers(const vector<info::VertexLayout>& layouts);
	void createBuffers(const vector<info::VertexLayout>& layouts, const vector<unsigned int>& indices);
	void createBuffers(const vector<info::VertexLayout>& layouts, const vector<glm::vec3>& colors);
	void updateBuffer(const vector<info::VertexLayout>& layouts);
	void updateBuffer(const vector<info::VertexLayout>& layouts, const vector<glm::vec3>& colors);
	void bind() const;
	void unbind() const;
	
	void getBBoxBoundary(const glm::mat4& M, glm::vec3& bbox_min, glm::vec3& bbox_max);

	inline vector<info::VertexLayout>& getLayouts() { return m_layouts; };
	inline vector<unsigned int> getIndices() { return m_indices; };
	inline unsigned int getSizeOfIndices() { return n_indices; };
	inline unsigned int getSizeOfInstance() { return unsigned int(m_matrices.size()); };

	inline void setLayouts(vector<info::VertexLayout> layouts)
	{
		m_layouts = layouts;
		n_layouts = layouts.size();
	}
	inline void setMatrices(vector<glm::mat4> ms) { m_matrices = ms; };

private:
	GLuint m_VAO;
	GLuint m_VBO;
	GLuint m_EBO;
	GLuint m_IBO;
	GLuint m_CBO;

	vector<info::VertexLayout> m_layouts;
	vector<unsigned int> m_indices;
	vector<glm::mat4> m_matrices;
	int n_layouts;
	int n_indices;
};

class FrameBuffer
{
public:
	FrameBuffer();
	virtual void createBuffers(int width, int height, bool multisample = false);
	
	void bind() const;
	void bindDraw();
	void bindRead();
	void unbind() const;
	void bindFrameTexture();
	void rescaleFrame(int width, int height);

	GLuint getTextureID() const { return m_framebuffer_texture; };
	int getWidth() const { return m_width; };
	int getHeight() const { return m_height; };

protected:
	GLuint m_FBO;
	GLuint m_RBO;
	GLuint m_framebuffer_texture;

	int m_width;
	int m_height;
};

class ShadowBuffer : public FrameBuffer
{
public:
	ShadowBuffer();
	~ShadowBuffer();

	virtual void createBuffers(int width, int height, bool multisample = false);
};

class CubemapBuffer : public FrameBuffer
{
public:
	CubemapBuffer();
	~CubemapBuffer();

	virtual void createBuffers(int width, int height, bool mipmap);
	virtual void bindFrameTexture(int i);
	virtual void bindMipMapTexture(int i, int mip);
	void bindCubemapTexture();
	void bindRenderBuffer(int width, int height);
	virtual inline unsigned int getCubemapTexture() { return m_cubemap; };

private:
	unsigned int m_cubemap;
};

#endif