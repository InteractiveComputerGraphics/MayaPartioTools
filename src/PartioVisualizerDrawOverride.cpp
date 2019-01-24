
#include <GL/glew.h>
#include "PartioVisualizerDrawOverride.h"

#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include "Shader.h"
#include <maya/MDrawContext.h>
#include <maya/MFnDependencyNode.h>

#include "colormap_jet.h"
#include "colormap_plasma.h"


MTypeId PartioVisualizerDrawOverride::id(0x81113);
Shader PartioVisualizerDrawOverride::m_shader_vector;
Shader PartioVisualizerDrawOverride::m_shader_scalar;
Shader PartioVisualizerDrawOverride::m_shader_vector_map;
Shader PartioVisualizerDrawOverride::m_shader_scalar_map;
GLuint PartioVisualizerDrawOverride::m_textureMap;
float const* PartioVisualizerDrawOverride::m_colorMapBuffer;


PartioVisualizerDrawOverride::PartioVisualizerDrawOverride(const MObject& obj) : MPxDrawOverride(obj, DrawCallback), m_visObject(obj)
{
	m_visualizer = nullptr;
	MStatus status;
	MFnDependencyNode dnode(m_visObject, &status);
	if (status)
		m_visualizer = dynamic_cast<PartioVisualizer*>(dnode.userNode());
}

PartioVisualizerDrawOverride::~PartioVisualizerDrawOverride()
{
}

MPxDrawOverride* PartioVisualizerDrawOverride::creator(const MObject& obj)
{
	return new PartioVisualizerDrawOverride(obj);
}

MUserData* PartioVisualizerDrawOverride::prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath,
	const MFrameContext& frameContext, MUserData* oldData)
{
	// Retrieve data cache (create if does not exist)
	PartioVisualizerData* data = dynamic_cast<PartioVisualizerData*>(oldData);
	if (!data)
	{
		data = new PartioVisualizerData();
	}

	// Force update of the data
	MObject upd;
	MPlug frameIndexPlug(m_visObject, PartioVisualizer::m_update);
	if (!frameIndexPlug.isNull())
		frameIndexPlug.getValue(upd);

	data->m_vis = m_visualizer;
	data->m_visDO = this;
	data->m_radius = MPlug(m_visObject, PartioVisualizer::m_radius).asFloat();
	data->m_minVal = MPlug(m_visObject, PartioVisualizer::m_minVal).asFloat();
	data->m_maxVal = MPlug(m_visObject, PartioVisualizer::m_maxVal).asFloat();
	data->m_colorMapType = MPlug(m_visObject, PartioVisualizer::m_colorMapType).asInt();
	MPlug col_plug(m_visObject, PartioVisualizer::m_color);
	data->m_color[0] = col_plug.child(0).asFloat();
	data->m_color[1] = col_plug.child(1).asFloat();
	data->m_color[2] = col_plug.child(2).asFloat();
	data->m_color[3] = 1.0f;

	return data;
}

void PartioVisualizerDrawOverride::initShaders(MString pluginPath)
{
	glewInit();

	//////////////////////////////////////////////////////////////////////////
	MString vertFile = pluginPath + "/vs_points_vector.glsl";
	MString fragFile = pluginPath + "/fs_points.glsl";
	if (!m_shader_vector.compileShaderFile(GL_VERTEX_SHADER, vertFile.asChar()))
		return;
	if (!m_shader_vector.compileShaderFile(GL_FRAGMENT_SHADER, fragFile.asChar()))
		return;
	if (!m_shader_vector.createAndLinkProgram())
		return;
	m_shader_vector.begin();
	m_shader_vector.addUniform("modelview_matrix");
	m_shader_vector.addUniform("projection_matrix");
	m_shader_vector.addUniform("radius");
	m_shader_vector.addUniform("viewport_width");
	m_shader_vector.addUniform("color");
	m_shader_vector.addUniform("min_scalar");
	m_shader_vector.addUniform("max_scalar");
	m_shader_vector.end();

	MString vertFileScalar = pluginPath + "/vs_points_scalar.glsl";
	if (!m_shader_scalar.compileShaderFile(GL_VERTEX_SHADER, vertFileScalar.asChar()))
		return;
	if (!m_shader_scalar.compileShaderFile(GL_FRAGMENT_SHADER, fragFile.asChar()))
		return;
	if (!m_shader_scalar.createAndLinkProgram())
		return;
	m_shader_scalar.begin();
	m_shader_scalar.addUniform("modelview_matrix");
	m_shader_scalar.addUniform("projection_matrix");
	m_shader_scalar.addUniform("radius");
	m_shader_scalar.addUniform("viewport_width");
	m_shader_scalar.addUniform("color");
	m_shader_scalar.addUniform("min_scalar");
	m_shader_scalar.addUniform("max_scalar");
	m_shader_scalar.end();

	MString fragFileMap = pluginPath + "/fs_points_colormap.glsl";
	if (!m_shader_vector_map.compileShaderFile(GL_VERTEX_SHADER, vertFile.asChar()))
		return;
	if (!m_shader_vector_map.compileShaderFile(GL_FRAGMENT_SHADER, fragFileMap.asChar()))
		return;
	if (!m_shader_vector_map.createAndLinkProgram())
		return;
	m_shader_vector_map.begin();
	m_shader_vector_map.addUniform("modelview_matrix");
	m_shader_vector_map.addUniform("projection_matrix");
	m_shader_vector_map.addUniform("radius");
	m_shader_vector_map.addUniform("viewport_width");
	m_shader_vector_map.addUniform("color");
	m_shader_vector_map.addUniform("min_scalar");
	m_shader_vector_map.addUniform("max_scalar");
	m_shader_vector_map.end();

	if (!m_shader_scalar_map.compileShaderFile(GL_VERTEX_SHADER, vertFileScalar.asChar()))
		return;
	if (!m_shader_scalar_map.compileShaderFile(GL_FRAGMENT_SHADER, fragFileMap.asChar()))
		return;
	if (!m_shader_scalar_map.createAndLinkProgram())
		return;
	m_shader_scalar_map.begin();
	m_shader_scalar_map.addUniform("modelview_matrix");
	m_shader_scalar_map.addUniform("projection_matrix");
	m_shader_scalar_map.addUniform("radius");
	m_shader_scalar_map.addUniform("viewport_width");
	m_shader_scalar_map.addUniform("color");
	m_shader_scalar_map.addUniform("min_scalar");
	m_shader_scalar_map.addUniform("max_scalar");
	m_shader_scalar_map.end();

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &m_textureMap);

}

void PartioVisualizerDrawOverride::deleteShaders()
{
	glDeleteTextures(1, &m_textureMap);
}

MHWRender::DrawAPI PartioVisualizerDrawOverride::supportedDrawAPIs() const
{
#if MAYA_API_VERSION >= 201600
	return MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile;
#else
	return MHWRender::kOpenGL;
#endif
}

void PartioVisualizerDrawOverride::drawSpheres() const
{
	// Draw simulation model
	const unsigned int nParticles = m_visualizer->m_partioData->numParticles();
	if (nParticles == 0)
		return;

	const float* partioPositions = m_visualizer->m_partioData->data<float>(m_visualizer->m_posAttr, 0);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, &partioPositions[0]);

	const float* partioVals = NULL;
	glEnableVertexAttribArray(1);

	if ((m_visualizer->m_userAttr.attributeIndex != -1) && (m_visualizer->m_userAttr.type == Partio::VECTOR))
	{
		partioVals = m_visualizer->m_partioData->data<float>(m_visualizer->m_userAttr, 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, &partioVals[0]);
	}
	else if ((m_visualizer->m_userAttr.attributeIndex != -1) && (m_visualizer->m_userAttr.type == Partio::FLOAT))
	{
		partioVals = m_visualizer->m_partioData->data<float>(m_visualizer->m_userAttr, 0);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, &partioVals[0]);
	}
	else
	{
		m_visualizer->m_dummyVel.resize(nParticles, 0.0f);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, &m_visualizer->m_dummyVel[0]);
	}

	glDrawArrays(GL_POINTS, 0, nParticles);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void PartioVisualizerDrawOverride::DrawCallback(const MDrawContext& context, const MUserData* data)
{
	const PartioVisualizerData *visData = reinterpret_cast<const PartioVisualizerData*>(data);

	const bool draw_bounding_box = context.getDisplayStyle() & MHWRender::MFrameContext::kBoundingBox;

	MStatus status;
	MBoundingBox frustrum_box = context.getFrustumBox(&status);
	if (status)
	{
		if (!frustrum_box.intersects(visData->m_vis->m_bbox))
			return;
	}

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	float world_view[4][4];
	MMatrix world_view_matrix = context.getMatrix(MHWRender::MDrawContext::kWorldViewMtx);
	world_view_matrix.get(world_view);
	float proj[4][4];
	context.getMatrix(MHWRender::MDrawContext::kProjectionMtx).get(proj);

	GLint current_program = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);

	Shader *shader_vector = &m_shader_vector_map;
	Shader *shader_scalar = &m_shader_scalar_map;
	bool useTexture = true;
	if (visData->m_colorMapType == 0)
	{
		shader_vector = &m_shader_vector;
		shader_scalar = &m_shader_scalar;
		useTexture = false;
	}

	bool chk = false;	
	if ((visData->m_vis->m_userAttr.attributeIndex != -1) && (visData->m_vis->m_userAttr.type == Partio::VECTOR))
		chk = pointShaderBegin(visData, shader_vector, world_view, proj, useTexture);
	else
		chk = pointShaderBegin(visData, shader_scalar, world_view, proj, useTexture);

	if (chk)
	{
		visData->m_visDO->drawSpheres();
		
		if ((visData->m_vis->m_userAttr.attributeIndex != -1) && (visData->m_vis->m_userAttr.type == Partio::VECTOR))
			pointShaderEnd(shader_vector, useTexture);
		else
			pointShaderEnd(shader_scalar, useTexture);
	}

	glUseProgram(current_program);

	glPopClientAttrib();
	glPopAttrib();
}


bool PartioVisualizerDrawOverride::pointShaderBegin(const PartioVisualizerData *visData, Shader *shader, float world_view[4][4], float proj[4][4], const bool useTexture)
{
	if (!shader->isInitialized())
		return false;
	shader->begin();

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	const float radius = visData->m_radius;
	glUniform1f(shader->getUniform("viewport_width"), (float)viewport[2]);
	glUniform1f(shader->getUniform("radius"), (float)radius);
	glUniform1f(shader->getUniform("min_scalar"), visData->m_minVal);
	glUniform1f(shader->getUniform("max_scalar"), visData->m_maxVal);
	glUniform3fv(shader->getUniform("color"), 1, visData->m_color);

	if (visData->m_colorMapType == 1)
		m_colorMapBuffer = colormap_jet[0];
	else if (visData->m_colorMapType == 2)
		m_colorMapBuffer = colormap_plasma[0];

	if (useTexture)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, m_textureMap);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 256u, 0, GL_RGB, GL_FLOAT,
			reinterpret_cast<float const*>(m_colorMapBuffer));
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glGenerateMipmap(GL_TEXTURE_1D);
	}


	glUniformMatrix4fv(shader->getUniform("modelview_matrix"), 1, GL_FALSE, &world_view[0][0]);
	glUniformMatrix4fv(shader->getUniform("projection_matrix"), 1, GL_FALSE, &proj[0][0]);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POINT_SPRITE);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glPointParameterf(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
	return true;
}

void PartioVisualizerDrawOverride::pointShaderEnd(Shader *shader, const bool useTexture)
{
	glBindTexture(GL_TEXTURE_1D, 0);
	shader->end();
}

