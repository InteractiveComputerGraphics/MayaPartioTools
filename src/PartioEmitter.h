#include <maya/MIOStream.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MPxEmitterNode.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <vector>
#include "extern/partio/src/lib/Partio.h"


#define CheckError(stat, msg)		\
	if ( MS::kSuccess != stat )		\
	{								\
		std::cerr << msg;			\
		return MS::kFailure;		\
	}

class PartioEmitter: public MPxEmitterNode
{
public:
	PartioEmitter();
	~PartioEmitter() override;

	static void		*creator();
	static MStatus	initialize();
	MStatus	compute( const MPlug& plug, MDataBlock& block ) override;

	static MTypeId m_id;
	static MObject m_activeAttr;
	static MObject m_particleFileAttr;
	static MObject m_removeParticlesAttr;
	static MObject m_nucleusFixAttr;
	static MObject m_attrName;
	static MObject m_minVal;
	static MObject m_maxVal;
	static MObject m_frameIndex;


protected:
	Partio::ParticlesDataMutable* m_partioData;
	Partio::ParticleAttribute m_posAttr;
	Partio::ParticleAttribute m_velAttr;
	Partio::ParticleAttribute m_idAttr;
	Partio::ParticleAttribute m_userAttr;
	int m_currentFrame;

	bool readParticles(const std::string &fileName, const std::string &attrName);

	std::string convertFileName(const std::string &inputFileName, const unsigned int currentFrame);
	std::string zeroPadding(const unsigned int number, const unsigned int length);

	float clampValue(const float value, const float min, const float max);
};
