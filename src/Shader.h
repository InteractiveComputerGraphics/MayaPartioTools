#ifndef __SHADER_H__
#define __SHADER_H__

#include <map>
#include <string>
#include "GL/glew.h"

class Shader
{
public:
	Shader();
	~Shader();

	bool compileShaderString(GLenum whichShader, const std::string &source);
	bool compileShaderFile(GLenum whichShader, const std::string &filename);
	bool createAndLinkProgram();
	void addAttribute(const std::string &attribute);
	void addUniform(const std::string &uniform);
	bool isInitialized();

	void begin();
	void end();

	//An indexer that returns the location of the attribute/uniform
	GLuint getAttribute(const std::string &attribute);
	GLuint getUniform(const std::string &uniform);

private:
	bool m_initialized;
	GLuint	m_program;
	std::map<std::string, GLuint> m_attributes;
	std::map<std::string, GLuint> m_uniforms;
	GLuint m_shaders[3];	
};

#endif