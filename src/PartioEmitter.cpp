#include <math.h>
#include <algorithm>
#include <stdlib.h>

#include "PartioEmitter.h"

#include <maya/MVectorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MArrayDataBuilder.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnMatrixData.h>

#include <maya/MFnParticleSystem.h>
#include <maya/MFnStringData.h>
#include <maya/MPlugArray.h>
#include <maya/MFnTypedAttribute.h>

#include "extern/partio/src/lib/Partio.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <maya/MGlobal.h>
#include <maya/MNodeMessage.h>
#include <maya/MAnimControl.h>


MTypeId PartioEmitter::m_id( 0x82222 );
MObject PartioEmitter::m_activeAttr;
MObject PartioEmitter::m_particleFileAttr;
MObject PartioEmitter::m_removeParticlesAttr;
MObject PartioEmitter::m_nucleusFixAttr;
MObject PartioEmitter::m_colorAttrName;
MObject PartioEmitter::m_rotationAttrName;
MObject PartioEmitter::m_minVal;
MObject PartioEmitter::m_maxVal;
MObject PartioEmitter::m_frameIndex;

PartioEmitter::PartioEmitter()
{
	m_partioData = nullptr;
	m_currentFrame = -1;
}


PartioEmitter::~PartioEmitter()
{
	if (m_partioData != nullptr)
		m_partioData->release();
}


void *PartioEmitter::creator()
{
    return new PartioEmitter;
}


MStatus PartioEmitter::initialize()
{
	MFnTypedAttribute tAttr;
	MFnNumericAttribute nAttr;

	MFnStringData fnStringData;
	MObject defaultString;

	// Create the default string.
	defaultString = fnStringData.create("c:/example/particle_data_###.bgeo");

	m_activeAttr = nAttr.create("active", "act", MFnNumericData::kBoolean, 1.0);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(false);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_activeAttr);

	m_particleFileAttr = tAttr.create("particleFile", "pFile", MFnStringData::kString, defaultString);
	tAttr.setReadable(true);
	tAttr.setWritable(true);
	tAttr.setKeyable(false);
	tAttr.setConnectable(true);
	tAttr.setStorable(true);
	addAttribute(m_particleFileAttr);

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

	defaultString = fnStringData.create("rotation");
	m_rotationAttrName = tAttr.create("rotationAttributeName", "rotAttr", MFnStringData::kString, defaultString);
	tAttr.setReadable(true);
	tAttr.setWritable(true);
	tAttr.setKeyable(false);
	tAttr.setConnectable(true);
	tAttr.setStorable(true);
	addAttribute(m_rotationAttrName);

	m_removeParticlesAttr = tAttr.create("removeParticles", "rmParticles", MFnStringData::kString);
	tAttr.setReadable(true);
	tAttr.setWritable(true);
	tAttr.setKeyable(false);
	tAttr.setConnectable(true);
	tAttr.setStorable(true);
	addAttribute(m_removeParticlesAttr);

	m_nucleusFixAttr = nAttr.create("nucleusFix", "nucF", MFnNumericData::kBoolean, 1.0);
	nAttr.setReadable(true);
	nAttr.setWritable(true);
	nAttr.setKeyable(false);
	nAttr.setConnectable(true);
	nAttr.setStorable(true);
	addAttribute(m_nucleusFixAttr);

	attributeAffects(m_particleFileAttr, mOutput);
	attributeAffects(m_colorAttrName, mOutput);
	attributeAffects(m_minVal, mOutput);
	attributeAffects(m_maxVal, mOutput);
	attributeAffects(m_rotationAttrName, mOutput);
	attributeAffects(m_frameIndex, mOutput);
	attributeAffects(m_removeParticlesAttr, mOutput);

	return( MS::kSuccess );
}


MStatus PartioEmitter::compute(const MPlug& plug, MDataBlock& block)
{
	MStatus status;

	if (plug != mOutput)
        return( MS::kUnknownParameter );

	MString particleFile = block.inputValue(m_particleFileAttr).asString();

	if (particleFile == "")
		return (MS::kFailure);

	bool active = block.inputValue(m_activeAttr).asBool();
	if (!active)
	{
		block.setClean(plug);
		return MS::kSuccess;
	}

	// find list of particles that should be removed from string separated by semicolon
	MString rmParticlesStr = block.inputValue(m_removeParticlesAttr).asString();
	MStringArray strArray;
	rmParticlesStr.split(';', strArray);
	std::vector<int> removeParticles;
	removeParticles.resize(strArray.length());
	for (unsigned int i=0; i < strArray.length(); i++)
		removeParticles[i] = strArray[i].asInt();

	MPlugArray connectionArray;
	plug.connectedTo(connectionArray, false, true, &status);

	if (connectionArray.length() == 0)
		return (MS::kFailure);

	MPlug particleShapeOutPlug = connectionArray[0];
	MObject particleShapeNode = particleShapeOutPlug.node(&status);
	MFnParticleSystem part(particleShapeNode, &status);

	MArrayDataHandle hOutArray = block.outputArrayValue( mOutput, &status);
	CheckError(status, "ERROR in hOutArray = block.outputArrayValue.\n");

	int multiIndex = plug.logicalIndex(&status);
	CheckError(status, "ERROR in plug.logicalIndex.\n");

	MArrayDataBuilder bOutArray = hOutArray.builder( &status );
	CheckError(status, "ERROR in bOutArray = hOutArray.builder.\n");

	MDataHandle hOut = bOutArray.addElement(multiIndex, &status);
	CheckError(status, "ERROR in hOut = bOutArray.addElement.\n");

	MFnArrayAttrsData fnOutput;
	MObject dOutput = fnOutput.create ( &status );
	CheckError(status, "ERROR in fnOutput.create.\n");

	MVectorArray fnOutPos = fnOutput.vectorArray("position", &status);
	MVectorArray fnOutVel = fnOutput.vectorArray("velocity", &status);
	MDoubleArray fnOutUserScalarColor = fnOutput.doubleArray("userScalarColor", &status);
	MVectorArray fnOutUserVectorRotation = fnOutput.vectorArray("userVectorRotation", &status);

	int frameIndex = block.inputValue(m_frameIndex).asInt();

	std::string currentFile = convertFileName(particleFile.asChar(), frameIndex);
	if (currentFile == "")
		return (MS::kFailure);
	std::cout << "Current file: " << currentFile << "\n";

	bool nucleusFix = block.inputValue(m_nucleusFixAttr).asBool();

	MString colorAttrName = block.inputValue(m_colorAttrName).asString();
	MString rotationAttrName = block.inputValue(m_rotationAttrName).asString();
	readParticles(currentFile, colorAttrName.asChar(), rotationAttrName.asChar());

	float minVal = block.inputValue(m_minVal).asFloat(); 
	float maxVal = block.inputValue(m_maxVal).asFloat();

	if (m_partioData == nullptr)
		return (MS::kFailure);

	unsigned int numParticles = m_partioData->numParticles() - removeParticles.size();
	if (part.count() >= numParticles)
	{
		MVectorArray outX, outV;
		MDoubleArray outUserScalarColor;
		MVectorArray outUserVectorRotation;
		if (nucleusFix)
		{
			outX.setLength(part.count());
			if (m_velAttr.attributeIndex != 0xffffffff)
				outV.setLength(part.count());
			if (m_userColorAttr.attributeIndex != 0xffffffff)
				outUserScalarColor.setLength(part.count());
			if (m_userRotationAttr.attributeIndex != 0xffffffff)
				outUserVectorRotation.setLength(part.count());
		}
		else
		{
			outX.setLength(numParticles);
			if (m_velAttr.attributeIndex != 0xffffffff)
				outV.setLength(numParticles);
			if (m_userColorAttr.attributeIndex != 0xffffffff)
				outUserScalarColor.setLength(numParticles);
			if (m_userRotationAttr.attributeIndex != 0xffffffff)
				outUserVectorRotation.setLength(numParticles);
		}
		unsigned int currentIndex = 0;
		for (int i = 0; i < m_partioData->numParticles(); i++)
		{
			if (m_idAttr.attributeIndex != 0xffffffff)
			{
				const int id = *m_partioData->data<int>(m_idAttr, i);
				if (std::find(removeParticles.begin(), removeParticles.end(), id) != removeParticles.end())
					continue;			// skip this particle
			}

			if (m_posAttr.attributeIndex != 0xffffffff)
			{
				const float *pos = m_partioData->data<float>(m_posAttr, i);
				outX[currentIndex].x = pos[0];
				outX[currentIndex].y = pos[1];
				outX[currentIndex].z = pos[2];
			}

			if (m_velAttr.attributeIndex != 0xffffffff)
			{
				const float *vel = m_partioData->data<float>(m_velAttr, i);
				outV[currentIndex].x = vel[0];
				outV[currentIndex].y = vel[1];
				outV[currentIndex].z = vel[2];
			}

			if (m_userColorAttr.attributeIndex != 0xffffffff)
			{
				const float *d = m_partioData->data<float>(m_userColorAttr, i);
				if (m_userColorAttr.type == Partio::FLOAT)
					outUserScalarColor[currentIndex] = clampValue(*d, minVal, maxVal);
				else if (m_userColorAttr.type == Partio::VECTOR)		// use the norm in case of a vector
					outUserScalarColor[currentIndex] = clampValue(sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]), minVal, maxVal);
				else 
					outUserScalarColor[currentIndex] = 0.0;
			}

			if (m_userRotationAttr.attributeIndex != 0xffffffff)
			{
				const float *r = m_partioData->data<float>(m_userRotationAttr, i);
				if (m_userRotationAttr.type == Partio::VECTOR)
				{
					outUserVectorRotation[currentIndex].x = r[0];
					outUserVectorRotation[currentIndex].y = r[1];
					outUserVectorRotation[currentIndex].z = r[2];
				}
				else
				{
					outUserVectorRotation[currentIndex].x = 0.0;
					outUserVectorRotation[currentIndex].y = 0.0;
					outUserVectorRotation[currentIndex].z = 0.0;
				}
			}
			currentIndex++;
		}

		if (nucleusFix)
		{
			for (int i = numParticles; i < (int)part.count(); i++)
			{
				if (m_posAttr.attributeIndex != 0xffffffff)
				{
					outX[currentIndex].x = 1000.0 + currentIndex;
					outX[currentIndex].y = 1000.0;
					outX[currentIndex].z = 1000.0;
				}
				if (m_velAttr.attributeIndex != 0xffffffff)
				{
					outV[currentIndex].x = 0.0;
					outV[currentIndex].y = 0.0;
					outV[currentIndex].z = 0.0;
				}
				if (m_userColorAttr.attributeIndex != 0xffffffff)
				{
					outUserScalarColor[currentIndex] = 0.0;
				}
				if (m_userRotationAttr.attributeIndex != 0xffffffff)
				{
					outUserVectorRotation[currentIndex].x = 0.0;
					outUserVectorRotation[currentIndex].y = 0.0;
					outUserVectorRotation[currentIndex].z = 0.0;
				}
				currentIndex++;
			}
		}

		if (m_posAttr.attributeIndex != 0xffffffff)
			part.setPerParticleAttribute("position", outX);
		if (m_velAttr.attributeIndex != 0xffffffff)
			part.setPerParticleAttribute("velocity", outV);
		if (m_userColorAttr.attributeIndex != 0xffffffff)
			part.setPerParticleAttribute("userScalarColor", outUserScalarColor);
		if (m_userRotationAttr.attributeIndex != 0xffffffff)
			part.setPerParticleAttribute("userVectorRotation", outUserVectorRotation);
	}
	else
	{ 
		MVectorArray outX, outV;
		MDoubleArray outUserScalarColor;
		MVectorArray outUserVectorRotation;
		outX.setLength(part.count());
		if (m_velAttr.attributeIndex != 0xffffffff)
			outV.setLength(part.count());
		if (m_userColorAttr.attributeIndex != 0xffffffff)
			outUserScalarColor.setLength(part.count());
		if (m_userRotationAttr.attributeIndex != 0xffffffff)
			outUserVectorRotation.setLength(part.count());

		unsigned int currentIndex = 0;
 		for (int i = 0; i < (int) part.count(); i++)
 		{
			if (m_idAttr.attributeIndex != 0xffffffff)
			{
				const int id = *m_partioData->data<int>(m_idAttr, i);
				if (std::find(removeParticles.begin(), removeParticles.end(), id) != removeParticles.end())
					continue;			// skip this particle
			}

			if (m_posAttr.attributeIndex != 0xffffffff)
			{
				const float *pos = m_partioData->data<float>(m_posAttr, i);
				outX[currentIndex].x = pos[0];
				outX[currentIndex].y = pos[1];
				outX[currentIndex].z = pos[2];
			}
			if (m_velAttr.attributeIndex != 0xffffffff)
			{
				const float *vel = m_partioData->data<float>(m_velAttr, i);
				outV[currentIndex].x = vel[0];
				outV[currentIndex].y = vel[1];
				outV[currentIndex].z = vel[2];
			}
			if (m_userColorAttr.attributeIndex != 0xffffffff)
			{
				const float *d = m_partioData->data<float>(m_userColorAttr, i);
				if (m_userColorAttr.type == Partio::FLOAT)
					outUserScalarColor[currentIndex] = clampValue(*d, minVal, maxVal);
				else if (m_userColorAttr.type == Partio::VECTOR)
					outUserScalarColor[currentIndex] = clampValue(sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]), minVal, maxVal);
				else 
					outUserScalarColor[currentIndex] = 0.0;
			}

			if (m_userRotationAttr.attributeIndex != 0xffffffff)
			{
				const float *r = m_partioData->data<float>(m_userRotationAttr, i);
				if (m_userRotationAttr.type == Partio::VECTOR)
				{
					outUserVectorRotation[currentIndex].x = r[0];
					outUserVectorRotation[currentIndex].y = r[1];
					outUserVectorRotation[currentIndex].z = r[2];
				}
				else
				{
					outUserVectorRotation[currentIndex].x = 0.0;
					outUserVectorRotation[currentIndex].y = 0.0;
					outUserVectorRotation[currentIndex].z = 0.0;
				}
			}
			currentIndex++;
 		}
		if (m_posAttr.attributeIndex != 0xffffffff)
		{
			part.setPerParticleAttribute("position", outX);
			fnOutPos.setLength(numParticles - part.count());
		}
		if (m_velAttr.attributeIndex != 0xffffffff)
		{
			part.setPerParticleAttribute("velocity", outV);
			fnOutVel.setLength(numParticles - part.count());
		}
		if (m_userColorAttr.attributeIndex != 0xffffffff)
		{
			part.setPerParticleAttribute("userScalarColor", outUserScalarColor);
			fnOutUserScalarColor.setLength(numParticles - part.count());
		}
		if (m_userRotationAttr.attributeIndex != 0xffffffff)
		{
			part.setPerParticleAttribute("userVectorRotation", outUserVectorRotation);
			fnOutUserVectorRotation.setLength(numParticles - part.count());
		}

		currentIndex = 0;
		for (int i = part.count(); i < m_partioData->numParticles(); i++)
		{
			if (m_idAttr.attributeIndex != 0xffffffff)
			{
				const int id = *m_partioData->data<int>(m_idAttr, i);
				if (std::find(removeParticles.begin(), removeParticles.end(), id) != removeParticles.end())
					continue;			// skip this particle
			}

			if (m_posAttr.attributeIndex != 0xffffffff)
			{
				const float *pos = m_partioData->data<float>(m_posAttr, i);
				fnOutPos[currentIndex].x = pos[0];
				fnOutPos[currentIndex].y = pos[1];
				fnOutPos[currentIndex].z = pos[2];
			}
			if (m_velAttr.attributeIndex != 0xffffffff)
			{
				const float *vel = m_partioData->data<float>(m_velAttr, i);
				fnOutVel[currentIndex].x = vel[0];
				fnOutVel[currentIndex].y = vel[1];
				fnOutVel[currentIndex].z = vel[2];
			}
			if (m_userColorAttr.attributeIndex != 0xffffffff)
			{
				const float *d = m_partioData->data<float>(m_userColorAttr, i);
				if (m_userColorAttr.type == Partio::FLOAT)
					fnOutUserScalarColor[currentIndex] = clampValue(*d, minVal, maxVal);
				else if (m_userColorAttr.type == Partio::VECTOR)
					fnOutUserScalarColor[currentIndex] = clampValue(sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]), minVal, maxVal);
				else 
					fnOutUserScalarColor[currentIndex] = 0.0;
			}
			if (m_userRotationAttr.attributeIndex != 0xffffffff)
			{
				if (m_userRotationAttr.type == Partio::VECTOR)
				{
					const float *r = m_partioData->data<float>(m_userRotationAttr, i);
					fnOutUserVectorRotation[currentIndex].x = r[0];
					fnOutUserVectorRotation[currentIndex].y = r[1];
					fnOutUserVectorRotation[currentIndex].z = r[2];
				}
				else
				{
					fnOutUserVectorRotation[currentIndex].x = 0.0;
					fnOutUserVectorRotation[currentIndex].y = 0.0;
					fnOutUserVectorRotation[currentIndex].z = 0.0;
				}
			}
			currentIndex++;
		}
	}

	status = hOut.set(dOutput);

	if (!status)
	{
		std::cout << "status\n";
		status.perror("Error: hOut.set(dOutput)");
		return(status);
	}
	
	block.setClean( plug );

	return( MS::kSuccess );
}


bool PartioEmitter::readParticles(const std::string &fileName, const std::string &colorAttrName, const std::string &rotationAttrName)
{
	if (m_partioData != nullptr)
		m_partioData->release();

	m_partioData = Partio::read(fileName.c_str());
	if (!m_partioData)
		return false;

	m_userColorAttr.attributeIndex = -1;
	m_userRotationAttr.attributeIndex = -1;
	m_velAttr.attributeIndex = -1;
	m_idAttr.attributeIndex = -1;
	for (int i = 0; i < m_partioData->numAttributes(); i++)
	{
		Partio::ParticleAttribute attr;
		m_partioData->attributeInfo(i, attr);
		if (attr.name == "position")
			m_posAttr = attr;
		else if (attr.name == "velocity")
			m_velAttr = attr;
		else if (attr.name == "id")
			m_idAttr = attr;

		if (attr.name == colorAttrName)
			m_userColorAttr = attr;
		if (attr.name == rotationAttrName)
			m_userRotationAttr = attr;
	}

	MGlobal::displayInfo(MString("# particles: ") + m_partioData->numParticles());

	return true;
}

std::string PartioEmitter::zeroPadding(const unsigned int number, const unsigned int length)
{
	std::ostringstream out;
	out << std::internal << std::setfill('0') << std::setw(length) << number;
	return out.str();
}

float PartioEmitter::clampValue(const float value, const float min, const float max)
{
	float res = (value - min) / (max - min);
	// clamp
	res = std::max(res, 0.0f);
	res = std::min(res, 1.0f);
	return res;
}

std::string PartioEmitter::convertFileName(const std::string &inputFileName, const unsigned int currentFrame)
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

