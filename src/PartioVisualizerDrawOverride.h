
#ifndef PartioVisualizerDrawOverride_H
#define PartioVisualizerDrawOverride_H

#include <maya/MPxDrawOverride.h>
#include <vector>
#include "extern/partio/src/lib/Partio.h"
#include "PartioVisualizer.h"

class Shader;
class PartioVisualizerDrawOverride;

class PartioVisualizerData : public MUserData
{
public:
	PartioVisualizerData() : MUserData(false) { m_vis = nullptr; m_visDO = nullptr; } // don't delete after draw
	~PartioVisualizerData() override {}
	PartioVisualizerDrawOverride *m_visDO;
	PartioVisualizer *m_vis;
	float m_radius;
	float m_color[4];
	float m_minVal;
	float m_maxVal;
	int m_colorMapType;
};

class PartioVisualizerDrawOverride : public MPxDrawOverride
{
public:
	PartioVisualizerDrawOverride(const MObject& obj);
	virtual ~PartioVisualizerDrawOverride();

	static MPxDrawOverride* creator(const MObject& obj);

public: 
	static MTypeId	id;
	static Shader m_shader_vector;
	static Shader m_shader_scalar;
	static Shader m_shader_vector_map;
	static Shader m_shader_scalar_map;
	static GLuint m_textureMap;
	static float const* m_colorMapBuffer;
	static unsigned int m_colorMapLength;

	static void initShaders(MString pluginPath);
	static void deleteShaders();
	static bool pointShaderBegin(const PartioVisualizerData *visData, Shader *shader, float world_view[4][4], float proj[4][4], const bool useTexture);
	static void pointShaderEnd(Shader *shader, const bool useTexture);

protected:
	const MObject m_visObject;

	static void DrawCallback(const MDrawContext& context, const MUserData* data);

	virtual MUserData* prepareForDraw(
		const MDagPath& objPath,
		const MDagPath& cameraPath,
		const MFrameContext& frameContext,
		MUserData* oldData);

	virtual MHWRender::DrawAPI supportedDrawAPIs() const;

	PartioVisualizer* m_visualizer;

	void drawSpheres() const;


};

#endif
