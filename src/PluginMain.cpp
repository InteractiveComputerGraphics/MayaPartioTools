
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MApiNamespace.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MDrawRegistry.h>
#include <stdio.h>

// nodes
#include "PartioEmitter.h"
#include "PartioVisualizer.h"
#include "PartioVisualizerDrawOverride.h"

bool RegisterPluginUI()
{
	// Create menu
	MGlobal::executeCommand(MString("string $mpt_root_menu = `menu -l \"Partio Tools\" -p MayaWindow -allowOptionBoxes true`"));
	MGlobal::executeCommand(MString("menuItem -label \"Create visualizer node\" -command \"createPartioVisualizer()\""));
	MGlobal::executeCommand(MString("menuItem -label \"Create emitter node\" -command \"createPartioEmitter()\""));

	return true;
}

bool DeregisterPluginUI()
{
	MGlobal::executeCommand(MString("deleteUI -m $mpt_root_menu"));

	// delete template editors
	MGlobal::executeCommand(MString("MPT_RemoveEditorTemplate(\"PartioEmitterNode\")"));
	MGlobal::executeCommand(MString("MPT_RemoveEditorTemplate(\"PartioVisualizerNode\")"));

	return true;
}


MStatus initializePlugin(MObject obj) 
{
	MStatus status;
	MFnPlugin plugin(obj, "Jan Bender", "1.0", "Any");

	MString pluginPath = plugin.loadPath();
	MString scriptPath = pluginPath + "/scripts";

	// add path of plugin to script path
#ifdef WIN32
	MGlobal::executeCommand(MString("$s=`getenv \"MAYA_SCRIPT_PATH\"`; putenv \"MAYA_SCRIPT_PATH\" ($s + \";") + scriptPath + "\")");
#else
	MGlobal::executeCommand(MString("$s=`getenv \"MAYA_SCRIPT_PATH\"`; putenv \"MAYA_SCRIPT_PATH\" ($s + \":") + scriptPath + "\")");
#endif
	
	// execute the specified mel script
	status = MGlobal::sourceFile("MayaPartioTools.mel");

	if (MGlobal::mayaState() == MGlobal::kInteractive)
	{
		MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
		if (renderer != 0 && renderer->drawAPIIsOpenGL())
		{
			PartioVisualizerDrawOverride::initShaders(scriptPath);
		}
	}

	status = plugin.registerNode("PartioVisualizerNode", PartioVisualizer::id,
		&PartioVisualizer::creator, &PartioVisualizer::initialize,
		MPxNode::kLocatorNode, &PartioVisualizer::drawDbClassification);
	if (!status)
	{
		status.perror("register PartioVisualizer");
		return status;
	}

	status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
		PartioVisualizer::drawDbClassification,
		"PartioVisualizerDrawOverride",
		PartioVisualizerDrawOverride::creator);
	if (!status)
	{
		status.perror("register PartioVisualizerDrawOverride");
		return status;
	}

	status = plugin.registerNode("PartioEmitterNode", PartioEmitter::m_id,
		&PartioEmitter::creator, &PartioEmitter::initialize,
		MPxNode::kEmitterNode);
	if (!status)
	{
		status.perror("register PartioEmitter");
		return status;
	}

	if (!RegisterPluginUI())
	{
		status.perror("Failed to create UI");
		return status;
	}

	return status;
}


MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterNode(PartioVisualizer::id);
	if (!status)
	{
		status.perror("deregister PartioVisualizer");
		return status;
	}

	status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
		PartioVisualizer::drawDbClassification,
		"PartioVisualizerDrawOverride");
	if (!status)
	{
		status.perror("deregister PartioVisualizerDrawOverride");
		return status;
	}

	if (MGlobal::mayaState() == MGlobal::kInteractive)
	{
		MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
		if (renderer != 0 && renderer->drawAPIIsOpenGL())
		{
			PartioVisualizerDrawOverride::deleteShaders();
		}
	}

	status = plugin.deregisterNode(PartioEmitter::m_id);
	if (!status)
	{
		status.perror("deregister PartioEmitter");
		return status;
	}

	if (!DeregisterPluginUI())
	{
		status.perror("Failed to deregister UI");
		return status;
	}

	return status;
}

