
    /**
    OsciPrime an Open Source Android Oscilloscope
    Copyright (C) 2012  Manuel Di Cerbo, Nexus-Computing GmbH Switzerland
    Copyright (C) 2012  Andreas Rudolf, Nexus-Computing GmbH Switzerland

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

/*
 * trigger.c
 * This file is part of OsciOne
 *
 * Copyright (C) 2010 - Manuel Di Cerbo
 *
 * OsciOne is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * OsciOne is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OsciOne; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
 //Only Bytetrigger, SelectBytePoints is tested

#include <jni.h>
#include <math.h>
#include <stdlib.h>
#include <android/log.h>
#include <time.h>
#include <stdio.h>

#define RISING_EDGE 0

int trigger(signed int* buffer, jint length, signed int trigger, char edge, char singleshot);
static void selectPoints(signed int* buffer, int buflen, signed int* dest, int destlen, int triggerIndex, int nthVal, signed int* bufferPreview, int previewLen);
static double now_ms(void);



/*
JNI STUFF
*/
jint
Java_ch_nexuscomputing_android_osciprimeics_OsciPrimeService_nativeTrigger(JNIEnv * env, jclass clazz, jintArray arr, jint len, jint tr, jboolean edge, jint singleshot){
//	jint* buffer;
	int res = 0;
	signed int* buffer = (signed int*)(malloc(len*sizeof(signed int)));
	(*env)->GetIntArrayRegion(env, arr, 0, len, buffer);
	//buffer = (*env)->GetIntArrayElements(env, arr, NULL);
//	if (buffer == NULL) {
//		__android_log_write(ANDROID_LOG_ERROR,"TRIGGER","ERROR");
//		return 0; /* exception occurred */
//	} 

	res = trigger(buffer, len, (signed int)tr, (char)edge, (char)singleshot);
	free(buffer);
//	(*env)->ReleaseIntArrayElements(env, arr, buffer, 0);//release

	return res;
}

jint
Java_ch_nexuscomputing_android_osciprimeics_OsciPrimeService_nativeTriggerBuffer(JNIEnv * env, jclass clazz, jobject buf, jint len, jint tr, jboolean edge){
	int res = 0;
	signed int* buffer = (signed int*)(*env)->GetDirectBufferAddress(env,buf);
	res = trigger(buffer, len, tr, (char) edge,0);
	return res;
}




//select points
void
Java_ch_nexuscomputing_android_osciprimeics_OsciPrimeService_nativeInterleave(JNIEnv * env, jclass clazz, jintArray arr, jint buflen, jint destlen, jint trigger, jint nthVal, jintArray interleaveBuffer, jintArray bufferPreview, jint previewLen){
	signed int* buffer = (signed int*)(*env)->GetPrimitiveArrayCritical(env,arr,0);
	signed int* ch1Interl = (signed int*)(*env)->GetPrimitiveArrayCritical(env,interleaveBuffer,0);
	signed int* ch1preview = (signed int*)(*env)->GetPrimitiveArrayCritical(env,bufferPreview,0);
	
	selectPoints(buffer, buflen, ch1Interl, destlen, trigger, nthVal, ch1preview, previewLen);
	
	(*env)->ReleasePrimitiveArrayCritical(env,bufferPreview,ch1preview,0);
	(*env)->ReleasePrimitiveArrayCritical(env,interleaveBuffer,ch1Interl,0);
	(*env)->ReleasePrimitiveArrayCritical(env,arr, buffer,0);

}

//select points


void
Java_ch_nexuscomputing_android_osciprimeics_OsciPrimeService_nativeInterleaveBuffer(JNIEnv * env, jclass clazz, jobject buf, jint buflen, jintArray jdest, jint destlen, jint trigger, jint nthVal){
	signed int* buffer = (signed int*)(*env)->GetDirectBufferAddress(env,buf);
	signed int* dest = (signed int*)(malloc(destlen*sizeof(signed int)));
	if(buffer == NULL){
		__android_log_write(ANDROID_LOG_ERROR,"selectShortPoints","buffer address is null");
		return;
	}
//	selectPoints(buffer, buflen, dest, destlen, trigger, nthVal);
	(*env)->SetIntArrayRegion(env, jdest, 0, destlen, dest);
	free(dest);
}



/*
	IMPLEMENTATION
*/

static int* indices;
static int len = 0;

int trigger(signed int* buffer, jint length, signed int trigger, char edge, char singleshot){	
	
	int ref = length/2;//this is the reference, "best" expected trigger
	signed int lastSample = 0;
	signed int n;
	int i = 0, j = 0;
	unsigned long long bestCost = ((unsigned long long)length)*((unsigned long long)length);
	unsigned long long curCost = 0;
	int triggerIndex = -1;
	static int start = 0;

	if(edge == RISING_EDGE){
		for(i = 0; i < length; i++){
			n = buffer[i];
			if(n >= trigger && lastSample < trigger){
				if(singleshot == 1){
					triggerIndex = i;
					break;
				}
				curCost = abs(i-ref);
				if(curCost < bestCost){
					bestCost = curCost;
					triggerIndex = i;
				}
			}
			lastSample = n;
		}
		start = 1;
	}else{
		for(i = 0; i < length; i++){
			n = buffer[i];
			if(n <= trigger && lastSample > trigger){
				if(singleshot == 1){
					triggerIndex = i;
					break;
				}
				curCost = abs(i-ref);
				if(curCost < bestCost){
					bestCost = curCost;
					triggerIndex = i;
				}
			}
			lastSample = n;
		}
	}	

	return triggerIndex;
}

static void selectPoints(signed int* buffer, int buflen, signed int* dest, int destlen, int triggerIndex, int nthVal, signed int* bufferPreview, int previewLen){
	int i = 0, j = 0;
	int start = triggerIndex-destlen/2*nthVal;
	int stop = triggerIndex+destlen/2*nthVal;
	if(start < 0){
		start = 0;//no trigger found
	}
	if(stop > buflen-1){
		start = buflen-1-destlen*nthVal;//trigger to far away
		//maybe it should be -2 ...
		if(start < 0)
			start = 0;
	}
	
	j = start;
	for(i = 0; i < destlen; i++){
		if(j > buflen-1){//so the interleave would overlap, that is not good
			dest[i] = 0;//this should actually never happen unless start is miss
						//calculated
		}else{
			dest[i] = buffer[j];
		}
		j += nthVal;
	}
	
    if(previewLen != -1){//ch1Preview has double the capacity of previewLen
        float seglen = ((float)buflen/(float)previewLen);
        int i = 0;
        float j = 0;
        for(i = 0; i < previewLen; i++){
            int min = 1 << 10, max = 0;
            int v = 0;
            for(j = i*seglen; j < (i+1)*seglen; j++){
                v = buffer[(int)j];
                if(v > max) max = v;
                if(v < min) min = v;
            }
            bufferPreview[2*i] = min;
            bufferPreview[2*i+1] = max;
        }
    }
}

/* return current time in milliseconds */
static double now_ms(void){
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return 1000.0*res.tv_sec + (double)res.tv_nsec/1e6;
}

