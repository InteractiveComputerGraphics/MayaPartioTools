
#ifndef PartioVisualizer_H
#define PartioVisualizer_H

#include <maya/MPxLocatorNode.h>
#include <maya/MComputation.h>
#include <maya/MNodeMessage.h>
#include <vector>
#include "extern/partio/src/lib/Partio.h"
#include <maya/MMessage.h>

class Shader;

class PartioVisualizer : public MPxLocatorNode
{
public:
	PartioVisualizer();
	virtual ~PartioVisualizer();

	static void* creator();
	static MStatus initialize();

	virtual void postConstructor();
	virtual MStatus compute( const MPlug& plug, MDataBlock& data );

	virtual void draw( M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);
	virtual MBoundingBox boundingBox() const;
	virtual bool isBounded() const;

public: 
	static MTypeId	id;
	static MString drawDbClassification;
	static MObject m_activeAttr;
	static MObject m_radius;
	static MObject m_minVal;
	static MObject m_maxVal;
	static MObject m_frameIndex;
	static MObject m_color;
	static MObject m_particleFile;
	static MObject m_colorAttrName;
	static MObject m_colorMapType;
	static MObject m_update;
	Partio::ParticlesDataMutable* m_partioData;
	Partio::ParticleAttribute m_posAttr;
	Partio::ParticleAttribute m_userColorAttr;
	std::vector<float> m_dummyVel;
	MBoundingBox m_bbox;

protected:
	MCallbackId m_attrNameCallbackId;
	int m_currentFrame;

	std::string convertFileName(const std::string &inputFileName, const unsigned int currentFrame);
	std::string zeroPadding(const unsigned int number, const unsigned int length);
	bool readParticles(const std::string &fileName, const std::string &attrName);

	static void attribteNameChangedCB(MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void* clientData);
};

#endif
