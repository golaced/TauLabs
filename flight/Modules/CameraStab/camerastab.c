/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup CameraStab Camera Stabilization Module
 * @brief Camera stabilization module
 * Updates accessory outputs with values appropriate for camera stabilization
 * @{
 *
 * @file       camerastab.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Stabilize camera against the roll pitch and yaw of aircraft
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * Output object: Accessory
 *
 * This module will periodically calculate the output values for stabilizing the camera
 *
 * UAVObjects are automatically generated by the UAVObjectGenerator from
 * the object definition XML file.
 *
 * Modules have no API, all communication to other modules is done through UAVObjects.
 * However modules may use the API exposed by shared libraries.
 * See the OpenPilot wiki for more details.
 * http://www.openpilot.org/OpenPilot_Application_Architecture
 *
 */

#include "openpilot.h"

#include "accessorydesired.h"
#include "attitudeactual.h"
#include "camerastabsettings.h"
#include "cameradesired.h"
#include "hwsettings.h"

//
// Configuration
//
#define SAMPLE_PERIOD_MS		10

// Private types

// Private variables

static uint8_t camerastabEnabled = 0;

// Private functions
static void attitudeUpdated(UAVObjEvent* ev);
static float bound(float val);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t CameraStabInitialize(void)
{
	static UAVObjEvent ev;

	HwSettingsInitialize();
	uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];
	HwSettingsOptionalModulesGet(optionalModules);
	if (optionalModules[HWSETTINGS_OPTIONALMODULES_CAMERASTAB] == HWSETTINGS_OPTIONALMODULES_ENABLED) {
		camerastabEnabled=1;
	} else {
		camerastabEnabled=0;
	}

	if (camerastabEnabled) {

		AttitudeActualInitialize();

		ev.obj = AttitudeActualHandle();
		ev.instId = 0;
		ev.event = 0;

		CameraStabSettingsInitialize();
		CameraDesiredInitialize();

		EventPeriodicCallbackCreate(&ev, attitudeUpdated, SAMPLE_PERIOD_MS / portTICK_RATE_MS);

		return 0;
	}

	return -1;
}

/* stub: module has no module thread */
int32_t CameraStabStart(void)
{
	return 0;
}

MODULE_INITCALL(CameraStabInitialize, CameraStabStart)

static void attitudeUpdated(UAVObjEvent* ev)
{
	if (ev->obj != AttitudeActualHandle())
		return;

	float attitude;
	float output;
	AccessoryDesiredData accessory;

	CameraStabSettingsData cameraStab;
	CameraStabSettingsGet(&cameraStab);

	// Read any input channels
	float inputs[3] = {0,0,0};
	if(cameraStab.Inputs[CAMERASTABSETTINGS_INPUTS_ROLL] != CAMERASTABSETTINGS_INPUTS_NONE) {
		if(AccessoryDesiredInstGet(cameraStab.Inputs[CAMERASTABSETTINGS_INPUTS_ROLL] - CAMERASTABSETTINGS_INPUTS_ACCESSORY0, &accessory) == 0)
			inputs[0] = accessory.AccessoryVal * cameraStab.InputRange[CAMERASTABSETTINGS_INPUTRANGE_ROLL];
	}
	if(cameraStab.Inputs[CAMERASTABSETTINGS_INPUTS_PITCH] != CAMERASTABSETTINGS_INPUTS_NONE) {
		if(AccessoryDesiredInstGet(cameraStab.Inputs[CAMERASTABSETTINGS_INPUTS_PITCH] - CAMERASTABSETTINGS_INPUTS_ACCESSORY0, &accessory) == 0)
			inputs[1] = accessory.AccessoryVal * cameraStab.InputRange[CAMERASTABSETTINGS_INPUTRANGE_PITCH];
	}
	if(cameraStab.Inputs[CAMERASTABSETTINGS_INPUTS_YAW] != CAMERASTABSETTINGS_INPUTS_NONE) {
		if(AccessoryDesiredInstGet(cameraStab.Inputs[CAMERASTABSETTINGS_INPUTS_YAW] - CAMERASTABSETTINGS_INPUTS_ACCESSORY0, &accessory) == 0)
			inputs[2] = accessory.AccessoryVal * cameraStab.InputRange[CAMERASTABSETTINGS_INPUTRANGE_YAW];
	}

	// Set output channels
	AttitudeActualRollGet(&attitude);
	output = bound((attitude + inputs[0]) / cameraStab.OutputRange[CAMERASTABSETTINGS_OUTPUTRANGE_ROLL]);
	CameraDesiredRollSet(&output);

	AttitudeActualPitchGet(&attitude);
	output = bound((attitude + inputs[1])  / cameraStab.OutputRange[CAMERASTABSETTINGS_OUTPUTRANGE_PITCH]);
	CameraDesiredPitchSet(&output);

	AttitudeActualYawGet(&attitude);
	output = bound((attitude + inputs[2])  / cameraStab.OutputRange[CAMERASTABSETTINGS_OUTPUTRANGE_YAW]);
	CameraDesiredYawSet(&output);

}

float bound(float val)
{
	return (val > 1) ? 1 : 
		(val < -1) ? -1 :
		val;
}
/**
  * @}
  */

/**
 * @}
 */