#include "Material.h"

Material::Material() :
	m_texture(nullptr)
{
	m_type = "";
	m_base_color = { 0.8f, 0.8f, 0.8f };
	m_metallic = 1.0f;
	m_roughness = 0.5f;
	m_ao = 1.0f;
}

Material::~Material()
{}

void Material::addTexture(const string& path)
{
	m_texture = make_shared<Texture>(path);
	m_texture->loadTexture();
}





