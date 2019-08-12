
#include <GL/glew.h>
#include "PartioVisualizer.h"

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnTransform.h>
#include <maya/MMatrix.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MTime.h>
#include <maya/MEventMessage.h>
#include <maya/MAnimControl.h>
#include <maya/MPlug.h>

#include "extern/partio/src/lib/Partio.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include "Shader.h"
#include <maya/MGlobal.h>
#include <maya/MFnEnumAttribute.h>



MTypeId PartioVisualizer::id(0x82223);
MString PartioVisualizer::drawDbClassification("drawdb/geometry/partioVisualizer");
MObject PartioVisualizer::m_activeAttr;
MObject PartioVisualizer::m_particleFile;
MObject PartioVisualizer::m_minVal;
MObject PartioVisualizer::m_maxVal;
MObject PartioVisualizer::m_frameIndex;
MObject PartioVisualizer::m_colorMapType;
MObject PartioVisualizer::m_radius;
MObject PartioVisualizer::m_color;
MObject PartioVisualizer::m_colorAttrName;
MObject PartioVisualizer::m_update;


PartioVisualizer::PartioVisualizer() : MPxLocatorNode()
{
	m_partioData = nullptr;
	m_bbox.clear();
	m_currentFrame = -1;
}

PartioVisualizer::~PartioVisualizer()
{
 	MStatus stat = MEventMessage::removeCallback(m_attrNameCallbackId);
 	if (stat != MStatus::kSuccess)
 		std::cerr << "Failed remove event callback\n";

	if (m_partioData != nullptr)
		m_partioData->release();
}

void* PartioVisualizer::creator()
{
	return new PartioVisualizer();
}

void PartioVisualizer::postConstructor()
{
	MStatus stat;
	MObject node = thisMObject();
	m_attrNameCallbackId = MNodeMessage::addAttributeChangedCallback(node, attribteNameChangedCB, this, &stat);
	if (stat != MStatus::kSuccess)
		std::cerr << "Failed add event callback\n";
}

MStatus PartioVisualizer::initialize()
{
	MStatus stat;

	MFnNumericAttribute nAttr;
	m_activeAttr = nAttr.create("active", "act", MFnNumericData::kBoolean, 1.0);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(false);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_activeAttr);

	MFnStringData fnStringData;
	MObject defaultString;

	// Create the default string.
	defaultString = fnStringData.create("c:/example/particle_data_###.bgeo");

	MFnTypedAttribute tAttr;
	m_particleFile = tAttr.create("particleFile", "pFile", MFnStringData::kString, defaultString);
	tAttr.setReadable(true);
	tAttr.setWritable(true);
	tAttr.setKeyable(false);
	tAttr.setConnectable(true);
	tAttr.setStorable(true);
	addAttribute(m_particleFile);

	m_radius = nAttr.create("radius", "rad", MFnNumericData::kFloat, 0.025);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(true);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_radius);

	m_frameIndex = nAttr.create("frameIndex", "fIndex", MFnNumericData::kInt, 1);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(true);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_frameIndex);

	defaultString = fnStringData.create("velocity");
	m_colorAttrName = tAttr.create("colorAttributeName", "colorAttr", MFnStringData::kString, defaultString);
	tAttr.setReadable(true);
	tAttr.setWritable(true);
	tAttr.setKeyable(false);
	tAttr.setConnectable(true);
	tAttr.setStorable(true);
	addAttribute(m_colorAttrName);

	m_color = nAttr.createColor("color", "col");
	nAttr.setDefault(0.0f, 0.15f, 1.0f);
	nAttr.setKeyable(true);
	addAttribute(m_color);

	m_minVal = nAttr.create("minVal", "minV", MFnNumericData::kFloat, 0.0);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(true);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_minVal);

	m_maxVal = nAttr.create("maxVal", "maxV", MFnNumericData::kFloat, 20.0);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(true);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_maxVal);

	MFnEnumAttribute enumAttr;
	m_colorMapType = enumAttr.create("colorMapType", "cmt", 0, &stat);
	enumAttr.addField("None", 0);
	enumAttr.addField("Jet", 1);
	enumAttr.addField("Plasma", 2);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(true);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_colorMapType);

	m_update = nAttr.create("update", "upd", MFnNumericData::kInt, 0);
	nAttr.setHidden(true);
	addAttribute(m_update);

	attributeAffects(m_frameIndex, m_update);

	return MStatus::kSuccess;
}

MStatus PartioVisualizer::compute( const MPlug& plug, MDataBlock& block)
{
	if ((plug == m_frameIndex) ||
		(plug == m_update))
	{
		bool active = block.inputValue(m_activeAttr).asBool();
		if (!active)
		{
			block.setClean(plug);
			return MS::kSuccess;
		}

		int frameIndex = block.inputValue(m_frameIndex).asInt();
		if (m_currentFrame != frameIndex)
		{
			MString particleFile = block.inputValue(m_particleFile).asString();
			std::string currentFile = convertFileName(particleFile.asChar(), frameIndex);
			if (currentFile == "")
				return (MS::kFailure);
			MGlobal::displayInfo(MString("Current file: ") + currentFile.c_str());

			MString attrName = block.inputValue(m_colorAttrName).asString();

			readParticles(currentFile, attrName.asChar());
			m_currentFrame = frameIndex;
		}
		return(MS::kSuccess);
	}
	return MStatus::kUnknownParameter;
}

void PartioVisualizer::draw( M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status)
{
	std::cout << "draw\n";
	view.beginGL();
	glPushAttrib( GL_ALL_ATTRIB_BITS );

	// remove all maya transforms
	MFnDagNode me(thisMObject());
	MFnTransform myTransform(me.parent(0));
	MMatrix m = myTransform.transformation().asMatrixInverse();
	glMultMatrixd((double*)&m.matrix[0][0]);

	bool lightingWasOn = glIsEnabled( GL_LIGHTING ) ? true : false;
	if (lightingWasOn) 
		glDisable(GL_LIGHTING);

	// draw stuff

	if (lightingWasOn)
		glEnable(GL_LIGHTING);

	glPopAttrib();
	view.endGL();
}

bool PartioVisualizer::isBounded() const
{
	return true;
}

MBoundingBox PartioVisualizer::boundingBox() const
{
	return m_bbox;
}

std::string PartioVisualizer::zeroPadding(const unsigned int number, const unsigned int length)
{
	std::ostringstream out;
	out << std::internal << std::setfill('0') << std::setw(length) << number;
	return out.str();
}

std::string PartioVisualizer::convertFileName(const std::string &inputFileName, const unsigned int currentFrame)
{
	std::string fileName = inputFileName;
	std::string::size_type pos1 = fileName.find_first_of("#", 0);
	if (pos1 == std::string::npos)
	{
		std::cerr << "# missing in file name.\n";
		return "";
	}
	std::string::size_type pos2 = fileName.find_first_not_of("#", pos1);
	std::string::size_type length = pos2 - pos1;

	std::string numberStr = zeroPadding(currentFrame, (unsigned int)length);
	fileName.replace(pos1, length, numberStr);
	return fileName;
}

bool PartioVisualizer::readParticles(const std::string &fileName, const std::string &attrName)
{
	if (m_partioData != nullptr)
		m_partioData->release();

	m_partioData = Partio::read(fileName.c_str());
	if (!m_partioData)
		return false;

	m_userColorAttr.attributeIndex = -1;
	for (int i = 0; i < m_partioData->numAttributes(); i++)
	{
		Partio::ParticleAttribute attr;
		m_partioData->attributeInfo(i, attr);
		if (attr.name == "position")
			m_posAttr = attr;
		
		if (attr.name == attrName)
			m_userColorAttr = attr;
	}

	if (m_posAttr.attributeIndex != -1)
	{
		m_bbox.clear();
		for (int i = 0; i < m_partioData->numParticles(); i++)
		{
			const float *pos = m_partioData->data<float>(m_posAttr, i);
			MPoint point(pos[0], pos[1], pos[2]);
			m_bbox.expand(point);
		}
	}

	MGlobal::displayInfo(MString("# particles: ") + m_partioData->numParticles());

	return true;
}

void PartioVisualizer::attribteNameChangedCB(MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void* clientData)
{
	if (msg & MNodeMessage::kAttributeSet) 
	{
		PartioVisualizer *visualizer = static_cast<PartioVisualizer*>(clientData);
		if ((plug == PartioVisualizer::m_particleFile) ||
			(plug == PartioVisualizer::m_colorAttrName))
		{			
			bool active = MPlug(visualizer->thisMObject(), PartioVisualizer::m_activeAttr).asBool();
			if (!active)
				return;

			int frameIndex = MPlug(visualizer->thisMObject(), PartioVisualizer::m_frameIndex).asInt();

			MString particleFile = MPlug(visualizer->thisMObject(), PartioVisualizer::m_particleFile).asString();
			std::string currentFile = visualizer->convertFileName(particleFile.asChar(), frameIndex);
			if (currentFile == "")
				return;
			MGlobal::displayInfo(MString("Current file: ") + currentFile.c_str());

			MString attrName = MPlug(visualizer->thisMObject(), PartioVisualizer::m_colorAttrName).asString();

			visualizer->readParticles(currentFile, attrName.asChar());
			visualizer->m_currentFrame = frameIndex;
		}
	}
}
