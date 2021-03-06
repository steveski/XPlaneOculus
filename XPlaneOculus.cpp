//
//  XPlaneOculus.h
//  XPlaneOculus
//
//  Created by Mike Akers on 5/13/13.
//  Copyright (c) 2013 Runway 12. All rights reserved.
//

#define _USE_MATH_DEFINES

#include "XPlaneOculus.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define XPLM200

#if IBM

#include <windows.h>
#include <stdio.h>

#include <cmath>
#endif
#if LIN
#include <GL/gl.h>
#else
#if __GNUC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif


#include "XPLMPlugin.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMMenus.h"
#include "XPLMUtilities.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"
#include "XPLMScenery.h"
#include "XPLMCamera.h"

//#include "OculusHandler.h"

using namespace OVR;

float lastSensorYawDegrees = 0.0f;
float lastSensorPitchDegrees = 0.0f;
float lastSensorRollDegrees = 0.0f;
float lastTheta = 0.0f;
float lastPsi = 0.0f;
float lastPhi = 0.0f;

int 	MyOrbitPlaneFunc(
                         XPLMCameraPosition_t * outCameraPosition,
                         int                  inIsLosingControl,
                         void *               inRefcon)
;

int	SDK200TestsDrawObject(
                          XPLMDrawingPhase     inPhase,
                          int                  inIsBefore,
                          void *               inRefcon);


void menuHandler(
                            void *               inMenuRef,
                            void *               inItemRef);

int keepControl;

XPLMCommandRef resetCommand = NULL;

int    resetCommandHandler(XPLMCommandRef       inCommand,         
                               XPLMCommandPhase     inPhase,
                               void *               inRefcon);

int XPluginDrawCallback(XPLMDrawingPhase phase, int isBefore, void* refcon);

PLUGIN_API int XPluginStart(
                            char *		outName,
                            char *		outSig,
                            char *		outDesc)
{
    
    XPLMDebugString("Starting oculus plugin\n");
    
	XPLMMenuID	PluginMenu;
	int			PluginSubMenuItem;
    
	strcpy(outName, "Instructions");
	strcpy(outSig, "Runway12.XPlaneOculus");
	strcpy(outDesc, "A plugin to enable head tracking for the Oculus Rift VR headset.");
    
    // Create our menu
	PluginSubMenuItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "Oculus Plugin", NULL, 1);
    
	PluginMenu = XPLMCreateMenu("Oculus Plugin", XPLMFindPluginsMenu(), PluginSubMenuItem, menuHandler, NULL);
    XPLMAppendMenuItem(PluginMenu, "init", (void *)"init", 1);
	XPLMAppendMenuItem(PluginMenu, "start", (void *)"start", 1);
    XPLMAppendMenuItem(PluginMenu, "stop", (void *)"stop", 1);
    

    resetCommand = XPLMCreateCommand("OCC/reset", "Counter Up");
    
    // Register our custom commands
    XPLMRegisterCommandHandler(resetCommand,           // in Command name
                               resetCommandHandler,    // in Handler
                               1,                          // Receive input before plugin windows.
                               (void *) 0);                // inRefcon.
    
    
    XPLMRegisterDrawCallback(XPluginDrawCallback, xplm_Phase_Airplanes, 1, NULL);
    
//
//    System::Init(Log::ConfigureDefaultLog(LogMask_All));
//    
//    Ptr<DeviceManager> pManager;
//    Ptr<HMDDevice> pHMD;
//    pManager = *DeviceManager::Create();
//    pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
//    
//    Ptr<SensorDevice> pSensor;
//    pSensor = *pHMD->GetSensor();
//    
//    SensorFusion SFusion;
//    if (pSensor) {
//        SFusion.AttachToSensor(pSensor);
//    }
    
    
	return 1;
}

void menuHandler(
                            void *               inMenuRef,
                            void *               inItemRef) {
    
    
    // Change all the strings
    if (strcmp((char *) inItemRef, "start") == 0) {
        SFusion.Reset();
        XPLMDebugString("Starting head tracking\n");
        keepControl = 1;
        XPLMControlCamera(xplm_ControlCameraForever, MyOrbitPlaneFunc, NULL);
    } else if (strcmp((char *) inItemRef, "stop") == 0) {
        XPLMDebugString("Stopping head tracking\n");
        keepControl = 0;
    } else if (strcmp((char *) inItemRef, "init") == 0) {
        XPLMDebugString("Init occulus rift\n");
        System::Init(Log::ConfigureDefaultLog(LogMask_All));

        pManager = *DeviceManager::Create();
        pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
        char buffer [1000];
        
        OVR::HMDInfo hmd;
        if (pHMD->GetDeviceInfo(&hmd)) {
            XPLMDebugString("We have an HMDinfo for a HMD!\n");

            sprintf(buffer, "DisplayDeviceName: %s", hmd.DisplayDeviceName);
            XPLMDebugString(buffer);
            
            sprintf(buffer, "InterpupillaryDistance: %f", hmd.InterpupillaryDistance);
            XPLMDebugString(buffer);            
        }

        pSensor = *pHMD->GetSensor();

        if (pSensor) {
            SFusion.AttachToSensor(pSensor);
//            SFusion.SetPredictionEnabled(true);
            
        }
        
        
        if (pSensor != NULL) {
            XPLMDebugString("Oculus intialized.\n");
        } else {
            XPLMDebugString("No sensor object created. Something may have gone wrong when initializing oculus rift :/\n");
        }
    }
}


int    resetCommandHandler(XPLMCommandRef       inCommand,
                           XPLMCommandPhase     inPhase,
                           void *               inRefcon) {
    SFusion.Reset();
	return 1;
}

int 	MyOrbitPlaneFunc(
                     XPLMCameraPosition_t * outCameraPosition,
                     int                  inIsLosingControl,
                     void *               inRefcon)
{
    if (outCameraPosition && !inIsLosingControl)
	{
        char buffer [1000];
        
        XPLMSetDataf(XPLMFindDataRef("sim/cockpit2/camera/camera_offset_heading"), 0.0f);
        XPLMSetDataf(XPLMFindDataRef("sim/cockpit2/camera/camera_offset_roll"), 0.0f);
        
        //get the plane's position
        float planeX = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/local_x"));
        float planeY = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/local_y"));
        float planeZ = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/local_z"));
        
        //get the vector to the players head relative to the plane's center of gravity.
        // Fudge factors only work for Cessna 172, sorry.
        float headX = XPLMGetDataf(XPLMFindDataRef("sim/aircraft/view/acf_peX")) + 0.35f; // back forward axis... + is back
        float headY = XPLMGetDataf(XPLMFindDataRef("sim/aircraft/view/acf_peY")) - 0.135f; // horizontal axis + is to the right
        float headZ = XPLMGetDataf(XPLMFindDataRef("sim/aircraft/view/acf_peZ")) + 0.4f; // vertical axis + is up
        
        //the planes orientation in euler angles for whatever reason, in theta isn't lined up right, so there's another fudge factor
        float theta = (XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/theta")) /*+ 25.0f*/) * ((float)M_PI / 180.0f);
        float psi = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/psi"))  * ((float)M_PI / 180.0f);
        float phi = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/phi"))  * ((float)M_PI / 180.0f);
        
        sprintf(buffer, "theta:%f psi:%f phi:%f \n", theta, psi, phi);
//        XPLMDebugString(buffer);

        float q[4];
        XPLMGetDatavf(XPLMFindDataRef("sim/flightmodel/position/q"), q, 0, 4);
        
        //Make a quaternion for our rotation
        Quatf planeQuat = Quatf(q[1], q[2], q[3], q[0]);
        //planeQuat->Normalize();
        
        //rotate headVector by plane quat
        Vector3f *headVector = new Vector3f(headX, headY, headZ);
        Vector3f rotatedHeadVec = planeQuat.Rotate(*headVector);
        
        //add in oculus orientation
        Quatf hmdOrient = SFusion.GetOrientation();
        
        
        float sensorYaw = 0.0f;
        float sensorPitch = 0.0f;
        float sensorRoll = 0.0f;
        
        hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&sensorYaw, &sensorPitch, &sensorRoll);
        sensorPitch = sensorPitch;// + 0.6109f;
                
        //output our final camera position and orientation
        outCameraPosition->x = planeX - rotatedHeadVec.y;
        outCameraPosition->y = planeY + rotatedHeadVec.z;
        outCameraPosition->z = planeZ + rotatedHeadVec.x;

        outCameraPosition->pitch = (theta + sensorPitch)  * (180.0f / (float)M_PI);
        outCameraPosition->heading = (psi - sensorYaw)  * (180.0f / (float)M_PI);
        outCameraPosition->roll = (phi - sensorRoll) * (180.0f / (float)M_PI);
        
//        sprintf(buffer, "hi X:%f Y:%f Z:%f      theta:%f psii:%f phi:%f \n",
//                outCameraPosition->x,
//                outCameraPosition->y,
//                outCameraPosition->z,
//                outCameraPosition->pitch,
//                outCameraPosition->heading,
//                outCameraPosition->roll);
//        XPLMDebugString(buffer);
	}
    
    
    return keepControl;
}


PLUGIN_API void	XPluginStop(void)
{
    XPLMDebugString("Stoping oculus plugin\n");
    
    
    OVR::System::Destroy();
    
    
    
    XPLMReloadPlugins();
    
}

PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, long inMsg, void * inParam)
{
}

//==============================================================================
// Rendering callback
//==============================================================================
int XPluginDrawCallback(XPLMDrawingPhase phase, int isBefore, void* refcon)
{
	switch (phase) {
		case xplm_Phase_Airplanes: {
            float m[16];
            glMatrixMode(GL_PROJECTION);
            glGetFloatv(GL_PROJECTION_MATRIX,m);
            {
                float zNear = 0.1f;
                float zFar = 5.0f;
                
                m[2*4+2] = -(zFar + zNear) / (zFar - zNear);
                m[3*4+2] = -2 * zNear * zFar / (zFar - zNear);
            }
            glLoadMatrixf(m);
            glMatrixMode(GL_MODELVIEW);
            break;

        }
			break;
		default:
			break;
	}
	return 1;
}


