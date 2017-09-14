/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <android/sensor.h>
//#include <android/looper.h>

#define POLLONCE 1


typedef struct _sensor_native{
	ASensorEventQueue* queue;
	ASensorManager *manager;
	ASensor const *Sensor;
	ALooper * looper;
	int32_t rate;
	int32_t max_delay;
	int32_t status;
	int count;
	ASensorList list;
}SensorNative;



ASensorList mSensorList;

int sensor_list_show(SensorNative *msn)
{
	printf("\tName\t\t\t StringType\t Type\t Vendor\t ReportMode\t IsWakeUp\t Resolution\t MinDelay\t MaxFifo\t FifoReserved\n ");
	for(int i = 0; i < msn->count; i++)
	{
		printf("\t%s\t\t\t %s\t %d\t %s\t %d\t %d\t %f\t %d\t %d\t %d\t\n",
			ASensor_getName(msn->list[i]), ASensor_getStringType(msn->list[i]), \
			ASensor_getType(msn->list[i]), ASensor_getVendor(msn->list[i]), \
			ASensor_getReportingMode(msn->list[i]) ? 1 : 0,ASensor_isWakeUpSensor(msn->list[i]), \
			ASensor_getResolution(msn->list[i]), ASensor_getMinDelay(msn->list[i]),	\
			ASensor_getFifoMaxEventCount(msn->list[i]), ASensor_getFifoReservedEventCount(msn->list[i]));
	}
	return 0;
}


int sensor_callback(int fd, int events, void *data)
{
	SensorNative * msn = (SensorNative *)data;

	//printf("[%s|%d]: %s call!\n", __FILE__, __LINE__, __func__);
	printf("fd:%d\n", fd);

	if(events == ALOOPER_EVENT_INPUT)
	{
		ASensorEvent mSensorEvent;
		while(ASensorEventQueue_getEvents(msn->queue, &mSensorEvent, 1) > 0)
		{
			if(mSensorEvent.type == ASENSOR_TYPE_ACCELEROMETER)
			{
				printf("acc: %f\t %f\t %f\n", mSensorEvent.acceleration.x, mSensorEvent.acceleration.y, mSensorEvent.acceleration.z);
			}
		}
	}

	return 1;
}

int sensor_stop(SensorNative *msn)
{
	msn->status = 0;

	ASensorEventQueue_disableSensor(msn->queue, msn->Sensor);
	ASensorManager_destroyEventQueue(msn->manager, msn->queue);
	return 0;
}

//void *thread_callback(void *arg)
void *sensor_start(void *arg)
{
	int ret = 0;

	SensorNative *msn = (SensorNative *)arg;

	msn->rate = 10000;
	msn->max_delay = 0;
	msn->status = 1;
	
	msn->looper = ALooper_forThread();
	if(!msn->looper)
	{
		printf("ALooper_forThread failed!\n");
		msn->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
		if(!msn->looper)
		{
			printf("ALooper_prepare failed!\n");
			return NULL;
		}
	}
		
	msn->manager = ASensorManager_getInstanceForPackage("sensornative");
	
	msn->count = ASensorManager_getSensorList(msn->manager, &msn->list);
	sensor_list_show(msn);
	msn->Sensor = ASensorManager_getDefaultSensor(msn->manager, ASENSOR_TYPE_ACCELEROMETER);

 	msn->queue = ASensorManager_createEventQueue(msn->manager, msn->looper, 0, sensor_callback, (void *)msn);
	if(!msn->queue)
	{
		printf("ASensorManager_createEventQueue failed!\n");
		return NULL;
	}
	if(msn->Sensor != NULL && msn->looper != NULL)
	{
		ret = ASensorEventQueue_registerSensor(msn->queue, msn->Sensor, msn->rate, msn->max_delay);
		if(ret < 0){
			printf("ASensorEventQueue_registerSensor failed!\n");
			
			ASensorEventQueue_setEventRate(msn->queue, msn->Sensor, msn->rate);
			ret = ASensorEventQueue_enableSensor(msn->queue, msn->Sensor);
			if(ret < 0)
			{
				printf("ASensorEventQueue_enableSensor failed!\n");
			}
			else
				printf("ASensorEventQueue_enableSensor success!\n");
		}
		else
			printf("ASensorEventQueue_registerSensor success!\n");
	}

#ifdef POLLONCE	
	do{
		int ident = ALooper_pollOnce(100, NULL, NULL, NULL);
		switch(ident){
			case ALOOPER_POLL_WAKE:
				printf("ALOOPER_POLL_WAKE\n");
				msn->status = 1;
				break;
			case ALOOPER_POLL_CALLBACK:
				printf("ALOOPER_POLL_CALLBACK\n");
				msn->status = 1;
				break;
			case ALOOPER_POLL_TIMEOUT:
				printf("ALOOPER_POLL_TIMEOUT\n");
				msn->status = 1;
				break;
			case ALOOPER_POLL_ERROR:
				printf("ALOOPER_POLL_ERROR\n");
				msn->status = 0;
				break;
			default:
				break;
				
		}
			
		
	}while(msn->status);

#endif


	return NULL;
}

int main()
{

	int acount = 0;

	SensorNative * msn = (SensorNative *)calloc(1, sizeof(SensorNative));

#if 1
	pthread_t tid;

	pthread_create(&tid, NULL, sensor_start, (void *)msn);
	pthread_detach(tid);
#endif
	//sensor_start(msn);


	while(acount <= 10)
	{
		sleep(1);
		acount ++;
		if(acount % 2 == 0)
		{
			printf("running %d s\n", acount);
		}
	}

	sensor_stop(msn);
	free(msn);

	return 0;
}


